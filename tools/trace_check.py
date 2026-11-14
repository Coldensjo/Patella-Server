r"""Prove that a change keeps the game's behaviour: the packet traces of two builds in a scripted scenario.

Usage, from any folder (the checkout is the one this file is in), on Windows or in WSL:
  python -I tools/trace_check.py [<old> [<new>]] [--scenario NAME] [--stop-round N] [--seed S] [--install PATH]
                                 [--port N] [--qm-port N] [--shm KEY]

<old> is a commit, faithful when left out. <new> is a commit too; left out, it is the working tree (as in
tools/repro_check.py: the files `git add -A` would commit).

What it does, in WSL Ubuntu-24.04:
  1. Builds both trees the way tools/repro_check.py does (one fixed source folder, one RSA key, a cache per tree), but
     with the normal flags, debug information included, since gdb needs the type of RoundNr; in
     ~/.cache/patella-check/build-trace.
  2. For each build, one after the other and at the same paths: makes a runtime copy of the original installation
     (--install, a tar or a folder; default /home/chris/patella-cmp/pristine.tar) in ~/.cache/patella-check/trace/game,
     points the paths of its .tibia at the copy, gives it a scratch SHM key (--shm, default 17011) and scratch query
     manager ports (--qm-port, default 17173, and the three after it for the other entries), seeds a scratch database
     with the build's querymanager-seed (game port --port, default 17171), adds the two test accounts with a new
     character each (900001 'Testa Alpha', male, and 900002 'Testb Beta', female; neither may have a player file) and
     starts the build's query manager.
  3. Runs the game under gdb: rand() is seeded again with --seed (default 4242) right after the game's own
     srand(time(NULL)), at InitSignalHandler; a breakpoint on SetRoundNr prints every round number into the console,
     and the scenario's clients (tools/trace/run_scenario.py; --scenario, default full) act in fixed rounds after it.
     At the start of the AdvanceGame call of round --stop-round (default 232, after the last step of full) gdb
     delivers SIGQUIT and the game shuts down by itself. This is the SCENARIO and STOPROUND mode of the reference
     project's notes/boot/compare.sh.
  4. Compares the two traces with tools/trace/compare_traces.py. What follows the wall clock and not the rounds may
     differ and is reported apart: the world light (message 0x82), the "New record: N players are logged in."
     broadcast (it follows the player list, made when the real minute is a multiple of 5) and dates in texts. The
     query manager requests of both runs are compared too, for information (the player list shows up there too).

Only the builds run, never the original bin/game. The ports 7171-7176, the SHM key 10011 and /game are refused, so a
server the owner plays on is never touched. Nothing of usr/ is printed: the seed's output and the game's console stay
in files. Every exit path stops the processes of the run and removes its SHM segment, database and runtime copy. A run
takes about five minutes per build. What the runs leave (consoles, query manager logs, traces, the comparison) is in
~/.cache/patella-check/trace/.

Exit code 0: the traces match apart from the allowed differences, and both runs went through. 1: they differ, or a
run or a build failed. 2: wrong usage.
"""

import argparse
import os
import re
import shutil
import signal
import subprocess
import sys
import time

# No __pycache__ next to the sources.
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import repro_check  # noqa: E402
from repro_check import CheckError, log  # noqa: E402

TRACE_TOOLS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "trace")
DEFAULT_INSTALL = "/home/chris/patella-cmp/pristine.tar"
LIVE_PORTS = range(7171, 7177)
LIVE_SHM = 10011
TEST_ACCOUNTS = [
	(900001, "testa", 1, "Testa Alpha"),
	(900002, "testb", 2, "Testb Beta"),
]
BOOT_SECONDS = 300
STOP_SECONDS = 600


def check_options(options):
	ports = [options["port"]] + [options["qm_port"] + i for i in range(4)]
	for port in ports:
		if port in LIVE_PORTS or not 1024 <= port <= 65535:
			raise CheckError("port %d: not one of 7171-7176 (a live server's) and not below 1024" % port)
	if len(set(ports)) != len(ports):
		raise CheckError("the game port is one of the query manager's (--qm-port and the three after it)")
	if options["shm"] == LIVE_SHM or options["shm"] <= 0:
		raise CheckError("SHM key %d: not 10011 (a live server's)" % options["shm"])
	if os.path.normpath(options["install"]).rstrip("/") == "/game":
		raise CheckError("--install /game: that is the live server's folder; use a copy")


# ---------------------------------------------------------------- WSL side: processes, ports, SHM

def processes():
	"""[(pid, parent pid, name, executable, command line)] of the processes this user can see."""
	found = []
	for entry in os.listdir("/proc"):
		if not entry.isdigit():
			continue
		try:
			with open("/proc/%s/stat" % entry, "rb") as f:
				stat = f.read().decode("utf-8", "replace")
			name = stat[stat.index("(") + 1:stat.rindex(")")]
			parent = int(stat[stat.rindex(")") + 2:].split()[1])
			with open("/proc/%s/cmdline" % entry, "rb") as f:
				command = f.read().replace(b"\0", b" ").decode("utf-8", "replace")
			try:
				executable = os.readlink("/proc/%s/exe" % entry)
			except OSError:
				executable = ""
		except (OSError, ValueError):
			continue
		found.append((int(entry), parent, name, executable, command))
	return found


def alive(pid):
	if not pid:
		return False
	try:
		os.kill(pid, 0)
	except ProcessLookupError:
		return False
	except PermissionError:
		return True
	# A zombie is not alive.
	try:
		with open("/proc/%d/stat" % pid) as f:
			return f.read().rsplit(")", 1)[1].split()[0] != "Z"
	except OSError:
		return False


def kill(pid):
	try:
		os.kill(pid, signal.SIGKILL)
	except (ProcessLookupError, PermissionError):
		pass


def listening(port):
	"""Whether some socket listens on the TCP port."""
	for table in ("/proc/net/tcp", "/proc/net/tcp6"):
		try:
			with open(table) as f:
				lines = f.readlines()[1:]
		except OSError:
			continue
		for line in lines:
			fields = line.split()
			if len(fields) > 3 and fields[3] == "0A" and int(fields[1].rsplit(":", 1)[1], 16) == port:
				return True
	return False


def shm_segment(key):
	"""(id, attached processes) of the SHM segment with the key, or None."""
	output = subprocess.run(["ipcs", "-m"], stdout=subprocess.PIPE, check=False).stdout.decode("utf-8", "replace")
	for line in output.splitlines():
		fields = line.split()
		if len(fields) >= 6 and fields[0].startswith("0x") and int(fields[0], 16) == key:
			return int(fields[1]), int(fields[5])
	return None


def remove_shm(key):
	"""Removes the segment of the key if no process uses it. Returns whether there was one."""
	segment = shm_segment(key)
	if segment is None:
		return False
	if segment[1] != 0:
		raise CheckError("the SHM segment of key %d is in use by %d processes" % (key, segment[1]))
	subprocess.run(["ipcrm", "-m", str(segment[0])], check=False)
	return True


def kill_leftovers(cache, root, key):
	"""Stops what an interrupted earlier run left: processes of the cached builds or with the run folder on their
	command line (gdb, the clients), and its SHM segment. A live server runs from elsewhere and is never matched."""
	builds = os.path.join(cache, "builds") + "/"
	mine = {os.getpid(), os.getppid()}
	for pid, _, name, executable, command in processes():
		if pid in mine:
			continue
		if executable.startswith(builds) or (root + "/") in command:
			log("  stopping a leftover of an earlier run: %s (pid %d)" % (name, pid))
			kill(pid)
	time.sleep(0.5)
	if remove_shm(key):
		log("  removed a leftover SHM segment (key %d)" % key)


# ---------------------------------------------------------------- WSL side: one run

def rewrite_tibia(path, game, shm, qm_port):
	"""Points the .tibia of the runtime copy at the copy, the SHM key and the query manager ports at the scratch
	ones. Returns the world's name."""
	with open(path, encoding="latin-1") as f:
		text = f.read()
	text, count = re.subn(r'(?m)^(\s*[A-Z]+PATH\s*=\s*)"([^"]*)"',
		lambda m: '%s"%s"' % (m.group(1), os.path.join(game, os.path.basename(m.group(2).rstrip("/")))), text)
	if count == 0:
		raise CheckError(".tibia: no paths")
	text, count = re.subn(r"(?m)^(\s*SHM\s*=\s*)\d+", lambda m: "%s%d" % (m.group(1), shm), text)
	if count != 1:
		raise CheckError(".tibia: no SHM key")
	entries = [0]

	def port(m):
		entries[0] += 1
		return "%s%d%s" % (m.group(1), qm_port + entries[0] - 1, m.group(2))

	line = re.search(r"(?m)^\s*QueryManager\s*=.*$", text)
	if line is None:
		raise CheckError(".tibia: no QueryManager")
	replaced = re.sub(r'(\(\s*"[^"]*"\s*,\s*)\d+(\s*,)', port, line.group())
	if not 1 <= entries[0] <= 4:
		raise CheckError(".tibia: %d QueryManager entries" % entries[0])
	text = text[:line.start()] + replaced + text[line.end():]
	world = re.search(r'(?m)^\s*World\s*=\s*"([^"]*)"', text)
	if world is None:
		raise CheckError(".tibia: no World")
	with open(path, "w", encoding="latin-1") as f:
		f.write(text)
	return world.group(1)


def seed(binary, db, game, world, port, log_file):
	"""Seeds the database from the copy and adds the test accounts, as compare.sh does. The output (it names player
	files) goes to log_file only."""
	with open(log_file, "w") as f:
		if subprocess.run([binary, "--db", db, "--game-root", game, "--port", str(port)], stdout=f,
				stderr=subprocess.STDOUT, check=False).returncode != 0:
			raise CheckError("querymanager-seed --game-root failed; see %s" % log_file)
	for account, password, sex, name in TEST_ACCOUNTS:
		result = subprocess.run([binary, "--db", db, "--add-account", str(account), "--password", password, "--world",
			world, "--sex", str(sex), "--new-character", name], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
			check=False)
		output = result.stdout.decode("utf-8", "replace")
		with open(log_file, "a") as f:
			f.write(output)
		ids = [int(i) for i in re.findall(r"(?m)^new character (\d+):", output)]
		if result.returncode != 0 or len(ids) != 1:
			raise CheckError("querymanager-seed --add-account %d failed; see %s" % (account, log_file))
		# A new character starts as a new one only without a player file; only its existence is tested.
		if os.path.exists(os.path.join(game, "usr", "%02d" % (ids[0] % 100), "%d.usr" % ids[0])):
			raise CheckError("new character %d has a player file" % ids[0])


def gdb_script(seed_value, stop_round):
	"""The gdb commands of compare.sh's SCENARIO and STOPROUND mode (breakpoint 2 is the one of the stop round)."""
	return "\n".join([
		"set pagination off",
		"set confirm off",
		"handle SIGALRM SIGUSR1 SIGUSR2 SIGIO SIGHUP SIGPIPE SIGCHLD SIGQUIT SIGTERM SIGINT SIGPWR nostop noprint pass",
		"handle SIG32 SIG33 SIG34 nostop noprint pass",
		"break InitSignalHandler",
		"commands",
		"silent",
		"call (void) srand(%d)" % seed_value,
		"continue",
		"end",
		"break AdvanceGame if RoundNr == %d" % stop_round,
		"commands",
		"silent",
		"delete 2",
		"signal SIGQUIT",
		"end",
		"break SetRoundNr",
		"commands",
		"silent",
		'printf "@@ROUND %u@@\\n", RoundNr',
		"continue",
		"end",
		"run nofork",
		"quit",
		""])


def game_exit(console):
	"""How the game ended, from gdb's last line about it in the console."""
	with open(console, "rb") as f:
		f.seek(max(0, os.path.getsize(console) - 65536))
		tail = f.read().decode("utf-8", "replace")
	found = re.findall(r"\[Inferior 1 \(process \d+\) (exited normally|exited with code \d+)\]", tail)
	return found[-1] if found else "unknown (no exit in the console)"


def run(side, build, options, key, root):
	"""One run of a build. Returns its result: {"ok", "boot", "scenario", "exit"}."""
	out = os.path.join(root, side)
	game = os.path.join(root, "game")
	db = os.path.join(root, "querymanager.sqlite")
	shutil.rmtree(out, ignore_errors=True)
	os.makedirs(out)
	binaries = os.path.join(build, "bin")
	result = {"ok": False, "boot": None, "scenario": None, "exit": None}
	qm = gdb = client = None
	game_pid = None
	try:
		shutil.rmtree(game, ignore_errors=True)
		for suffix in ("", "-wal", "-shm", "-journal"):
			if os.path.exists(db + suffix):
				os.remove(db + suffix)
		if os.path.isdir(options["install"]):
			shutil.copytree(options["install"], game, symlinks=True)
		else:
			os.makedirs(game)
			if subprocess.run(["tar", "-x", "-C", game, "-f", options["install"]], check=False).returncode != 0:
				raise CheckError("cannot unpack %s" % options["install"])
		world = rewrite_tibia(os.path.join(game, ".tibia"), game, options["shm"], options["qm_port"])
		seed(os.path.join(binaries, "querymanager", "querymanager-seed"), db, game, world, options["port"],
			os.path.join(out, "seed.log"))
		qm_log = os.path.join(out, "qm.log")
		with open(os.path.join(out, "qm.stderr"), "w") as f:
			qm = subprocess.Popen([os.path.join(binaries, "querymanager", "querymanager"), "--db", db, "--port",
				str(options["qm_port"]), "--log", qm_log, "--log-level", "info"], stdin=subprocess.DEVNULL,
				stdout=f, stderr=subprocess.STDOUT, start_new_session=True)
		for _ in range(100):
			if listening(options["qm_port"]) or qm.poll() is not None:
				break
			time.sleep(0.1)
		if not listening(options["qm_port"]):
			raise CheckError("%s: the query manager did not start; see %s" % (side, os.path.join(out, "qm.stderr")))
		script = os.path.join(out, "gdb.cmd")
		with open(script, "w") as f:
			f.write(gdb_script(options["seed"], options["stop_round"]))
		console = os.path.join(out, "console")
		with open(console, "wb") as f:
			gdb = subprocess.Popen(["gdb", "-q", "-batch", "-x", script, "--args", os.path.join(binaries, "src", "game")],
				cwd=game, env=dict(os.environ, HOME=game), stdin=subprocess.DEVNULL, stdout=f, stderr=subprocess.STDOUT,
				start_new_session=True)
		for _ in range(300):
			game_pid = next((pid for pid, parent, name, _, _ in processes() if parent == gdb.pid and name == "game"), None)
			if game_pid or gdb.poll() is not None:
				break
			time.sleep(0.1)
		if not game_pid:
			raise CheckError("%s: the game did not start under gdb; see %s" % (side, console))
		start = time.time()
		while time.time() - start < BOOT_SECONDS:
			if not alive(game_pid):
				raise CheckError("%s: the game ended during its boot; see %s" % (side, console))
			if listening(options["port"]) and os.path.isfile(qm_log) and b"getAuctions" in open(qm_log, "rb").read():
				break
			time.sleep(1)
		else:
			raise CheckError("%s: the game did not boot within %d s" % (side, BOOT_SECONDS))
		result["boot"] = round(time.time() - start)
		log("  %s: game (pid %d) booted after %d s; scenario %s" % (side, game_pid, result["boot"], options["scenario"]))
		time.sleep(3)
		with open(os.path.join(out, "client.log"), "w") as f:
			client = subprocess.Popen([sys.executable, "-u", os.path.join(TRACE_TOOLS, "run_scenario.py"), "--scenario",
				options["scenario"], "--out", os.path.join(out, "client"), "--objects", os.path.join(game, "dat",
				"objects.srv"), "--console", console, "--port", str(options["port"]), "--rsa-key", key],
				stdin=subprocess.DEVNULL, stdout=f, stderr=subprocess.STDOUT, start_new_session=True)
		status = client.wait()
		with open(os.path.join(out, "client.log"), encoding="utf-8", errors="replace") as f:
			lines = f.read().splitlines()
		result["scenario"] = "exit status %d (%s)" % (status, lines[-1] if lines else "no output")
		log("  %s: scenario %s" % (side, result["scenario"]))
		try:
			gdb.wait(timeout=STOP_SECONDS)
		except subprocess.TimeoutExpired:
			result["exit"] = "did not stop within %d s after the scenario, killed" % STOP_SECONDS
			log("  %s: the game %s" % (side, result["exit"]))
			return result
		result["exit"] = game_exit(console)
		log("  %s: the game %s" % (side, result["exit"]))
		result["ok"] = status in (0, 3) and result["exit"] == "exited normally"
		return result
	finally:
		# The clients first, then the game, then gdb: a game whose gdb is gone would run on.
		if client and client.poll() is None:
			kill(client.pid)
		if gdb:
			game_pid = game_pid or next((pid for pid, parent, name, _, _ in processes()
				if parent == gdb.pid and name == "game"), None)
		if game_pid and alive(game_pid):
			kill(game_pid)
		if gdb and gdb.poll() is None:
			kill(gdb.pid)
		if qm and qm.poll() is None:
			qm.terminate()
		for process in (client, gdb, qm):
			if process:
				try:
					process.wait(timeout=10)
				except subprocess.TimeoutExpired:
					kill(process.pid)
					process.wait()
		for _ in range(50):
			if not (game_pid and alive(game_pid)):
				break
			time.sleep(0.1)
		try:
			if remove_shm(options["shm"]):
				log("  %s: removed the SHM segment the game left (key %d)" % (side, options["shm"]))
		except CheckError as e:
			log("  %s: %s" % (side, e))
		shutil.rmtree(game, ignore_errors=True)
		for suffix in ("", "-wal", "-shm", "-journal"):
			if os.path.exists(db + suffix):
				os.remove(db + suffix)


def query_manager_requests(path):
	"""The requests of a query manager log, without times, connection numbers and ports (as compare.sh)."""
	lines = []
	if os.path.isfile(path):
		with open(path, encoding="utf-8", errors="replace") as f:
			for line in f:
				line = line.rstrip("\n").split(" ", 2)[2:]
				line = line[0] if line else ""
				line = re.sub(r"conn [0-9]+:? ?", "", line, count=1)
				line = re.sub(r"from 127\.0\.0\.1:[0-9]+", "", line, count=1)
				line = re.sub(r"database .*", "", line, count=1)
				lines.append(line)
	return lines


def on_signal(number, frame):
	raise KeyboardInterrupt("signal %d" % number)


def trace(arguments, data):
	"""WSL step: runs both builds and compares their traces. Returns {"code": 0 or 1}."""
	options = arguments["options"]
	check_options(options)
	cache = repro_check.cache_dir()
	key = repro_check.key_header(cache)
	root = os.path.join(cache, "trace")
	for number in (signal.SIGTERM, signal.SIGHUP):
		signal.signal(number, on_signal)
	with repro_check.lock(cache, "trace"):
		kill_leftovers(cache, root, options["shm"])
		for port in [options["port"]] + [options["qm_port"] + i for i in range(4)]:
			if listening(port):
				raise CheckError("port %d is in use" % port)
		os.makedirs(root, exist_ok=True)
		results = {}
		for side in ("old", "new"):
			log("trace_check: running %s, %s" % (side, arguments[side]["label"]))
			results[side] = run(side, arguments[side]["dir"], options, key, root)
		import difflib
		sys.path.insert(0, TRACE_TOOLS)
		import compare_traces
		code = 0
		for side in ("old", "new"):
			if not results[side]["ok"]:
				log("trace_check: the %s run did not go through (scenario %s; game %s)" % (side,
					results[side]["scenario"], results[side]["exit"]))
				code = 1
		clients = [os.path.join(root, side, "client") for side in ("old", "new")]
		if not all(os.path.isdir(folder) for folder in clients):
			log("trace_check: no traces to compare")
			return {"code": 1}
		total, summary, report, allowed = compare_traces.compare_dirs(clients[0], clients[1], allow_clock=True)
		with open(os.path.join(root, "diff-client.txt"), "w", encoding="utf-8") as f:
			f.write("\n".join([summary, "allowed differences:"] + ["  " + a for a in allowed] + report) + "\n")
		requests = [query_manager_requests(os.path.join(root, side, "qm.log")) for side in ("old", "new")]
		qm_diff = [line for line in difflib.unified_diff(requests[0], requests[1], lineterm="", n=0)
			if line[:1] in "+-" and not line.startswith(("+++", "---"))]
		with open(os.path.join(root, "diff-qm.txt"), "w", encoding="utf-8") as f:
			f.write("\n".join(qm_diff) + "\n")
		log("trace_check: allowed differences (they follow the wall clock, not the rounds):")
		for line in allowed:
			log("  " + line)
		log("trace_check: query manager requests (for information): %s" % ("identical" if not qm_diff else
			"%d lines differ, see %s" % (len(qm_diff), os.path.join(root, "diff-qm.txt"))))
		if total:
			code = 1
			for line in report[:120]:
				log(line)
			if len(report) > 120:
				log("... %d more lines in %s" % (len(report) - 120, os.path.join(root, "diff-client.txt")))
		log("trace_check: traces %s" % summary)
		return {"code": code}


WORKERS = {"trace": trace}


def main(argv):
	if argv and argv[0] == "--worker":
		return repro_check.worker_main(WORKERS, argv[1:])
	if argv and argv[0] in ("-h", "--help"):
		print(__doc__)
		return 0
	parser = argparse.ArgumentParser(prog="trace_check.py", add_help=False)
	parser.add_argument("commits", nargs="*", help="<old> (default faithful) and <new> (default the working tree)")
	parser.add_argument("--scenario", default="full")
	parser.add_argument("--stop-round", type=int, default=232)
	parser.add_argument("--seed", type=int, default=4242)
	parser.add_argument("--install", default=DEFAULT_INSTALL)
	parser.add_argument("--port", type=int, default=17171)
	parser.add_argument("--qm-port", type=int, default=17173)
	parser.add_argument("--shm", type=int, default=17011)
	args = parser.parse_args(argv)
	if len(args.commits) > 2:
		parser.print_usage(sys.stderr)
		return 2
	options = {"scenario": args.scenario, "stop_round": args.stop_round, "seed": args.seed, "install": args.install,
		"port": args.port, "qm_port": args.qm_port, "shm": args.shm}
	check_options(options)
	who = "trace_check"
	old_tree, old_label = repro_check.resolve(args.commits[0] if args.commits else "faithful")
	new_tree, new_label = repro_check.resolve(args.commits[1] if len(args.commits) > 1 else None)
	old_dir = repro_check.build_tree(old_tree, old_label, "trace", who)
	new_dir = repro_check.build_tree(new_tree, new_label, "trace", who)
	return repro_check.call_worker(__file__, WORKERS, "trace", {"options": options,
		"old": {"dir": old_dir, "label": old_label}, "new": {"dir": new_dir, "label": new_label}})["code"]


if __name__ == "__main__":
	sys.stdout.reconfigure(errors="backslashreplace")
	try:
		sys.exit(main(sys.argv[1:]))
	except CheckError as e:
		print("trace_check.py: %s" % e, file=sys.stderr)
		sys.exit(1)
