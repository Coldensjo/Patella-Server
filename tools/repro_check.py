r"""Prove that a change leaves the programs as they were: the stripped binaries of two trees, byte for byte.

Usage, from any folder (the checkout is the one this file is in), on Windows or in WSL:
  python -I tools/repro_check.py [<old> [<new>]]

<old> is a commit, HEAD when left out. <new> is a commit too; left out, it is the working tree: the files `git add -A`
would commit, untracked ones included and ignored ones not (so never src/RSA_PrivateKey.hh or a key.pem).

The proof for a change that must not change the programs: comments, names, named constants. A moved line still
changes the test programs (their CHECK macros store __LINE__), and a renamed class with virtual functions changes the
type name the binary keeps; both show up here, as they should.

What it does:
  1. Exports each tree with git archive (Windows git; in WSL, git.exe) and hands it to WSL Ubuntu-24.04, where it
     replaces the sources in one fixed folder, ~/.cache/patella-check/src. A file whose content differs is written,
     the others keep their time stamps, so the build compiles only what changed. Both trees are built at that one
     path, so __FILE__ and every other path is the same in both.
  2. Builds them in ~/.cache/patella-check/build-repro with the normal flags (RelWithDebInfo, -O2 -DNDEBUG) but
     without debug information (-g0) and without a build ID (-Wl,--build-id=none: the ID is a hash of the linked
     file, symbol table included, so a renamed function would change it); SQLite comes from
     CMAKE_PREFIX_PATH=/home/chris/patella-deps/usr.
  3. The RSA key: a tree that reads its key from a key file at run time (it has src/rsakeyfile.cc, and its
     keygen/CMakeLists.txt no longer knows PATELLA_RSA_KEY_HEADER) needs none to build; its binaries hold no key, so
     they are the same whatever key is used. Older trees compiled the key in, and their builds made a random one
     without src/RSA_PrivateKey.hh, so every build would differ. Those builds all use one key,
     ~/.cache/patella-check/key/RSA_PrivateKey.hh, made once by this script in the old rsakeygen's header format:
     given as PATELLA_RSA_KEY_HEADER to a tree whose keygen/CMakeLists.txt knows that variable, and copied to
     src/RSA_PrivateKey.hh in a tree from before it. tools/trace_check.py gives the runs of a key-file tree the same
     key as a key.pem.
  4. Keeps every executable of the build (game, querymanager, querymanager-seed, loginserver, rsakeygen and the test
     programs), as linked and stripped (strip --strip-all), in ~/.cache/patella-check/builds/repro/<tree>/. A tree
     that was built before, with the same configure command and the same compiler and cmake, is not built again: the
     cache is per tree, so a commit and a working tree with the same files share it. Deleting
     ~/.cache/patella-check/key makes a new key and empties the cache.
  5. Compares the stripped binaries byte for byte. When one differs, it names the sections that differ, the strings
     of .rodata that are only in one of them, and the functions whose code differs. Those come from the unstripped
     binaries: objdump's disassembly of each function, with every address replaced by the symbol it names (by the
     text, for a string literal). Code that moved without changing (a jump table, say) can still be listed, so the
     list is where to look, not a proof.

Checks run one at a time: the source, build and cache folders are shared by every checkout (git worktrees too), so
a run holds ~/.cache/patella-check/run.lock from the first export to the end of the comparison, and a second run, a
trace_check too, waits for it ("waiting for another check").

Exit code 0: the binaries are equal in every byte. 1: they differ, or a build or command failed. 2: wrong usage.
"""

import contextlib
import io
import json
import os
import re
import secrets
import shutil
import struct
import subprocess
import sys
import tarfile
import threading

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DISTRO = "Ubuntu-24.04"
PREFIX_PATH = "/home/chris/patella-deps/usr"
CACHE = "~/.cache/patella-check"
# Windows' git; in WSL too, since git itself is slow on a /mnt/c checkout (the WSL PATH may lack Windows' folders).
GIT = "git" if os.name == "nt" else next((p for p in (shutil.which("git.exe"), "/mnt/c/Program Files/Git/cmd/git.exe")
	if p and os.path.isfile(p)), "git.exe")
RESULT = "@@RESULT "
# Flags of the repro builds on top of the normal ones (see the module comment).
REPRO_FLAGS = ["-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O2 -g0 -DNDEBUG", "-DCMAKE_EXE_LINKER_FLAGS=-Wl,--build-id=none"]


class CheckError(Exception):
	pass


def log(text):
	print(text, flush=True)


# ---------------------------------------------------------------- Windows side: git and wsl.exe

def git(*arguments, env=None, binary=False):
	result = subprocess.run([GIT] + list(arguments), cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
		check=False)
	if result.returncode != 0:
		raise CheckError("git %s: %s" % (" ".join(arguments), result.stderr.decode("utf-8", "replace").strip()))
	return result.stdout if binary else result.stdout.decode("utf-8").strip()


def linux_path(path):
	"""The path of a Windows file as WSL sees it (C:\\x\\y -> /mnt/c/x/y); a Linux path as it is."""
	if os.name != "nt":
		return path
	drive, rest = os.path.splitdrive(os.path.abspath(path))
	return "/mnt/%s%s" % (drive[0].lower(), rest.replace("\\", "/"))


def host_path(path):
	"""A path git.exe printed, as this Python can open it (C:/x -> /mnt/c/x in WSL)."""
	if os.name != "nt" and re.match(r"^[A-Za-z]:[/\\]", path):
		return "/mnt/%s%s" % (path[0].lower(), path[2:].replace("\\", "/"))
	return os.path.join(ROOT, path)


def resolve(spec):
	"""(tree, label) of a commit, or of the working tree when spec is None. The working tree's is the tree `git add -A`
	would make, written with a temporary index; the real index is not touched."""
	if spec is not None:
		commit = git("rev-parse", "--verify", "--quiet", spec + "^{commit}")
		if not commit:
			raise CheckError("not a commit: %s" % spec)
		return git("rev-parse", commit + "^{tree}"), "%s (%s)" % (spec, commit[:10])
	index = git("rev-parse", "--git-path", "index")
	# One per process: a trace_check and a repro_check may both read the working tree at the same time.
	temporary = git("rev-parse", "--git-path", "patella-check-index-%d" % os.getpid())
	try:
		if os.path.isfile(host_path(index)):
			shutil.copyfile(host_path(index), host_path(temporary))
		env = dict(os.environ, GIT_INDEX_FILE=temporary)
		if os.name != "nt":
			env["WSLENV"] = ":".join(filter(None, [os.environ.get("WSLENV"), "GIT_INDEX_FILE"]))
		git("add", "--all", "--", ".", env=env)
		tree = git("write-tree", env=env)
	finally:
		if os.path.isfile(host_path(temporary)):
			os.remove(host_path(temporary))
	return tree, "the working tree"


def call_worker(script, functions, command, arguments, data=b""):
	"""Runs a step in WSL: through wsl.exe on Windows (the script's --worker mode), in this process in WSL. The step's
	output is printed as it comes; it returns the step's result."""
	if os.name != "nt":
		return functions[command](arguments, data)
	process = subprocess.Popen(["wsl.exe", "-d", DISTRO, "--exec", "python3", "-u", linux_path(script), "--worker",
		command, json.dumps(arguments)], stdin=subprocess.PIPE, stdout=subprocess.PIPE)

	def feed():
		try:
			process.stdin.write(data)
			process.stdin.close()
		except OSError:
			pass

	writer = threading.Thread(target=feed, daemon=True)
	writer.start()
	result = None
	try:
		for raw in process.stdout:
			line = raw.decode("utf-8", "replace").rstrip("\r\n")
			if line.startswith(RESULT):
				result = json.loads(line[len(RESULT):])
			else:
				log(line)
		code = process.wait()
	except KeyboardInterrupt:
		process.terminate()
		raise
	if code != 0 or result is None:
		raise CheckError("the WSL step '%s' failed (exit %d)" % (command, code))
	return result


def worker_main(functions, argv):
	"""--worker <command> <arguments as JSON>, in WSL: runs the step with the standard input as its data and prints its
	result as the last line."""
	data = sys.stdin.buffer.read()
	try:
		result = functions[argv[0]](json.loads(argv[1]), data)
	except CheckError as e:
		print("%s: %s" % (os.path.basename(sys.argv[0]), e), file=sys.stderr, flush=True)
		return 1
	print(RESULT + json.dumps(result), flush=True)
	return 0


@contextlib.contextmanager
def run_lock():
	"""Holds ~/.cache/patella-check/run.lock for a whole check, so that checks from several checkouts run one after the
	other. In WSL with lock(); on Windows through a WSL process (--hold-lock) that holds it until its input closes,
	which also happens when this process dies."""
	if os.name != "nt":
		with lock(cache_dir(), "run"):
			yield
		return
	process = subprocess.Popen(["wsl.exe", "-d", DISTRO, "--exec", "python3", "-u", linux_path(__file__),
		"--hold-lock"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
	try:
		for raw in process.stdout:
			line = raw.decode("utf-8", "replace").rstrip("\r\n")
			if line.startswith(RESULT):
				break
			log(line)
		else:
			raise CheckError("could not take ~/.cache/patella-check/run.lock (exit %s)" % process.wait())
		yield
	finally:
		with contextlib.suppress(OSError):
			process.stdin.close()
		process.wait()


def hold_lock():
	"""--hold-lock, in WSL: takes run.lock, says so, and keeps it until the standard input closes."""
	with lock(cache_dir(), "run"):
		print(RESULT + "{}", flush=True)
		sys.stdin.buffer.read()
	return 0


def build_tree(tree, label, mode, who):
	"""Builds a tree in WSL (mode "repro" or "trace"), or finds it in the cache; returns the folder with its
	executables."""
	log("%s: %s, tree %s" % (who, label, tree[:10]))
	data = git("archive", "--format=tar", tree, binary=True)
	result = call_worker(__file__, WORKERS, "build", {"tree": tree, "mode": mode, "prefix": PREFIX_PATH}, data)
	return result["dir"]


# ---------------------------------------------------------------- WSL side: key, sources, build

def cache_dir():
	return os.path.expanduser(CACHE)


@contextlib.contextmanager
def lock(cache, name):
	"""Holds ~/.cache/patella-check/<name>.lock: run.lock for a whole check (run_lock), build.lock and trace.lock for
	one step, so that two checks never share the source and build folders."""
	import fcntl
	os.makedirs(cache, exist_ok=True)
	with open(os.path.join(cache, name + ".lock"), "w") as f:
		try:
			fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
		except OSError:
			log("  waiting for another check (%s.lock)" % name)
			fcntl.flock(f, fcntl.LOCK_EX)
		yield


SMALL_PRIMES = [p for p in range(3, 2000, 2) if all(p % k for k in range(3, int(p ** 0.5) + 1, 2))]


def probable_prime(n):
	for p in SMALL_PRIMES:
		if n % p == 0:
			return n == p
	d, s = n - 1, 0
	while d % 2 == 0:
		d //= 2
		s += 1
	for _ in range(40):
		x = pow(secrets.randbelow(n - 3) + 2, d, n)
		if x in (1, n - 1):
			continue
		for _ in range(s - 1):
			x = pow(x, 2, n)
			if x == n - 1:
				break
		else:
			return False
	return True


def random_prime():
	"""A 512-bit prime as rsakeygen makes them: the two top bits set, p-1 not a multiple of 65537."""
	while True:
		n = int.from_bytes(os.urandom(64), "big") | (3 << 510) | 1
		if (n - 1) % 65537 != 0 and probable_prime(n):
			return n


def key_header(cache):
	"""The one key of all builds; made on the first call. A new key empties the build cache."""
	path = os.path.join(cache, "key", "RSA_PrivateKey.hh")
	if os.path.isfile(path):
		return path
	log("  making the RSA key of the checks: %s" % path)
	p = random_prime()
	q = random_prime()
	while q == p:
		q = random_prime()
	text = ("#ifndef PATELLA_RSA_PRIVATEKEY_HH\n"
		"#define PATELLA_RSA_PRIVATEKEY_HH\n"
		"\n"
		"// Generated by tools/repro_check.py, in rsakeygen's format, for the builds of the checks.\n"
		"// RsaModulus=%d\n"
		"\n"
		"static const char RSA_PRIME_P[156] = \"%d\";\n"
		"static const char RSA_PRIME_Q[156] = \"%d\";\n"
		"\n"
		"#endif\n") % (p * q, p, q)
	os.makedirs(os.path.dirname(path), exist_ok=True)
	with open(path + ".tmp", "w", encoding="ascii") as f:
		f.write(text)
	os.rename(path + ".tmp", path)
	shutil.rmtree(os.path.join(cache, "builds"), ignore_errors=True)
	return path


def key_source(data):
	"""How the tree in data (a tar) gets its RSA key: "variable" (PATELLA_RSA_KEY_HEADER in keygen/CMakeLists.txt),
	"file" (no key at build time: src/rsakeyfile.cc reads key.pem at run time) or "header" (src/RSA_PrivateKey.hh,
	the trees before both)."""
	with tarfile.open(fileobj=io.BytesIO(data)) as archive:
		names = set(archive.getnames())
		keygen = ""
		if "keygen/CMakeLists.txt" in names:
			keygen = archive.extractfile("keygen/CMakeLists.txt").read().decode("utf-8", "replace")
	if "PATELLA_RSA_KEY_HEADER" in keygen:
		return "variable"
	return "file" if "src/rsakeyfile.cc" in names else "header"


def same_content(a, b):
	if os.path.islink(b) or not os.path.isfile(b) or os.path.getsize(a) != os.path.getsize(b):
		return False
	with open(a, "rb") as x, open(b, "rb") as y:
		return x.read() == y.read()


def sync(stage, target):
	"""Makes the folder target hold what stage holds. Files of equal content are left alone, with their times."""
	written = 0
	wanted = set()
	for folder, _, names in os.walk(stage):
		relative = os.path.relpath(folder, stage)
		os.makedirs(os.path.normpath(os.path.join(target, relative)), exist_ok=True)
		for name in names:
			path = os.path.normpath(os.path.join(relative, name))
			wanted.add(path)
			source = os.path.join(stage, path)
			destination = os.path.join(target, path)
			if same_content(source, destination):
				continue
			if os.path.lexists(destination):
				os.remove(destination)
			# A plain copy: the file gets the time of now, so make rebuilds what depends on it.
			shutil.copyfile(source, destination)
			shutil.copymode(source, destination)
			written += 1
	removed = 0
	for folder, _, names in os.walk(target, topdown=False):
		for name in names:
			path = os.path.normpath(os.path.join(os.path.relpath(folder, target), name))
			if path not in wanted:
				os.remove(os.path.join(target, path))
				removed += 1
		if folder != target and not os.listdir(folder):
			os.rmdir(folder)
	return written, removed


def executables(folder):
	"""The ELF executables of a build folder, outside CMakeFiles: [relative path]."""
	found = []
	for current, folders, names in os.walk(folder):
		folders[:] = sorted(f for f in folders if f != "CMakeFiles")
		for name in names:
			path = os.path.join(current, name)
			if os.path.islink(path) or not os.access(path, os.X_OK):
				continue
			with open(path, "rb") as f:
				if f.read(4) != b"\x7fELF":
					continue
			found.append(os.path.relpath(path, folder))
	return sorted(found)


def run_logged(command, log_file):
	"""Runs a command with its output appended to log_file; returns (exit code, output)."""
	result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
	output = result.stdout.decode("utf-8", "replace")
	with open(log_file, "a", encoding="utf-8") as f:
		f.write("$ %s\n%s\n" % (" ".join(command), output))
	return result.returncode, output


def tool_version(command):
	"""The first line of a tool's --version."""
	result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
	return result.stdout.decode("utf-8", "replace").partition("\n")[0].strip()


def build(arguments, data):
	"""WSL step: builds the tree in data (a tar) unless the cache has it. Returns {"dir": its folder}."""
	tree = arguments["tree"]
	mode = arguments["mode"]
	if mode not in ("repro", "trace") or not re.fullmatch(r"[0-9a-f]{40,64}", tree):
		raise CheckError("bad build request %r" % arguments)
	cache = cache_dir()
	with lock(cache, "build"):
		key = key_header(cache)
		key_kind = key_source(data)
		out = os.path.join(cache, "builds", mode, tree)
		source = os.path.join(cache, "src")
		folder = os.path.join(cache, "build-" + mode)
		configure = ["cmake", "-S", source, "-B", folder, "-G", "Unix Makefiles", "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
			"-DCMAKE_PREFIX_PATH=" + arguments["prefix"]]
		if key_kind == "variable":
			configure.append("-DPATELLA_RSA_KEY_HEADER=" + key)
		configure += REPRO_FLAGS if mode == "repro" else []
		# A cached build counts only if it was made the same way: the same configure command and the same compiler.
		settings = "\n".join(configure + [tool_version(["c++", "--version"]), tool_version(["cmake", "--version"])]) + "\n"
		if os.path.isfile(os.path.join(out, "complete")):
			with open(os.path.join(out, "settings") if os.path.isfile(os.path.join(out, "settings")) else os.devnull) as f:
				if f.read() == settings:
					log("  built before: %s" % out)
					return {"dir": out}
			log("  built before, but with other flags or another compiler: building again")
		stage = os.path.join(cache, "stage")
		shutil.rmtree(stage, ignore_errors=True)
		os.makedirs(stage)
		with tarfile.open(fileobj=io.BytesIO(data)) as archive:
			archive.extractall(stage, filter="tar")
		if key_kind == "header":
			shutil.copyfile(key, os.path.join(stage, "src", "RSA_PrivateKey.hh"))
		written, removed = sync(stage, source)
		shutil.rmtree(stage)
		log("  sources in %s: %d files written, %d removed; %s" % (source, written, removed, {
			"variable": "the key is built in, given as PATELLA_RSA_KEY_HEADER",
			"file": "the programs read their key from a key file, none is built in",
			"header": "the tree predates PATELLA_RSA_KEY_HEADER, the key is src/RSA_PrivateKey.hh"}[key_kind]))
		os.makedirs(folder, exist_ok=True)
		# Linked again every time, so that no executable of another tree is left over.
		for path in executables(folder):
			os.remove(os.path.join(folder, path))
		shutil.rmtree(out, ignore_errors=True)
		os.makedirs(out)
		log_file = os.path.join(out, "build.log")
		log("  building in %s (%s)" % (folder, "-g0, no build ID" if mode == "repro" else "with debug information"))
		code, output = run_logged(configure, log_file)
		if code == 0:
			code, output = run_logged(["cmake", "--build", folder, "-j", str(os.cpu_count() or 4)], log_file)
		if code != 0:
			lines = output.splitlines()
			errors = [line for line in lines if "error" in line.lower()]
			log("\n".join(errors[:60] if errors else lines[-60:]))
			raise CheckError("the build failed (exit %d); all of it is in %s" % (code, log_file))
		warnings = sorted(set(line for line in output.splitlines() if "warning:" in line))
		for line in warnings[:40]:
			log("  WARNING: " + line)
		found = executables(folder)
		if not found:
			raise CheckError("the build made no executables")
		for path in found:
			kept = os.path.join(out, "bin", path)
			os.makedirs(os.path.dirname(kept), exist_ok=True)
			shutil.copy2(os.path.join(folder, path), kept)
			if mode == "repro":
				stripped = os.path.join(out, "stripped", path)
				os.makedirs(os.path.dirname(stripped), exist_ok=True)
				code, output = run_logged(["strip", "--strip-all", "-o", stripped, kept], log_file)
				if code != 0:
					raise CheckError("strip %s failed: %s" % (path, output.strip()))
		with open(os.path.join(out, "settings"), "w") as f:
			f.write(settings)
		with open(os.path.join(out, "complete"), "w") as f:
			f.write("\n".join(found) + "\n")
		log("  built: %s" % ", ".join(found))
		return {"dir": out}


# ---------------------------------------------------------------- WSL side: comparison

def elf_sections(data):
	"""{name: (type, address, offset, size)} of an ELF64 file."""
	if data[:4] != b"\x7fELF" or data[4] != 2:
		raise CheckError("not an ELF64 file")
	offset, = struct.unpack_from("<Q", data, 0x28)
	size, count, names_index = struct.unpack_from("<HHH", data, 0x3a)
	headers = [struct.unpack_from("<IIQQQQ", data, offset + i * size) for i in range(count)]
	names = headers[names_index][4]
	sections = {}
	for name, kind, _, address, start, length in headers:
		end = data.index(b"\0", names + name)
		sections[data[names + name:end].decode("ascii", "replace")] = (kind, address, start, length)
	return sections


def section_bytes(data, section):
	kind, _, start, length = section
	return b"" if kind == 8 else data[start:start + length]   # 8: SHT_NOBITS (.bss)


def is_text(text):
	"""Whether bytes look like a string literal: printable, Latin-1 letters allowed, but mostly ASCII (a jump table
	of negative offsets is mostly bytes from 0x80 up)."""
	return bool(re.fullmatch(rb"[\x20-\x7e\t\n\xa0-\xff]+", text)) and len(re.findall(rb"[\x20-\x7e\t\n]", text)) >= 0.8 * len(text)


def rodata_strings(data, sections):
	"""The strings (4 characters or more, ended by a zero byte) of .rodata, with their counts."""
	counts = {}
	if ".rodata" in sections:
		for text in section_bytes(data, sections[".rodata"]).split(b"\0"):
			if len(text) >= 4 and is_text(text):
				counts[text] = counts.get(text, 0) + 1
	return counts


ADDRESS_COMMENT = re.compile(r"#\s*([0-9a-f]+)(?: <([^>]*)>)?\s*$")


def functions(path):
	"""{function: [instruction]} of an unstripped executable, from objdump, with the addresses replaced by what they
	name: a symbol, or the text of a string literal in .rodata."""
	with open(path, "rb") as f:
		data = f.read()
	sections = elf_sections(data)
	rodata = sections.get(".rodata")

	def target(address, symbol):
		"""A named object by its name; in .rodata, a string literal by its text and anything else (a jump table, whose
		bytes depend on where the code is) as just "rodata"."""
		if symbol and "+0x" not in symbol:
			return symbol
		if rodata and rodata[1] <= address < rodata[1] + rodata[3]:
			start = rodata[2] + address - rodata[1]
			end = data.find(b"\0", start, start + 256)
			text = data[start:end] if end > start else b""
			if text and is_text(text):
				return "rodata:%r" % text
			return "rodata"
		return symbol or "?"

	result = subprocess.run(["objdump", "-d", "--no-show-raw-insn", "-w", "-C", path], stdout=subprocess.PIPE,
		stderr=subprocess.PIPE, check=False)
	if result.returncode != 0:
		raise CheckError("objdump %s: %s" % (path, result.stderr.decode("utf-8", "replace").strip()))
	found = {}
	current = None
	for line in result.stdout.decode("utf-8", "replace").splitlines():
		header = re.match(r"^[0-9a-f]+ <(.+)>:$", line)
		if header:
			name = header.group(1)
			key = name
			number = 2
			while key in found:
				key = "%s #%d" % (name, number)
				number += 1
			current = found[key] = []
			continue
		if line.startswith("Disassembly of section"):
			current = None
			continue
		instruction = re.match(r"^\s*[0-9a-f]+:\s*(.*)$", line)
		if current is None or not instruction:
			continue
		text = instruction.group(1)
		comment = ADDRESS_COMMENT.search(text)
		if comment:
			text = text[:comment.start()] + "# " + target(int(comment.group(1), 16), comment.group(2))
		text = re.sub(r"-?0x[0-9a-f]+\(%rip\)", "(%rip)", text)
		text = re.sub(r"\b[0-9a-f]+ <([^>]*)>", r"<\1>", text)
		current.append(text)
	return found


def explain(old_dir, new_dir, path):
	"""Prints where the binary differs: sections, strings, functions."""
	with open(os.path.join(old_dir, "stripped", path), "rb") as f:
		a = f.read()
	with open(os.path.join(new_dir, "stripped", path), "rb") as f:
		b = f.read()
	sections_a = elf_sections(a)
	sections_b = elf_sections(b)
	for name in sorted(set(sections_a) | set(sections_b)):
		if name not in sections_b or name not in sections_a:
			log("      section %s only in %s" % (name, "old" if name in sections_a else "new"))
			continue
		x = section_bytes(a, sections_a[name])
		y = section_bytes(b, sections_b[name])
		if sections_a[name][3] != sections_b[name][3]:
			log("      section %s: %d bytes, were %d" % (name, sections_b[name][3], sections_a[name][3]))
		elif x != y:
			log("      section %s: %d of %d bytes differ" % (name, sum(1 for i, j in zip(x, y) if i != j), len(x)))
	strings_a = rodata_strings(a, sections_a)
	strings_b = rodata_strings(b, sections_b)
	for label, mine, other in (("old", strings_a, strings_b), ("new", strings_b, strings_a)):
		only = [s for s in sorted(mine) if mine[s] > other.get(s, 0)]
		for s in only[:15]:
			log("      string only in %s: %r" % (label, s.decode("latin-1")))
		if len(only) > 15:
			log("      ... %d more strings only in %s" % (len(only) - 15, label))
	code_a = functions(os.path.join(old_dir, "bin", path))
	code_b = functions(os.path.join(new_dir, "bin", path))
	differing = [name for name in sorted(set(code_a) & set(code_b)) if code_a[name] != code_b[name]]
	for label, names in (("code differs", differing), ("only in old", sorted(set(code_a) - set(code_b))),
			("only in new", sorted(set(code_b) - set(code_a)))):
		if names:
			log("      functions, %s (%d): %s%s" % (label, len(names), ", ".join(names[:40]),
				", ..." if len(names) > 40 else ""))


def compare(arguments, data):
	"""WSL step: compares the stripped binaries of two cached builds. Returns {"code": 0 or 1}."""
	old_dir = arguments["old"]
	new_dir = arguments["new"]
	old = set(executables(os.path.join(old_dir, "stripped")))
	new = set(executables(os.path.join(new_dir, "stripped")))
	differing = 0
	for path in sorted(old | new):
		if path not in old or path not in new:
			log("  ONLY IN %s: %s" % ("OLD" if path in old else "NEW", path))
			differing += 1
			continue
		with open(os.path.join(old_dir, "stripped", path), "rb") as f:
			a = f.read()
		with open(os.path.join(new_dir, "stripped", path), "rb") as f:
			b = f.read()
		if a == b:
			log("  identical: %s (%d bytes)" % (path, len(a)))
			continue
		differing += 1
		log("  DIFFERENT: %s (%d bytes, were %d)" % (path, len(b), len(a)))
		explain(old_dir, new_dir, path)
	if differing:
		log("DIFFERENT: %d of %d binaries" % (differing, len(old | new)))
		return {"code": 1}
	log("IDENTICAL: all %d stripped binaries, every byte" % len(old))
	return {"code": 0}


WORKERS = {"build": build, "compare": compare}


def main(argv):
	if argv and argv[0] == "--worker":
		return worker_main(WORKERS, argv[1:])
	if argv == ["--hold-lock"]:
		return hold_lock()
	if argv and argv[0] in ("-h", "--help"):
		print(__doc__)
		return 0
	if len(argv) > 2 or any(a.startswith("-") for a in argv):
		print("usage: repro_check.py [<old> [<new>]]", file=sys.stderr)
		return 2
	who = "repro_check"
	old_tree, old_label = resolve(argv[0] if argv else "HEAD")
	new_tree, new_label = resolve(argv[1] if len(argv) > 1 else None)
	with run_lock():
		old_dir = build_tree(old_tree, old_label, "repro", who)
		new_dir = build_tree(new_tree, new_label, "repro", who)
		log("%s: %s against %s%s" % (who, new_label, old_label, " (the same tree)" if old_tree == new_tree else ""))
		return call_worker(__file__, WORKERS, "compare", {"old": old_dir, "new": new_dir})["code"]


if __name__ == "__main__":
	sys.stdout.reconfigure(errors="backslashreplace")
	try:
		sys.exit(main(sys.argv[1:]))
	except CheckError as e:
		print("repro_check.py: %s" % e, file=sys.stderr)
		sys.exit(1)
