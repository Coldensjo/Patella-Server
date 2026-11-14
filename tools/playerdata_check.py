r"""Prove that the player data survives the way through the query manager, for every player file of an installation.

Usage, from any folder (the checkout is the one this file is in), on Windows or in WSL:
  python -I tools/playerdata_check.py [<commit>] [--install PATH] [--qm-port N] [--shm KEY]

<commit> is the tree to build and check; left out, it is the working tree (as in tools/repro_check.py: the files
`git add -A` would commit).

What it does, in WSL Ubuntu-24.04:
  1. Builds the tree the way tools/trace_check.py does (~/.cache/patella-check/build-trace).
  2. Makes a runtime copy of the installation (--install, a tar or a folder; default
     /home/chris/patella-cmp/pristine.tar) in ~/.cache/patella-check/playerdata/game and lays it out for the tree as
     trace_check does (its data/config.toml with a scratch SHM key and scratch query manager ports, --qm-port, default
     17373, and the three after it; its data/items.toml and the other TOML files in the copy's data folder).
  3. Seeds a scratch database from the copy with the tree's querymanager-seed, which imports the characters and their
     player files (the game's old reader, the game's own record), and starts the tree's query manager on it.
  4. Runs the tree's patella-convert usr-check on the copy's player files: each file read with the game's old reader
     must give the same TPlayerData, field by field, as loadPlayerData after the import; the same again after
     savePlayerData and loadPlayerData (but for the murders older than 30 days, which a save drops, as the player
     file's writer did); and it counts the imported files that the old writer gives back byte for byte.

The player files hold real players' data: nothing of them is printed, only counts. The seed's output (it names player
files) stays in a file of the run folder, and the run removes the copy, the database and that file at the end, on
every exit path. A run holds repro_check's ~/.cache/patella-check/run.lock, like the other checks. Ports 7171-7176,
SHM key 10011 and /game are refused.

Exit code 0: every file passed. 1: a difference, a failure, or a step that did not go through. 2: wrong usage.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import time

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import repro_check  # noqa: E402
import trace_check  # noqa: E402
from repro_check import CheckError, log  # noqa: E402

DEFAULT_INSTALL = "/home/chris/patella-cmp/pristine.tar"

# The seed's report lines that hold counts only.
SEED_COUNTS = re.compile(r"^(player files|players imported|players skipped|player data [a-z]+):")


def check(arguments, data):
	"""WSL step: seeds, starts the query manager, runs usr-check. Returns {"code": 0 or 1}."""
	import io
	import tarfile
	options = arguments["options"]
	trace_check.check_options(dict(options, port=options["qm_port"] + 100))
	cache = repro_check.cache_dir()
	root = os.path.join(cache, "playerdata")
	game = os.path.join(root, "game")
	db = os.path.join(root, "querymanager.sqlite")
	seed_log = os.path.join(root, "seed.log")
	binaries = os.path.join(arguments["dir"], "bin")
	qm = None
	with repro_check.lock(cache, "playerdata"):
		for port in [options["qm_port"] + i for i in range(4)]:
			if trace_check.listening(port):
				raise CheckError("port %d is in use" % port)
		try:
			shutil.rmtree(root, ignore_errors=True)
			os.makedirs(root)
			trees = os.path.join(root, "data")
			if data:
				with tarfile.open(fileobj=io.BytesIO(data)) as archive:
					archive.extractall(trees, filter="data")
			folder = os.path.join(trees, "new", "data")
			if not os.path.isfile(os.path.join(folder, "config.toml")):
				raise CheckError("the tree has no data/config.toml")
			if os.path.isdir(options["install"]):
				shutil.copytree(options["install"], game, symlinks=True)
			else:
				os.makedirs(game)
				if subprocess.run(["tar", "-x", "-C", game, "-f", options["install"]], check=False).returncode != 0:
					raise CheckError("cannot unpack %s" % options["install"])
			trace_check.lay_out(game, folder, dict(options, shm=options["shm"]))
			log("playerdata_check: seeding a scratch database from the copy")
			with open(seed_log, "w") as f:
				status = subprocess.run([os.path.join(binaries, "querymanager", "querymanager-seed"), "--db", db,
					"--game-root", game, "--port", str(options["qm_port"] + 100)], stdout=f, stderr=subprocess.STDOUT,
					check=False).returncode
			with open(seed_log, encoding="utf-8", errors="replace") as f:
				for line in f:
					if SEED_COUNTS.match(line):
						log("  " + line.rstrip("\n"))
			if status != 0:
				raise CheckError("querymanager-seed failed (exit %d)" % status)
			qm = subprocess.Popen([os.path.join(binaries, "querymanager", "querymanager"), "--db", db, "--port",
				str(options["qm_port"]), "--log", os.devnull, "--log-level", "error"], stdin=subprocess.DEVNULL,
				stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
			for _ in range(100):
				if trace_check.listening(options["qm_port"]) or qm.poll() is not None:
					break
				time.sleep(0.1)
			if not trace_check.listening(options["qm_port"]):
				raise CheckError("the query manager did not start")
			log("playerdata_check: patella-convert usr-check on every player file")
			start = time.time()
			result = subprocess.run([os.path.join(binaries, "tools", "convert", "patella-convert"), "usr-check",
				os.path.join(game, "config.toml")], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
			# usr-check prints counts only.
			for line in result.stdout.decode("utf-8", "replace").splitlines():
				log("  " + line)
			log("playerdata_check: usr-check took %d s, exit %d" % (time.time() - start, result.returncode))
			return {"code": 0 if result.returncode == 0 else 1}
		finally:
			if qm and qm.poll() is None:
				qm.terminate()
				try:
					qm.wait(timeout=10)
				except subprocess.TimeoutExpired:
					trace_check.kill(qm.pid)
			shutil.rmtree(root, ignore_errors=True)


WORKERS = {"check": check}


def main(argv):
	if argv and argv[0] == "--worker":
		return repro_check.worker_main(WORKERS, argv[1:])
	if argv and argv[0] in ("-h", "--help"):
		print(__doc__)
		return 0
	parser = argparse.ArgumentParser(prog="playerdata_check.py", add_help=False)
	parser.add_argument("commit", nargs="?", help="the tree to check (default: the working tree)")
	parser.add_argument("--install", default=DEFAULT_INSTALL)
	parser.add_argument("--qm-port", type=int, default=17373)
	parser.add_argument("--shm", type=int, default=17311)
	args = parser.parse_args(argv)
	options = {"install": args.install, "qm_port": args.qm_port, "shm": args.shm}
	trace_check.check_options(dict(options, port=args.qm_port + 100))
	who = "playerdata_check"
	tree, label = repro_check.resolve(args.commit)
	with repro_check.run_lock():
		build = repro_check.build_tree(tree, label, "trace", who)
		return repro_check.call_worker(__file__, WORKERS, "check", {"options": options, "dir": build},
			trace_check.data_archive({"new": tree}))["code"]


if __name__ == "__main__":
	sys.stdout.reconfigure(errors="backslashreplace")
	try:
		sys.exit(main(sys.argv[1:]))
	except CheckError as e:
		print("playerdata_check.py: %s" % e, file=sys.stderr)
		sys.exit(1)
