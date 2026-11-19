r"""Prove that a change keeps the game's behaviour: the packet traces and the logs of two builds in a scripted scenario.

Usage, from any folder (the checkout is the one this file is in), on Windows or in WSL:
  python -I tools/trace_check.py [<old> [<new>]] [--scenario NAME] [--stop-round N] [--seed S] [--install PATH]
                                 [--port N] [--qm-port N] [--shm KEY] [--stall-old ROUND:MS]
                                 [--debug-level N] [--logs check|report]

--logs report: the logs are compared and the differences reported, but they do not decide the exit code (for an
<old> whose log texts differ on purpose: faithful writes them in German, main in English since 2026-11-14). Default
check: a difference of the logs fails the check like one of the traces.

--stall-old ROUND:MS makes the old build's main loop wait MS milliseconds at ProcessCommand in round ROUND. A reload
(scenario reload) stalls the new build's main loop for as long as it takes, which shifts the next creature movement
(one longer step instead of several); with the old build stalled as long in the same round, the two runs can be
compared: take ROUND and the time from the new run's log ("Reload ... done in N ms" and the lag line after it).

<old> is a commit, faithful when left out. <new> is a commit too; left out, it is the working tree (as in
tools/repro_check.py: the files `git add -A` would commit).

What it does, in WSL Ubuntu-24.04:
  1. Builds both trees the way tools/repro_check.py does (one fixed source folder, one RSA key for the trees that
     build it in, a cache per tree), but with the normal flags, debug information included, since gdb needs the type
     of RoundNr; in ~/.cache/patella-check/build-trace.
  2. For each build, one after the other and at the same paths: makes a runtime copy of the original installation
     (--install, a tar or a folder; default /home/chris/patella-cmp/pristine.tar) in ~/.cache/patella-check/trace/game
     and lays it out for the build's tree. The TOML files the tree has in data/ replace the original files they were
     converted from: data/config.toml becomes the copy's config.toml and its .tibia is removed (its [paths] must be
     relative, so they point at the copy); data/items.toml, data/circles.toml, data/spells.toml, data/commands.toml,
     data/triggers.toml (data/moveuse.toml before its rename), data/houses.toml, data/map.toml, data/conversion.toml,
     data/npcs.toml, data/monsters.toml, data/raids.toml, data/monster_homes.toml, data/events.toml and data/talkactions.toml go into the
     copy's data folder ([paths] data), as does data/scripts/ (the Lua scripts, as a whole folder), and the files they
     were converted from, objects.srv, circles.dat, moveuse.dat, houseareas.dat, houses.dat, map.dat with the
     mem.dat it includes, conversion.lst, monster.db, and the folders npc/ (the .npc and .ndb files) and mon/ (the
     .mon and .evt files) of the copy, are removed (spells.toml and commands.toml replace the spell table and the
     commands' rights in the code). A tree without them (faithful) gets the copy's .tibia with its paths pointed at the
     copy, and keeps those files. The scenario's clients read the object types from the file the copy has. Either config
     gets a scratch SHM key (--shm, default 17011), scratch query manager ports (--qm-port, default 17173, and the
     three after it for the other entries) and the debug level --debug-level (default 3, one above the original
     installation's 2, so that the game's print(3, ...) lines, which the logs do not hold, are on the console and
     compared in step 6; the level decides only what is printed). Then it seeds a scratch database with the build's
     querymanager-seed (game port --port, default 17171), adds the test accounts with a new character each (900001
     'Testa Alpha', male, and 900002 'Testb Beta', female; and the gamemasters 900003 to 900007, 'Testc Gamma' to 'Testg Eta', male, each with
     some of the rights the commands check, GM_RIGHTS; and the caster 900008 'Testh Theta', male, with the rights to
     cast every spell and make and use every rune, CASTER_RIGHTS; and the door opener 900009 'Testi Iota', male, who
     may open every name door, DOOR_RIGHTS; and the talkers 900010 to 900013, 'Testj Kappa' to 'Testm Nu', male,
     with NO_BANISHMENT and TELEPORT_TO_COORDINATE, TALKER_RIGHTS; none may have a player file or player data) and
     starts the build's query manager. The gamemaster 'Teste Epsilon' also gets the rights RELOAD_SCRIPTS (the command
     Reload Scripts of the scenario reload) and ONLINE_LIST (the command Online List of the scenario online_list),
     each from a tree whose query manager knows it (its querymanager/src/protocol.cc names it; faithful's does not,
     and its seed refuses a right it does not know); the rights from 82 on never reach
     the client, so the other scenarios do not see it. A build whose query manager keeps the player data (its database has the table
     player_data) gets them from the player files with the seed, which imports them; the copy's usr/ is then removed,
     so the run also shows that its game no longer reads or writes player files. A scenario that names houses for test
     characters (its attribute houses, {house id: client key}; tools/trace/scenarios.py) gets them as the owners: after
     the seed, each such house gets an entry in the copy's owners.dat (in its data folder, where the game reads it),
     the character as owner, paid until 2^31-1, no guests and no subowners. Only a house the original owners.dat does
     not list may be given: the seed has imported that file's owners into the database, and the query manager would
     otherwise name the old owner (or evict the house). The game runs with HOME set to the copy, so it reads the
     copy's config.toml (or .tibia). The copy gets the check's RSA key (repro_check's key
     header) as a key file in PKCS#1 PEM, mode 0600, at the path the copy's config.toml names with rsa_key (key.pem in
     the copy without one, and for a .tibia): a tree that reads its key at run time uses it, one that builds the key
     in has the same key built in. The scenario's clients encrypt their logins with that file's modulus.
  3. Runs the game under gdb: rand() is seeded again with --seed (default 4242) right after the game's own
     srand(time(NULL)), at InitSignalHandler; a breakpoint on SetRoundNr prints every round number into the console,
     and the scenario's clients (tools/trace/run_scenario.py; --scenario, default full) act in fixed rounds after it.
     At the start of the AdvanceGame call of round --stop-round (default: the scenario's stop_round in
     tools/trace/scenarios.py if it has one, as commands, else 232, after the last step of full) gdb
     delivers SIGQUIT and the game shuts down by itself. This is the SCENARIO and STOPROUND mode of the reference
     project's notes/boot/compare.sh. One addition: the game runs with a small library preloaded (RAND_SHIM, built
     with gcc for each run) whose rand() is the C library's in the main thread and draws from a sequence of its own
     in any other thread. A connection thread calls rand() when a client connects (the seed of the connection's
     packet padding), at a moment that depends on the threads' timing; with the one sequence of the C library, a
     login during a creature's move gives the creatures other numbers in one run than in the other. The library also
     makes the game's standard output line buffered: each line the game prints is written whole when it is printed,
     between the round numbers gdb writes into the same console file, so that step 6 knows the round of each line
     (with the default full buffering the game wrote 4 KB blocks that gdb's lines cut in the middle of a line).
     The game's CPU time over the scenario is measured on the way, for information (a change that should cost less
     or no more CPU, e.g. a rule engine moved elsewhere): utime + stime of the game process (all its threads), read
     from /proc/<pid>/stat as soon as the console shows the scenario's base round (the first line of
     run_scenario.py's output names it), and printed by gdb (info proc stat, after the line @@CPU AT STOP@@) while
     the game stands at the breakpoint of the stop round, before SIGQUIT; the shutdown is not counted. The two runs
     go one after the other on the same machine, so the numbers show noise of a few per cent; it is reported per
     run and at the end, and does not decide the exit code.
  4. Compares the two traces with tools/trace/compare_traces.py. What follows the wall clock and not the rounds may
     differ and is reported apart: the world light (message 0x82), the "New record: N players are logged in."
     broadcast (it follows the player list, made when the real minute is a multiple of 5) and dates in texts. The
     query manager requests of both runs are compared too, for information (the player list shows up there too).
  5. Compares what each game saved of the test characters when it shut down: the player files a build with
     player files wrote, and the player data a build with a player database saved, written as player files with that
     build's patella-convert usr-export (the game's old writer). LastLogin and LastLogout follow the wall clock and
     are left out. They must be the same, and at least one character must have been saved.
  6. Compares the logs of the two runs: the console and the log files. The game prints with print(level, ...) to its
     standard output (the console file, through gdb) when the level is at most the debug level, error(...) to the
     console and into error.log, and Log(name, ...) writes log/<name>.log (the log folder of the copy: [paths] log, or
     LOGPATH of the .tibia) and prints the line as "<name>.log: <text>" at level 2. The Lua scripts' Game.print,
     debugPrint and Game.log are those three. Each run's log folder is copied to <side>/log before the runtime copy
     is removed; the console stays as <side>/console. Compared are:
     - the console: each line the game printed with the round of gdb's last @@ROUND line before it (0 for the boot),
       without gdb's own lines (round lines, breakpoints, threads, the CPU block, the end) and without the
       "<name>.log: " echoes (the log files are compared instead);
     - each log file either run wrote, line for line: Log's lines with their round but without date and time, the
       start line of InitLog without its time.
     Left out, as they follow the wall clock and not the rounds (CLOCK_LOGS, CLOCK_LINES; the counts are reported):
     lag.log (rounds that took longer than the beat), load.log (the player list of every 5th real minute, with the
     time), netload.log (bytes per real hour); the lines "AdvanceGame: No creature movement due to lag" and "Lag
     detected!"; "Refreshing sector X/Y/Z ..." (the sectors near players refreshed every real minute,
     RefreshCylinders); "Saving all player data..." and the players' "Attaching slot"/"Freeing slot" lines of its
     round (the save ordered every 15 real minutes, done by the writer thread). Replaced by a place holder: the
     game's pid ("Pid N - ...", "(Pid=N)"), socket numbers (file descriptors, which depend on the files the threads
     have open at that moment), the rounds the boot schedules the raids for ("Scheduling raid X for round N" in round
     0: the time until the raid's date or until the next reboot), the reload's "done in N ms" and the number of a
     "New record: N players". Compared without their
     round and order (LOOSE_LINES): "Connection on socket N broke down." (a connection thread, when a client has
     closed its connection, as the clients do at the end of a scenario). The other lines a thread other than the
     game's main thread prints (a connection's login, the writer thread's replies) are compared like the others; a
     run with a different order or round of them shows as a difference.
     The differences are reported per file, the differing lines in ~/.cache/patella-check/trace/diff-logs.txt; with
     --logs check (default) they fail the check like the traces, with --logs report they are for information.

Only the builds run, never the original bin/game. The ports 7171-7176, the SHM key 10011 and /game are refused, so a
server the owner plays on is never touched. Nothing of usr/ is printed: the seed's output and the game's console stay
in files (the log comparison prints lines that differ, and the game names the players of the installation there by
their character ID at most, e.g. the tenants in houses.log). Every exit path stops the processes of the run and removes its SHM segment, database and runtime copy. A run
takes about five minutes per build. What the runs leave (consoles, log folders, query manager logs, traces, the
comparisons) is in ~/.cache/patella-check/trace/. Checks run one at a time: from the first build to the comparison a
run holds repro_check's ~/.cache/patella-check/run.lock, and a repro_check or trace_check from any checkout waits for
it.

Exit code 0: the traces match apart from the allowed differences, the logs match apart from what is left out (or
--logs report), and both runs went through. 1: they differ, or a run or a build failed. 2: wrong usage.
"""

import argparse
import base64
import io
import os
import re
import shutil
import signal
import subprocess
import sys
import tarfile
import time

# No __pycache__ next to the sources.
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import repro_check  # noqa: E402
from repro_check import CheckError, log  # noqa: E402
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "trace"))
import tclient  # noqa: E402

TRACE_TOOLS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "trace")
DEFAULT_INSTALL = "/home/chris/patella-cmp/pristine.tar"
LIVE_PORTS = range(7171, 7177)
LIVE_SHM = 10011
# The rights the commands check (commands.toml), each with a number 1 to 16 whose bits say which of the five
# gamemasters 'Testc Gamma' (bit 0) to 'Testg Eta' (bit 4) has it. Every right is held by one of them at least and no
# two by the same ones, so a command that checks another right than before answers another one of them.
GM_RIGHTS = ["CREATE_OBJECTS", "TELEPORT_VERTICAL", "MODIFY_GOSTRENGTH", "TELEPORT_TO_COORDINATE", "TELEPORT_TO_MARK",
	"CHANGE_SKILLS", "TELEPORT_TO_CHARACTER", "RETRIEVE", "CREATE_MONSTERS", "SHOW_COORDINATE", "HOME_TELEPORT",
	"CREATE_MONEY", "CHANGE_PROFESSION", "KICK", "CLEANUP_FIELDS", "LEVITATE"]


def gm_rights(bit):
	return [right for number, right in enumerate(GM_RIGHTS, 1) if number & (1 << bit)]


# The rights of the caster 'Testh Theta' (scenarios spells and runes): every spell regardless of level, spellbook,
# mana, soul points and premium account, aggressive spells everywhere, no damage taken (the monsters it fights and the
# fields it makes would kill a level 1 character), and the commands it sets itself up with: Get Item (blank runes, a
# staff), Summon Wild Creature and Kill All Creatures (targets, corpses), Move to coordinates, and Change Profession
# (a druid for Paralyze).
CASTER_RIGHTS = ["ALL_SPELLS", "UNLIMITED_MANA", "PREMIUM_ACCOUNT", "ATTACK_EVERYWHERE", "INVULNERABLE",
	"CREATE_OBJECTS", "CREATE_MONSTERS", "TELEPORT_TO_COORDINATE", "CHANGE_PROFESSION"]

# The rights of the door opener 'Testi Iota' (scenario items): it opens a house door as no test character can own a
# house, and goes there by Move to coordinates. The query manager and the game's query client spell the right
# OPEN_NAMEDDOORS (the game's right OPEN_NAMEDOORS), as the original query manager did.
DOOR_RIGHTS = ["OPEN_NAMEDDOORS", "TELEPORT_TO_COORDINATE"]

# The rights of the talkers 'Testj Kappa' to 'Testm Nu' (scenario npcs_busy): each says something every round to an
# NPC, which mutes a player without NO_BANISHMENT after a few lines (TPlayer::RecordTalk: one line in 2.5 seconds, four
# in a row); they go to their NPC by Move to coordinates.
TALKER_RIGHTS = ["NO_BANISHMENT", "TELEPORT_TO_COORDINATE"]

# The rights of our own commands, Reload Scripts (scenario reload) and Online List (scenario online_list), and the
# account that gets each of them from a tree whose query manager knows it (own_rights): the gamemaster
# 'Teste Epsilon', who goes to a field of its own by Move to coordinates to give Reload Scripts.
OWN_RIGHTS = ["RELOAD_SCRIPTS", "ONLINE_LIST"]
OWN_RIGHTS_ACCOUNT = 900005

# Account, password, sex, character name and rights.
TEST_ACCOUNTS = [
	(900001, "testa", 1, "Testa Alpha", []),
	(900002, "testb", 2, "Testb Beta", []),
	(900003, "testc", 1, "Testc Gamma", gm_rights(0)),
	(900004, "testd", 1, "Testd Delta", gm_rights(1)),
	(900005, "teste", 1, "Teste Epsilon", gm_rights(2)),
	(900006, "testf", 1, "Testf Zeta", gm_rights(3)),
	(900007, "testg", 1, "Testg Eta", gm_rights(4)),
	(900008, "testh", 1, "Testh Theta", CASTER_RIGHTS),
	(900009, "testi", 1, "Testi Iota", DOOR_RIGHTS),
	(900010, "testj", 1, "Testj Kappa", TALKER_RIGHTS),
	(900011, "testk", 1, "Testk Lambda", TALKER_RIGHTS),
	(900012, "testl", 1, "Testl Mu", TALKER_RIGHTS),
	(900013, "testm", 1, "Testm Nu", TALKER_RIGHTS),
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

def rewrite_tibia(path, game, shm, qm_port, debug_level):
	"""Points the paths of the runtime copy's .tibia at the copy, its SHM key and query manager ports at the scratch
	ones, and sets its debug level. Returns the world's name."""
	with open(path, encoding="latin-1") as f:
		text = f.read()
	text, count = re.subn(r'(?m)^(\s*[A-Z]+PATH\s*=\s*)"([^"]*)"',
		lambda m: '%s"%s"' % (m.group(1), os.path.join(game, os.path.basename(m.group(2).rstrip("/")))), text)
	if count == 0:
		raise CheckError(".tibia: no paths")
	text, count = re.subn(r"(?m)^(\s*SHM\s*=\s*)\d+", lambda m: "%s%d" % (m.group(1), shm), text)
	if count != 1:
		raise CheckError(".tibia: no SHM key")
	text, count = re.subn(r"(?m)^(\s*DebugLevel\s*=\s*)-?\d+", lambda m: "%s%d" % (m.group(1), debug_level), text)
	if count != 1:
		raise CheckError(".tibia: no DebugLevel")
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


def write_config(source, game, shm, qm_port, debug_level):
	"""Writes the tree's data/config.toml as the copy's config.toml, with the SHM key and the query manager ports set
	to the scratch ones and the debug level set; its paths must be relative, so they point at the copy. Returns the
	world's name."""
	import tomllib
	with open(source, encoding="utf-8") as f:
		text = f.read()
	try:
		config = tomllib.loads(text)
	except tomllib.TOMLDecodeError as e:
		raise CheckError("data/config.toml: %s" % e)
	for key, value in config.get("paths", {}).items():
		if not isinstance(value, str) or value.startswith("/"):
			raise CheckError("data/config.toml: [paths] %s = %r is not a relative path; the runtime copy needs paths "
				"relative to it" % (key, value))
	text, count = re.subn(r"(?m)^(\s*shm_key\s*=\s*)-?\d+", lambda m: "%s%d" % (m.group(1), shm), text)
	if count != 1:
		raise CheckError("data/config.toml: no shm_key")
	text, count = re.subn(r"(?m)^(\s*debug_level\s*=\s*)-?\d+", lambda m: "%s%d" % (m.group(1), debug_level), text)
	if count != 1:
		raise CheckError("data/config.toml: no debug_level")
	entries = [0]

	def port(m):
		entries[0] += 1
		return "%s%d" % (m.group(1), qm_port + entries[0] - 1)

	# port is a key of [[query_manager]] only.
	text = re.sub(r"(?m)^(\s*port\s*=\s*)\d+", port, text)
	if not 1 <= entries[0] <= 4 or entries[0] != len(config.get("query_manager", [])):
		raise CheckError("data/config.toml: %d query_manager entries with a port" % entries[0])
	patched = tomllib.loads(text)
	if patched["shm_key"] != shm or patched["debug_level"] != debug_level or [q["port"] for q in
			patched["query_manager"]] != [qm_port + i for i in range(entries[0])]:
		raise CheckError("data/config.toml: the SHM key, the debug level or the query manager ports could not be set")
	world = patched.get("world", {}).get("name")
	if not world:
		raise CheckError("data/config.toml: no [world] name")
	with open(os.path.join(game, "config.toml"), "w", encoding="utf-8", newline="\n") as f:
		f.write(text)
	return world


def paths_folder(game, key):
	"""A folder of the copy as its config.toml names it ([paths] key, relative to the copy)."""
	import tomllib
	with open(os.path.join(game, "config.toml"), "rb") as f:
		folder = tomllib.load(f).get("paths", {}).get(key)
	if not folder or folder.startswith("/"):
		raise CheckError("data/config.toml: [paths] %s = %r does not name a folder in the copy" % (key, folder))
	return os.path.normpath(os.path.join(game, folder))


def data_folder(game):
	"""The copy's data folder as its config.toml names it ([paths] data, relative to the copy)."""
	return paths_folder(game, "data")


def game_folder(game, key, tibia_key):
	"""A folder of the copy as its game reads it: [paths] key of its config.toml, or else tibia_key of its .tibia
	(which rewrite_tibia pointed at the copy)."""
	if os.path.isfile(os.path.join(game, "config.toml")):
		return paths_folder(game, key)
	with open(os.path.join(game, ".tibia"), encoding="latin-1") as f:
		found = re.search(r'(?m)^\s*%s\s*=\s*"([^"]*)"' % tibia_key, f.read())
	if found is None:
		raise CheckError(".tibia: no %s" % tibia_key)
	return found.group(1)


# The TOML files of a tree's data/ that go into the copy's data folder, each with the original files of that folder it
# was converted from and replaces (none: one the game had in its code, or new data such as events.toml).
DATA_FILES = [
	("items.toml", ("objects.srv",)),
	("circles.toml", ("circles.dat",)),
	("spells.toml", ()),
	("commands.toml", ()),
	("moveuse.toml", ("moveuse.dat",)),
	("triggers.toml", ("moveuse.dat",)),
	("houses.toml", ("houseareas.dat", "houses.dat")),
	("map.toml", ("map.dat", "mem.dat")),
	("conversion.toml", ("conversion.lst",)),
	("npcs.toml", ()),
	("monsters.toml", ()),
	("raids.toml", ()),
	("monster_homes.toml", ("monster.db",)),
	("events.toml", ()),
	("talkactions.toml", ()),
]

# The TOML files of a tree's data/ that replace a whole folder of the copy, the folder as the original installation
# lays it out (its .tibia's NPCPATH and MONSTERPATH; config.toml no longer names them).
DATA_FOLDERS = {
	"npcs.toml": "npc",
	"monsters.toml": "mon",
}


def lay_out(game, data, options):
	"""Lays the runtime copy out for a tree whose data/ folder is data (None: the tree has none). Returns the world's
	name, the layout for the log and the object types file for the clients."""
	tibia = os.path.join(game, ".tibia")
	objects = os.path.join(game, "dat", "objects.srv")
	if data and os.path.isfile(os.path.join(data, "config.toml")):
		world = write_config(os.path.join(data, "config.toml"), game, options["shm"], options["qm_port"],
			options["debug_level"])
		if os.path.exists(tibia):
			os.remove(tibia)
		layout = "config.toml from the tree's data/, no .tibia"
		for name, originals in DATA_FILES:
			if not os.path.isfile(os.path.join(data, name)):
				continue
			folder = data_folder(game)
			shutil.copyfile(os.path.join(data, name), os.path.join(folder, name))
			layout += "; %s from the tree's data/" % name
			for original in originals:
				if os.path.exists(os.path.join(folder, original)):
					os.remove(os.path.join(folder, original))
				layout += ", no %s" % original
			if name in DATA_FOLDERS:
				replaced = os.path.join(game, DATA_FOLDERS[name])
				if os.path.exists(replaced):
					shutil.rmtree(replaced)
				layout += ", no %s/" % DATA_FOLDERS[name]
			if name == "items.toml":
				objects = os.path.join(folder, name)
		if os.path.isdir(os.path.join(data, "scripts")):
			folder = data_folder(game)
			if os.path.exists(os.path.join(folder, "scripts")):
				shutil.rmtree(os.path.join(folder, "scripts"))
			shutil.copytree(os.path.join(data, "scripts"), os.path.join(folder, "scripts"))
			layout += "; scripts/ from the tree's data/"
		return world, layout, objects
	for name, _ in DATA_FILES:
		if data and os.path.isfile(os.path.join(data, name)):
			raise CheckError("data/%s without data/config.toml: the copy's data folder is unknown" % name)
	if data and os.path.isdir(os.path.join(data, "scripts")):
		raise CheckError("data/scripts without data/config.toml: the copy's data folder is unknown")
	return rewrite_tibia(tibia, game, options["shm"], options["qm_port"], options["debug_level"]), ".tibia", objects


# The seed's report lines that hold counts only.
SEED_COUNTS = re.compile(r"^(player files|players imported|players skipped|player data [a-z]+):")


def has_player_data(db):
	"""Whether the database keeps the player data (the table player_data of schema version 2)."""
	import sqlite3
	connection = sqlite3.connect(db)
	try:
		return connection.execute("SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'player_data'"
			).fetchone() is not None
	finally:
		connection.close()


def stored_player_data(db, character):
	"""Whether the database has player data of the character."""
	import sqlite3
	connection = sqlite3.connect(db)
	try:
		return connection.execute("SELECT 1 FROM player_data WHERE character_id = ?", (character,)).fetchone() \
			is not None
	finally:
		connection.close()


def seed(binary, db, game, world, port, log_file, own_rights):
	"""Seeds the database from the copy and adds the test accounts, as compare.sh does, OWN_RIGHTS_ACCOUNT with the
	rights own_rights (those of OWN_RIGHTS the tree's query manager knows) too. The output (it names player files) goes to log_file only; the lines with the
	counts of the player files and player data are logged. Returns the IDs of the new characters, one per test
	account."""
	with open(log_file, "w") as f:
		if subprocess.run([binary, "--db", db, "--game-root", game, "--port", str(port)], stdout=f,
				stderr=subprocess.STDOUT, check=False).returncode != 0:
			raise CheckError("querymanager-seed --game-root failed; see %s" % log_file)
	with open(log_file, encoding="utf-8", errors="replace") as f:
		for line in f:
			if SEED_COUNTS.match(line):
				log("    seed: " + line.rstrip("\n"))
	characters = []
	for account, password, sex, name, rights in TEST_ACCOUNTS:
		if account == OWN_RIGHTS_ACCOUNT:
			rights = rights + own_rights
		result = subprocess.run([binary, "--db", db, "--add-account", str(account), "--password", password, "--world",
			world, "--sex", str(sex), "--new-character", name] + [a for right in rights for a in ("--right", right)],
			stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
		output = result.stdout.decode("utf-8", "replace")
		with open(log_file, "a") as f:
			f.write(output)
		ids = [int(i) for i in re.findall(r"(?m)^new character (\d+):", output)]
		if result.returncode != 0 or len(ids) != 1:
			raise CheckError("querymanager-seed --add-account %d failed; see %s" % (account, log_file))
		# A new character starts as a new one only without a player file and without player data; only their
		# existence is tested.
		if os.path.exists(os.path.join(game, "usr", "%02d" % (ids[0] % 100), "%d.usr" % ids[0])):
			raise CheckError("new character %d has a player file" % ids[0])
		if has_player_data(db) and stored_player_data(db, ids[0]):
			raise CheckError("new character %d has player data" % ids[0])
		characters.append(ids[0])
	return characters


def give_houses(game, houses, characters):
	"""Makes test characters the owners of houses for the run (step 2 of the comment at the top): houses is {house id
	(as text, from JSON): client key} (the scenario's attribute houses), characters the IDs of the new characters, one
	per TEST_ACCOUNTS entry. Appends an entry per house to the copy's owners.dat. Returns what it did, for the log."""
	import run_scenario
	path = os.path.join(game_folder(game, "data", "DATAPATH"), "owners.dat")
	text = ""
	if os.path.isfile(path):
		with open(path, encoding="latin-1") as f:
			text = f.read()
	listed = {int(n) for n in re.findall(r"(?m)^\s*ID\s*=\s*(\d+)", text)}
	accounts = [entry[0] for entry in TEST_ACCOUNTS]
	given = []
	for house, key in sorted((int(house), key) for house, key in houses.items()):
		if house in listed:
			raise CheckError("house %d: the original owners.dat lists it; a scenario may only be given a house it does "
				"not list" % house)
		account, _, name = run_scenario.ACCOUNTS[key]
		text += "\nID = %d\nOwner = %d\nLastTransition = 0\nPaidUntil = 2147483647\nGuests = {}\nSubowners = {}\n" % (
			house, characters[accounts.index(account)])
		given.append("house %d to %s" % (house, name))
	with open(path, "w", encoding="latin-1", newline="\n") as f:
		f.write(text)
	return ", ".join(given)


def saved_characters(binaries, game, db, characters, out):
	"""What the game saved of the test characters, as player file texts without the LastLogin and LastLogout lines
	({id: text}; a character it saved nothing of is missing): the player files it wrote, or, with a player database,
	the player data written as player files by the build's patella-convert usr-export."""
	folder = os.path.join(game, "usr")
	if has_player_data(db):
		folder = os.path.join(out, "saved")
		shutil.rmtree(folder, ignore_errors=True)
		with open(os.path.join(out, "export.log"), "w") as f:
			if subprocess.run([os.path.join(binaries, "tools", "convert", "patella-convert"), "usr-export",
					os.path.join(game, "config.toml"), folder] + [str(c) for c in characters], stdout=f,
					stderr=subprocess.STDOUT, check=False).returncode != 0:
				raise CheckError("patella-convert usr-export failed; see %s" % os.path.join(out, "export.log"))
	texts = {}
	for character in characters:
		path = os.path.join(folder, "%02d" % (character % 100), "%d.usr" % character)
		if os.path.isfile(path):
			with open(path, encoding="latin-1") as f:
				texts[character] = "".join(line for line in f if not line.startswith(("LastLogin", "LastLogout")))
	return texts


# The library the game runs with (LD_PRELOAD, see the comment at the top, step 3): rand() of the main thread is the
# C library's, that of any other thread draws from a sequence of its own; patella_stall is the wait of --stall-old;
# the standard output is line buffered.
RAND_SHIM = r"""
#define _GNU_SOURCE
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>

/* Each line the game prints is written whole when it is printed (the log comparison gives it the round of the
   console's last round line). */
__attribute__((constructor)) static void patella_line_buffered(void) {
	setvbuf(stdout, NULL, _IOLBF, 0);
}

/* Waits ms milliseconds of the monotonic clock, signals delivered meanwhile (--stall-old). */
void patella_stall(int ms) {
	struct timespec start, now, step = {0, 1000000};
	clock_gettime(CLOCK_MONOTONIC, &start);
	do {
		nanosleep(&step, NULL);
		clock_gettime(CLOCK_MONOTONIC, &now);
	} while ((now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000 < ms);
}

static unsigned int OtherSeed = 1;
static pthread_mutex_t OtherLock = PTHREAD_MUTEX_INITIALIZER;

int rand(void) {
	static int (*Real)(void);
	if ((pid_t)syscall(SYS_gettid) != getpid()) {
		pthread_mutex_lock(&OtherLock);
		int Value = rand_r(&OtherSeed);
		pthread_mutex_unlock(&OtherLock);
		return Value;
	}
	if (Real == NULL) {
		Real = (int (*)(void))dlsym(RTLD_NEXT, "rand");
	}
	return Real();
}
"""


def build_rand_shim(folder):
	"""Compiles RAND_SHIM into folder/randshim.so; returns its path."""
	source = os.path.join(folder, "randshim.c")
	library = os.path.join(folder, "randshim.so")
	with open(source, "w") as f:
		f.write(RAND_SHIM)
	if subprocess.run(["gcc", "-O2", "-shared", "-fPIC", "-o", library, source, "-ldl", "-lpthread"],
			check=False).returncode != 0:
		raise CheckError("cannot build %s" % library)
	return library


# What the gdb script prints before and after the game's /proc stat (gdb's info proc stat) at the stop round.
CPU_STOP_MARK = "@@CPU AT STOP@@"
CPU_STOP_END = "@@CPU END@@"


def game_cpu(pid):
	"""The CPU time the game process has used so far, utime + stime of /proc/<pid>/stat (all its threads), in
	seconds; None if it cannot be read."""
	try:
		with open("/proc/%d/stat" % pid) as f:
			fields = f.read().rsplit(")", 1)[1].split()
	except (OSError, IndexError):
		return None
	# After the name: state is field 3, utime field 14 and stime field 15.
	return (int(fields[11]) + int(fields[12])) / os.sysconf("SC_CLK_TCK")


def game_cpu_at_stop(console):
	"""utime + stime of the game in seconds as gdb printed them (info proc stat) when the game stood at the stop
	round, before SIGQUIT; None if the console has no such lines."""
	with open(console, "rb") as f:
		f.seek(max(0, os.path.getsize(console) - 4 * 1048576))
		tail = f.read().decode("utf-8", "replace")
	at = tail.rfind(CPU_STOP_MARK)
	if at < 0:
		return None
	times = {}
	for name, value in re.findall(r"(?m)^(utime|stime): (\d+)$", tail[at:]):
		times.setdefault(name, int(value))
	if len(times) != 2:
		return None
	return (times["utime"] + times["stime"]) / os.sysconf("SC_CLK_TCK")


def gdb_script(seed_value, stop_round, rand_shim, stall=None):
	"""The gdb commands of compare.sh's SCENARIO and STOPROUND mode (breakpoint 2 is the one of the stop round), the
	game run with rand_shim (build_rand_shim) preloaded."""
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
		# The game's CPU time at the stop round, while it is stopped here (game_cpu_at_stop).
		'printf "%s\\n"' % CPU_STOP_MARK,
		"info proc stat",
		'printf "%s\\n"' % CPU_STOP_END,
		"signal SIGQUIT",
		"end",
		"break SetRoundNr",
		"commands",
		"silent",
		'printf "@@ROUND %u@@\\n", RoundNr',
		"continue",
		"end",
	] + ([
		# --stall-old: the main loop waits MS milliseconds in round ROUND at ProcessCommand, where the other
		# build's AdvanceGame runs a reload (ProcessScriptReload), as long as that reload took.
		"break ProcessCommand if RoundNr == %d" % stall[0],
		"commands",
		"silent",
		"call (void) patella_stall(%d)" % stall[1],
		"continue",
		"end",
	] if stall else []) + [
		"set environment LD_PRELOAD %s" % rand_shim,
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


def der_length(length):
	if length < 0x80:
		return bytes([length])
	data = length.to_bytes((length.bit_length() + 7) // 8, "big")
	return bytes([0x80 | len(data)]) + data


def der_integer(value):
	data = value.to_bytes(value.bit_length() // 8 + 1, "big")
	return b"\x02" + der_length(len(data)) + data


def write_key_file(header, path):
	"""Writes the key of an RSA_PrivateKey.hh as a key file: PKCS#1 PEM (n, e, d, p, q, d mod p-1, d mod q-1, q^-1 mod
	p), mode 0600. Returns the path."""
	p, q = tclient.rsa_primes(header)
	e = tclient.RSA_E
	d = pow(e, -1, (p - 1) * (q - 1))
	content = b"".join(der_integer(v) for v in (0, p * q, e, d, p, q, d % (p - 1), d % (q - 1), pow(q, -1, p)))
	text = base64.b64encode(b"\x30" + der_length(len(content)) + content).decode("ascii")
	lines = ["-----BEGIN RSA PRIVATE KEY-----"] + [text[i:i + 64] for i in range(0, len(text), 64)] + [
		"-----END RSA PRIVATE KEY-----"]
	os.makedirs(os.path.dirname(path), exist_ok=True)
	if os.path.lexists(path):
		os.remove(path)
	descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
	with os.fdopen(descriptor, "w", encoding="ascii", newline="\n") as f:
		f.write("\n".join(lines) + "\n")
	return path


def key_file_path(game):
	"""Where the copy's game looks for its key file: rsa_key of its config.toml, relative to the copy, or key.pem."""
	config = os.path.join(game, "config.toml")
	name = "key.pem"
	if os.path.isfile(config):
		import tomllib
		with open(config, "rb") as f:
			name = tomllib.load(f).get("rsa_key", name)
		if not isinstance(name, str) or not name or os.path.isabs(name):
			raise CheckError("data/config.toml: rsa_key = %r is not a relative path; the runtime copy needs one" % name)
	# The key file is written there (an existing file removed first), so it must lie in the copy, also through
	# ".." and symbolic links of the installation.
	root = os.path.realpath(game)
	path = os.path.realpath(os.path.join(game, name))
	if path == root or os.path.commonpath([root, path]) != root:
		raise CheckError("data/config.toml: rsa_key = %r points out of the runtime copy" % name)
	return path


def run(side, build, data, options, key, root, own_rights):
	"""One run of a build, with data the tree's data/ folder (None: none); own_rights: the rights of OWN_RIGHTS its
	query manager knows. Returns its result: {"ok", "boot", "scenario", "exit"}."""
	out = os.path.join(root, side)
	game = os.path.join(root, "game")
	db = os.path.join(root, "querymanager.sqlite")
	shutil.rmtree(out, ignore_errors=True)
	os.makedirs(out)
	binaries = os.path.join(build, "bin")
	result = {"ok": False, "boot": None, "scenario": None, "exit": None, "saved": None, "cpu": None}
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
		world, layout, objects = lay_out(game, data, options)
		key_file = write_key_file(key, key_file_path(game))
		log("  %s: runtime copy with %s; the check's RSA key in %s" % (side, layout,
			os.path.relpath(key_file, os.path.realpath(game))))
		characters = seed(os.path.join(binaries, "querymanager", "querymanager-seed"), db, game, world,
			options["port"], os.path.join(out, "seed.log"), own_rights)
		for right in own_rights:
			log("  %s: the query manager knows the right %s: account %d has it" % (side, right, OWN_RIGHTS_ACCOUNT))
		if options["houses"]:
			log("  %s: owners.dat of the copy gives %s" % (side, give_houses(game, options["houses"], characters)))
		if has_player_data(db):
			shutil.rmtree(os.path.join(game, "usr"))
			log("  %s: the database keeps the player data; usr/ removed from the copy" % side)
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
			stall = None
			if side == "old" and options.get("stall_old"):
				stall = [int(v) for v in options["stall_old"].split(":")]
				log("  %s: a stall of %d ms at ProcessCommand of round %d (--stall-old)" % (side, stall[1], stall[0]))
			f.write(gdb_script(options["seed"], options["stop_round"], build_rand_shim(out), stall))
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
				options["scenario"], "--out", os.path.join(out, "client"), "--objects", objects, "--console", console,
				"--port", str(options["port"]), "--rsa-key", key_file],
				stdin=subprocess.DEVNULL, stdout=f, stderr=subprocess.STDOUT, start_new_session=True)
		# The game's CPU time when the scenario starts: at the first look after the console shows the scenario's
		# base round (run_scenario.py prints it first).
		clock = tclient.RoundClock(console)
		base = cpu_start = None
		while client.poll() is None:
			if base is None:
				with open(os.path.join(out, "client.log"), encoding="utf-8", errors="replace") as f:
					found = re.search(r"base round (\d+)", f.read())
				base = int(found.group(1)) if found else None
			if base is not None and cpu_start is None and clock.now() >= base:
				cpu_start = game_cpu(game_pid)
			time.sleep(0.02)
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
		cpu_stop = game_cpu_at_stop(console)
		if cpu_start is not None and cpu_stop is not None:
			result["cpu"] = cpu_stop - cpu_start
			log("  %s: game CPU over the scenario (round %d to the stop round %d): %.2f s" % (side, base,
				options["stop_round"], result["cpu"]))
		else:
			log("  %s: game CPU over the scenario: not measured (%s)" % (side, "no reading at the start round"
				if cpu_start is None else "gdb printed no CPU time at the stop round"))
		result["ok"] = status in (0, 3) and result["exit"] == "exited normally"
		if result["exit"] == "exited normally":
			result["saved"] = saved_characters(binaries, game, db, characters, out)
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
		# The game's log files stay with the run's other output (step 6 compares them).
		try:
			logs = game_folder(game, "log", "LOGPATH")
			if os.path.isdir(logs):
				shutil.copytree(logs, os.path.join(out, "log"))
		except (CheckError, OSError) as e:
			log("  %s: the log files were not kept: %s" % (side, e))
		shutil.rmtree(game, ignore_errors=True)
		for suffix in ("", "-wal", "-shm", "-journal"):
			if os.path.exists(db + suffix):
				os.remove(db + suffix)


# ---------------------------------------------------------------- the logs (step 6 of the comment at the top)

# The time stamp Log writes in front of a line of a log file (writer.cc): the date and time, then the round.
LOG_TIME = re.compile(r"^\d\d\.\d\d\.\d{4} \d\d:\d\d:\d\d \((-?\d+)\): ")
# The line InitLog writes when the game starts a log file: its name and the start time (ctime); faithful writes
# "gestartet".
LOG_STARTED = re.compile(r"^(\S+\.log - (?:started|gestartet) ).*$")

# The log files whose lines all follow the wall clock, not the rounds; they are left out (their line counts are
# reported).
CLOCK_LOGS = {
	"lag.log": "a round that took longer than the beat (AdvanceGame: Delay N msec)",
	"load.log": "the time and the first free player slot, with the player list, when the real minute is a multiple "
		"of 5 (CreatePlayerList)",
	"netload.log": "the bytes sent and received since the last full real hour, at every full real hour "
		"(NetLoadSummary)",
}

# What follows the wall clock or the harness, not the rounds, in the lines of the console and of the log files (their
# text, without time stamp and round): a pattern, what replaces each match (None: the line is left out), why, and
# whether only lines of the boot (round 0) are meant. Each of them is applied to every line; main's texts and
# faithful's German ones both.
CLOCK_LINES = [
	# The game's process ID ("Pid N - listening on port N", "LaunchGame: Game server is ready (Pid=N).").
	(re.compile(r"^Pid \d+ "), "Pid <pid> ", "the game's process ID", False),
	(re.compile(r"\(Pid=\d+\)"), "(Pid=<pid>)", "the game's process ID", False),
	# A connection's socket, a file descriptor: its number depends on the files the game's threads have open at
	# that moment (the protocol thread opens a log file for every line it writes).
	(re.compile(r"\b([Ss]ocket )\d+"), r"\1<fd>", "a socket's file descriptor", False),
	# The rounds the boot schedules raids for: the round now plus the seconds until the raid's date, or a random
	# round before the next reboot (LoadMonsterRaids). Raids started later (Start Monsterraid) keep their round.
	(re.compile(r"^(Scheduling raid .* for round |Plane Raid .* ein f\xfcr Runde )-?\d+\.$"), r"\1<round>.",
		"the round the boot schedules a raid for (the time until its date or the next reboot)", True),
	# The lag lines: a round that took too long (main.cc AdvanceGame) and the load check (NetLoadCheck).
	(re.compile(r"^AdvanceGame: (No creature movement due to lag|Keine Kreaturbewegung wegen Lag)"), None,
		"a round that took too long", False),
	(re.compile(r"^Lag detected!$"), None, "the load check of the real minute", False),
	# The sectors refreshed from the original map near the players (RefreshCylinders, every real minute).
	(re.compile(r"^(Refreshing sector|Refreshe Sektor) \d+/\d+/\d+ \.\.\.$"), None, "the refresh of sectors every "
		"real minute", False),
	# The reload's summary (reload.cc): how long the reload took.
	(re.compile(r"\bdone in \d+ ms"), "done in <N> ms", "how long the reload took", False),
	# The new record of players online, found with the player list of every 5th real minute.
	(re.compile(r"\bNew record: \d+ players"), "New record: <N> players", "the player list of the real minute",
		False),
]


# The save of all player data, ordered every 15 real minutes (AdvanceGame, SavePlayerDataOrder) and done by the writer
# thread (SavePlayerPoolSlots): its first line, and the lines of each player it saves, which it prints in the same
# round; those of that round are left out with it.
SAVE_START = re.compile(r"^(Saving all player data|Speichere alle Spielerdaten)\.\.\.$")
SAVE_SLOT = re.compile(r"^(Attaching slot of player \d+\.|Freeing slot of player \d+\.|Attache Slot von Spieler \d+\.|"
	r"Gebe Slot von Spieler \d+ frei\.)$")
SAVE_REASON = "the save of all player data every 15 real minutes (writer thread)"

# The lines of the log files a connection thread writes at a moment that depends on the threads' timing, not on the
# rounds: they are compared without their round and order (loose).
LOOSE_LINES = [
	# A client that closed its connection (the clients at the end of a scenario) when the game sends to it.
	re.compile(r"^(Connection on socket <fd> broke down\.|Verbindung an Socket <fd> zusammengebrochen\.)$"),
]


def clock_filter(text, boot):
	"""(text, reasons): the text with what follows the wall clock or the harness replaced, None if it is left out
	as a whole; boot: whether the line is of the boot (round 0)."""
	reasons = []
	for pattern, replacement, reason, boot_only in CLOCK_LINES:
		if (boot or not boot_only) and pattern.search(text):
			reasons.append(reason)
			if replacement is None:
				return None, reasons
			text = pattern.sub(replacement, text)
	return text, reasons


def count_reasons(filtered, reasons):
	for reason in reasons:
		filtered[reason] = filtered.get(reason, 0) + 1


def log_file_lines(path):
	"""The lines of a log file as compared: without the date and time of Log's time stamp (the round stays) and the
	start time of InitLog's line, through clock_filter. Returns (lines, {reason: count}, the LOOSE_LINES texts)."""
	lines = []
	filtered = {}
	loose = []
	with open(path, "rb") as f:
		raw = f.read().decode("latin-1").split("\n")
	if raw[-1] == "":
		raw.pop()
	for line in raw:
		line = LOG_STARTED.sub(r"\1<time>", line)
		stamp = LOG_TIME.match(line)
		text, reasons = clock_filter(line[stamp.end():] if stamp else line, stamp is not None and
			int(stamp.group(1)) == 0)
		count_reasons(filtered, reasons)
		if text is None:
			continue
		if any(pattern.match(text) for pattern in LOOSE_LINES):
			loose.append(text)
		else:
			lines.append("(%s): %s" % (stamp.group(1), text) if stamp else text)
	return lines, filtered, loose


# The lines gdb writes into the console: the round numbers (they give the following game lines their round), the
# breakpoints, threads and the end of the game, and the CPU block (CPU_STOP_MARK to CPU_STOP_END).
GDB_ROUND = re.compile(r"^@@ROUND (\d+)@@$")
GDB_LINE = re.compile(r"^(Breakpoint \d+ at 0x[0-9a-f]+: file .*|\[Thread debugging using libthread_db enabled\]|"
	r"Using host libthread_db library .*|\[New Thread 0x[0-9a-f]+ \(LWP \d+\)\]|"
	r"\[Thread 0x[0-9a-f]+ \(LWP \d+\) exited\]|\[Inferior \d+ \(process \d+\) [^\]]*\])$")
# The console lines of Log (print(2, "<name>.log: <text>")): the log files hold them, and are compared.
CONSOLE_LOG_ECHO = re.compile(r"^[a-z-]+\.log: ")


def console_lines(path):
	"""What the game printed on the console, each line with the round gdb's last round line before it named (0 before
	the first): ["<round>: <text>"], without gdb's lines, without the echoes of the log files' lines, through
	clock_filter. Returns (lines, {reason: count})."""
	lines = []
	filtered = {}
	current = 0
	save_round = None
	inside_cpu = False
	with open(path, "rb") as f:
		raw = f.read().decode("latin-1").split("\n")
	if raw[-1] == "":
		raw.pop()
	for line in raw:
		if inside_cpu:
			inside_cpu = line != CPU_STOP_END
			continue
		if line == CPU_STOP_MARK:
			inside_cpu = True
			continue
		found = GDB_ROUND.match(line)
		if found:
			current = int(found.group(1))
			continue
		if GDB_LINE.match(line) or CONSOLE_LOG_ECHO.match(line):
			continue
		if SAVE_START.match(line):
			save_round = current
		if save_round == current and (SAVE_START.match(line) or SAVE_SLOT.match(line)):
			count_reasons(filtered, [SAVE_REASON])
			continue
		text, reasons = clock_filter(line, current == 0)
		count_reasons(filtered, reasons)
		if text is not None:
			lines.append("%d: %s" % (current, text))
	return lines, filtered, []


def compare_logs(root):
	"""Compares the console and the log files of the runs old and new (in root/<side>/console and root/<side>/log/).
	Returns (whether they differ, summary lines, the differing lines)."""
	import difflib
	sources = [("console", [os.path.join(root, side, "console") for side in ("old", "new")])]
	names = set()
	for side in ("old", "new"):
		folder = os.path.join(root, side, "log")
		if os.path.isdir(folder):
			names.update(n for n in os.listdir(folder) if n.endswith(".log"))
	sources += [(name, [os.path.join(root, side, "log", name) for side in ("old", "new")]) for name in sorted(names)]
	differs = False
	summary = []
	report = []
	for name, paths in sources:
		if name in CLOCK_LOGS:
			counts = []
			for path in paths:
				with open(path, "rb") if os.path.isfile(path) else io.BytesIO() as f:
					counts.append(f.read().count(b"\n"))
			summary.append("%s: left out, it follows the wall clock (%s): old %d, new %d lines" % (name,
				CLOCK_LOGS[name], counts[0], counts[1]))
			continue
		read = console_lines if name == "console" else log_file_lines
		results = [read(path) if os.path.isfile(path) else None for path in paths]
		if None in results:
			differs = True
			summary.append("%s: only the %s run wrote it" % (name, "new" if results[0] is None else "old"))
			report.append("=== %s: only the %s run wrote it" % (name, "new" if results[0] is None else "old"))
			continue
		(old, old_filtered, old_loose), (new, new_filtered, new_loose) = results
		filtered = "; ".join("%s: old %d, new %d" % (reason, old_filtered.get(reason, 0), new_filtered.get(reason, 0))
			for reason in sorted(set(old_filtered) | set(new_filtered)))
		diff = [line for line in difflib.unified_diff(old, new, lineterm="", n=0)
			if not line.startswith(("+++", "---"))]
		loose = [line for line in difflib.unified_diff(sorted(old_loose), sorted(new_loose), lineterm="", n=0)
			if line[:1] in "+-" and not line.startswith(("+++", "---"))]
		if loose:
			diff += ["@@ lines of other threads, compared without round and order"] + loose
		if old_loose or new_loose:
			filtered += "%sother threads' lines without round and order: old %d, new %d" % ("; " if filtered else "",
				len(old_loose), len(new_loose))
		changed = sum(1 for line in diff if line[:1] in "+-")
		if changed:
			differs = True
			summary.append("%s: %d lines differ (old %d, new %d lines)%s" % (name, changed, len(old), len(new),
				"; filtered: " + filtered if filtered else ""))
			report.append("=== %s" % name)
			report += diff
		else:
			summary.append("%s: the same (%d lines)%s" % (name, len(old), "; filtered: " + filtered if filtered else
				""))
	return differs, summary, report


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
		trees = os.path.join(root, "data")
		shutil.rmtree(trees, ignore_errors=True)
		os.makedirs(trees)
		if data:
			with tarfile.open(fileobj=io.BytesIO(data)) as archive:
				archive.extractall(trees, filter="data")
		results = {}
		for side in ("old", "new"):
			log("trace_check: running %s, %s" % (side, arguments[side]["label"]))
			folder = os.path.join(trees, side, "data")
			results[side] = run(side, arguments[side]["dir"], folder if os.path.isdir(folder) else None, options, key,
				root, arguments[side]["own_rights"])
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
		log("trace_check: game CPU over the scenario (for information): %s" % ", ".join("%s %s" % (side,
			"%.2f s" % results[side]["cpu"] if results[side]["cpu"] is not None else "not measured")
			for side in ("old", "new")))
		log("trace_check: query manager requests (for information): %s" % ("identical" if not qm_diff else
			"%d lines differ, see %s" % (len(qm_diff), os.path.join(root, "diff-qm.txt"))))
		saved = [results[side]["saved"] for side in ("old", "new")]
		if None in saved:
			log("trace_check: saved test characters: not compared (a game did not exit normally)")
			code = 1
		else:
			old_saved = [saved[0].get(c) for c in sorted(saved[0])]
			new_saved = [saved[1].get(c) for c in sorted(saved[1])]
			if not saved[0] or sorted(saved[0]) != sorted(saved[1]) or old_saved != new_saved:
				code = 1
				log("trace_check: saved test characters differ (old saved %d, new %d):" % (len(saved[0]),
					len(saved[1])))
				for character in sorted(set(saved[0]) | set(saved[1])):
					for line in difflib.unified_diff(saved[0].get(character, "").splitlines(),
							saved[1].get(character, "").splitlines(), "old %d" % character, "new %d" % character,
							lineterm="", n=0):
						log("  " + line)
			else:
				log("trace_check: saved test characters: the same in both runs (%d characters, LastLogin and "
					"LastLogout left out)" % len(saved[0]))
		logs_differ, log_summary, log_report = compare_logs(root)
		with open(os.path.join(root, "diff-logs.txt"), "w", encoding="utf-8") as f:
			f.write("\n".join(log_summary + [""] + log_report) + "\n")
		log("trace_check: logs, the console and the log files (%s):" % ("they decide the exit code"
			if options["logs"] == "check" else "for information, --logs report"))
		for line in log_summary:
			log("  " + line)
		if logs_differ:
			if options["logs"] == "check":
				code = 1
			for line in log_report[:120]:
				log("  " + line)
			if len(log_report) > 120:
				log("  ... %d more lines in %s" % (len(log_report) - 120, os.path.join(root, "diff-logs.txt")))
		if total:
			code = 1
			for line in report[:120]:
				log(line)
			if len(report) > 120:
				log("... %d more lines in %s" % (len(report) - 120, os.path.join(root, "diff-client.txt")))
		log("trace_check: traces %s" % summary)
		return {"code": code}


WORKERS = {"trace": trace}


def known_own_rights(tree):
	"""The rights of OWN_RIGHTS the tree's query manager knows: those its querymanager/src/protocol.cc names."""
	try:
		source = repro_check.git("show", "%s:querymanager/src/protocol.cc" % tree)
	except CheckError:
		return []
	return [right for right in OWN_RIGHTS if ('"%s"' % right) in source]


def data_archive(trees):
	"""A tar of the data/ folders of the trees ({side: tree}), as <side>/data/...; a tree without one adds nothing."""
	buffer = io.BytesIO()
	with tarfile.open(fileobj=buffer, mode="w") as out:
		for side, tree in trees.items():
			if not repro_check.git("ls-tree", "--name-only", tree, "data"):
				continue
			with tarfile.open(fileobj=io.BytesIO(repro_check.git("archive", "--format=tar", tree, "data",
					binary=True))) as source:
				for member in source.getmembers():
					content = source.extractfile(member) if member.isfile() else None
					member.name = side + "/" + member.name
					out.addfile(member, content)
	return buffer.getvalue()


def main(argv):
	if argv and argv[0] == "--worker":
		return repro_check.worker_main(WORKERS, argv[1:])
	if argv and argv[0] in ("-h", "--help"):
		print(__doc__)
		return 0
	parser = argparse.ArgumentParser(prog="trace_check.py", add_help=False)
	parser.add_argument("commits", nargs="*", help="<old> (default faithful) and <new> (default the working tree)")
	parser.add_argument("--scenario", default="full")
	parser.add_argument("--stop-round", type=int, default=None)
	parser.add_argument("--seed", type=int, default=4242)
	parser.add_argument("--install", default=DEFAULT_INSTALL)
	parser.add_argument("--port", type=int, default=17171)
	parser.add_argument("--qm-port", type=int, default=17173)
	parser.add_argument("--shm", type=int, default=17011)
	parser.add_argument("--stall-old", default=None, metavar="ROUND:MS")
	parser.add_argument("--debug-level", type=int, default=3)
	parser.add_argument("--logs", choices=("check", "report"), default="check")
	args = parser.parse_args(argv)
	if len(args.commits) > 2:
		parser.print_usage(sys.stderr)
		return 2
	import scenarios
	if args.scenario not in scenarios.SCENARIOS:
		print("trace_check.py: no scenario %r (tools/trace/scenarios.py has %s)" % (args.scenario,
			", ".join(sorted(scenarios.SCENARIOS))), file=sys.stderr)
		return 2
	# The houses the scenario's test characters own (give_houses), as {house id as text: client key} for JSON.
	houses = {str(house): key for house, key in getattr(scenarios.SCENARIOS[args.scenario], "houses", {}).items()}
	stop_round = args.stop_round
	if stop_round is None:
		stop_round = getattr(scenarios.SCENARIOS[args.scenario], "stop_round", None) or 232
	options = {"scenario": args.scenario, "stop_round": stop_round, "seed": args.seed, "install": args.install,
		"port": args.port, "qm_port": args.qm_port, "shm": args.shm, "stall_old": args.stall_old,
		"debug_level": args.debug_level, "logs": args.logs, "houses": houses}
	check_options(options)
	who = "trace_check"
	old_tree, old_label = repro_check.resolve(args.commits[0] if args.commits else "faithful")
	new_tree, new_label = repro_check.resolve(args.commits[1] if len(args.commits) > 1 else None)
	# The run lock of repro_check: the builds and the runs use the shared cache, so concurrent checks wait.
	with repro_check.run_lock():
		old_dir = repro_check.build_tree(old_tree, old_label, "trace", who)
		new_dir = repro_check.build_tree(new_tree, new_label, "trace", who)
		if old_tree == new_tree:
			log("%s: %s and %s are the same tree: both runs use the same build, so this proves only that the runs "
				"repeat" % (who, old_label, new_label))
		return repro_check.call_worker(__file__, WORKERS, "trace", {"options": options,
			"old": {"dir": old_dir, "label": old_label, "own_rights": known_own_rights(old_tree)},
			"new": {"dir": new_dir, "label": new_label, "own_rights": known_own_rights(new_tree)}},
			data_archive({"old": old_tree, "new": new_tree}))["code"]


if __name__ == "__main__":
	sys.stdout.reconfigure(errors="backslashreplace")
	try:
		sys.exit(main(sys.argv[1:]))
	except CheckError as e:
		print("trace_check.py: %s" % e, file=sys.stderr)
		sys.exit(1)
