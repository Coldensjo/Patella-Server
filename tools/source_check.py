r"""Prove what kind of change the sources went through: only comments, or only names. And rename.

Usage, from any folder (the checkout is the one this file is in):
  python -I tools/source_check.py comments [<old> [<new>]]
  python -I tools/source_check.py names [<old> [<new>]] Old=New [Old=New ...]
  python -I tools/source_check.py rename Old=New [Old=New ...]

<old> is a commit, HEAD when left out. <new> is a commit too; left out, it is the working tree. The sources are the
.cc, .hh and .h files under src/, querymanager/, loginserver/ and keygen/, without any extern/ folder. On Windows it
runs git; in WSL it runs Windows' git.exe, since git itself is slow on a /mnt/c checkout.

comments  Every source that differs between <old> and <new> is the same code: the same lines once the comments are
          removed, trailing white space is removed and empty lines are dropped. White space inside a line of code
          counts, because the test macros store their expressions as written. Added, deleted and renamed sources fail.
          Changed files that are not sources are listed and not checked.
names     Every source of <new> is <old>'s with the names replaced, white space apart: the same tokens once each old
          name is mapped to its new one (in code, comments and strings alike). No new name is in <old>, and no old name
          is left in <new>. Added, deleted and renamed sources fail.
rename    Replaces each old name, as a whole word, in every source of the working tree: code, comments and strings.
          The replacements inside string literals are listed, since a text the player sees may be among them.

Neither check proves that the binaries are the same (a moved line changes __LINE__ in the tests, a renamed class its
type name); tools/repro_check.py does.

Exit code 0: the check holds. 1: it does not (each finding is printed), or a command failed.
"""

import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GIT = "git" if os.name == "nt" else "git.exe"
FOLDERS = ("src", "querymanager", "loginserver", "keygen")
RAW_STRING = re.compile(r'R"([^()\\ ]{0,16})\(')
ENCODING_PREFIX = re.compile(r"(?:^|[^A-Za-z0-9_])(?:u8|u|U|L)$")
# An identifier, a number with its suffix, a run of white space, or any other single character.
TOKEN = re.compile(r"[A-Za-z_]\w*|\d[\w.]*|\s+|.", re.DOTALL)


class CheckError(Exception):
	pass


def spans(text):
	"""The comments and the string literals of a C++ text: [(start, end, "comment" or "string")]. A `//` comment ends
	before its newline."""
	found = []
	i = 0
	n = len(text)
	while i < n:
		c = text[i]
		if c == "/" and text.startswith("//", i):
			end = text.find("\n", i)
			end = n if end < 0 else end
			found.append((i, end, "comment"))
			i = end
		elif c == "/" and text.startswith("/*", i):
			end = text.find("*/", i + 2)
			end = n if end < 0 else end + 2
			found.append((i, end, "comment"))
			i = end
		elif c == '"':
			start = i
			raw = RAW_STRING.match(text, i - 1) if i > 0 and text[i - 1] == "R" else None
			if raw:
				close = ")" + raw.group(1) + '"'
				end = text.find(close, raw.end())
				i = n if end < 0 else end + len(close)
			else:
				i += 1
				while i < n and text[i] != '"' and text[i] != "\n":
					i += 2 if text[i] == "\\" else 1
				i += 1
			found.append((start, min(i, n), "string"))
		elif c == "'":
			# After a digit or a letter it separates digits (1'000), unless the letters are an encoding prefix (u8'a').
			if i > 0 and (text[i - 1].isalnum() or text[i - 1] == "_") and not ENCODING_PREFIX.search(text[max(0, i - 3):i]):
				i += 1
			else:
				i += 1
				while i < n and text[i] != "'" and text[i] != "\n":
					i += 2 if text[i] == "\\" else 1
				i += 1
		else:
			i += 1
	return found


def code_lines(text):
	"""The text's lines without comments, trailing white space and empty lines."""
	out = []
	last = 0
	for start, end, kind in spans(text):
		if kind == "comment":
			out.append(text[last:start])
			last = end
	out.append(text[last:])
	lines = [line.rstrip() for line in "".join(out).split("\n")]
	return [line for line in lines if line]


def tokens(text, names=None):
	"""The text's tokens without white space, each name of `names` replaced."""
	found = [t for t in TOKEN.findall(text) if not t.isspace()]
	return [names.get(t, t) for t in found] if names else found


def git(*arguments, binary=False):
	result = subprocess.run([GIT] + list(arguments), cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
	if result.returncode != 0:
		raise CheckError("git %s: %s" % (" ".join(arguments), result.stderr.decode("utf-8", "replace").strip()))
	return result.stdout if binary else result.stdout.decode("utf-8")


def is_source(path):
	parts = path.split("/")
	return parts[0] in FOLDERS and "extern" not in parts[1:-1] and path.endswith((".cc", ".hh", ".h"))


def read(commit, path):
	"""The file's text in the commit, or in the working tree when commit is None."""
	if commit is None:
		with open(os.path.join(ROOT, path), encoding="utf-8", errors="surrogateescape", newline="") as f:
			return f.read()
	return git("show", "%s:%s" % (commit, path), binary=True).decode("utf-8", "surrogateescape")


def sources(commit):
	"""The sources of the commit, or of the working tree when commit is None (the files git knows that exist)."""
	if commit is None:
		listed = git("ls-files", "-z", "--cached", "--others", "--exclude-standard", "--", *FOLDERS).split("\0")
		return sorted(p for p in listed if p and is_source(p) and os.path.isfile(os.path.join(ROOT, p)))
	listed = git("ls-tree", "-r", "-z", "--name-only", commit, "--", *FOLDERS).split("\0")
	return sorted(p for p in listed if p and is_source(p))


def changed(old, new):
	"""[(status, path)] of the files that differ between the commit `old` and `new` (the working tree when None),
	untracked files of the working tree included."""
	arguments = ["diff", "--name-status", "--no-renames", "-z", old] + ([new] if new else []) + ["--"]
	fields = git(*arguments).split("\0")
	entries = [(fields[i], fields[i + 1]) for i in range(0, len(fields) - 1, 2)]
	if new is None:
		for path in git("ls-files", "-z", "--others", "--exclude-standard").split("\0"):
			if path:
				entries.append(("A", path))
	return entries


def commits_and_names(arguments, most):
	"""Splits the arguments into at most `most` leading commits and the Old=New pairs after them."""
	commits = []
	while arguments and "=" not in arguments[0]:
		commits.append(arguments.pop(0))
	if len(commits) > most:
		raise CheckError("too many commits: %s" % " ".join(commits))
	names = {}
	for argument in arguments:
		old, _, new = argument.partition("=")
		if not re.fullmatch(r"[A-Za-z_]\w*", old) or not re.fullmatch(r"[A-Za-z_]\w*", new) or old in names:
			raise CheckError("not a rename, or given twice: %s" % argument)
		names[old] = new
	old = commits[0] if commits else "HEAD"
	new = commits[1] if len(commits) > 1 else None
	return old, new, names


def check_comments(arguments):
	old, new, names = commits_and_names(arguments, 2)
	if names:
		raise CheckError("comments takes no names")
	bad = 0
	checked = 0
	skipped = []
	for status, path in changed(old, new):
		if not is_source(path):
			skipped.append(path)
			continue
		checked += 1
		if status != "M":
			print("NOT A MODIFICATION (%s): %s" % (status, path))
			bad += 1
			continue
		before = code_lines(read(old, path))
		after = code_lines(read(new, path))
		if before == after:
			continue
		bad += 1
		for x, y in zip(before, after):
			if x != y:
				print("CODE DIFFERS: %s\n  was: %s\n  is:  %s" % (path, x, y))
				break
		else:
			print("CODE DIFFERS: %s (%d lines of code, were %d)" % (path, len(after), len(before)))
	for path in skipped:
		print("not checked (not a source): %s" % path)
	if bad:
		print("comments: %d of %d changed sources differ in more than comments" % (bad, checked))
		return 1
	print("comments: %d changed sources, the same code in each (only comments, line ends and empty lines differ)" % checked)
	return 0


def check_names(arguments):
	old, new, names = commits_and_names(arguments, 2)
	if not names:
		raise CheckError("names needs Old=New pairs")
	new_names = set(names.values())
	before = sources(old)
	after = sources(new)
	bad = 0
	differing = 0
	for path in sorted(set(before) ^ set(after)):
		print("%s: %s" % ("ADDED" if path in after else "DELETED", path))
		bad += 1
	for path in sorted(set(before) & set(after)):
		was = read(old, path)
		now = read(new, path)
		was_tokens = tokens(was)
		found = new_names.intersection(was_tokens)
		if found:
			print("NEW NAME ALREADY IN %s: %s" % (path, ", ".join(sorted(found))))
			bad += 1
		left = set(names).intersection(tokens(now))
		if left:
			print("OLD NAME LEFT IN %s: %s" % (path, ", ".join(sorted(left))))
			bad += 1
		if [names.get(t, t) for t in was_tokens] != tokens(now):
			print("OTHER DIFFERENCE IN %s" % path)
			bad += 1
		if was != now:
			differing += 1
	if bad:
		print("names: %d problems" % bad)
		return 1
	print("names: %d sources differ, in nothing but the %d names (and white space)" % (differing, len(names)))
	return 0


def rename(arguments):
	_, _, names = commits_and_names(arguments, 0)
	if not names:
		raise CheckError("rename needs Old=New pairs")
	pattern = re.compile(r"\b(%s)\b" % "|".join(map(re.escape, names)))
	total = 0
	files = 0
	for path in sources(None):
		text = read(None, path)
		strings = [(start, end) for start, end, kind in spans(text) if kind == "string"]
		for match in pattern.finditer(text):
			if any(start <= match.start() < end for start, end in strings):
				print("in a string literal: %s:%d: %s" % (path, text.count("\n", 0, match.start()) + 1, match.group(1)))
		replaced, count = pattern.subn(lambda m: names[m.group(1)], text)
		if count:
			with open(os.path.join(ROOT, path), "w", encoding="utf-8", errors="surrogateescape", newline="") as f:
				f.write(replaced)
			total += count
			files += 1
	print("rename: %d replacements in %d sources" % (total, files))
	return 0


def main(argv):
	commands = {"comments": check_comments, "names": check_names, "rename": rename}
	if not argv or argv[0] in ("-h", "--help") or argv[0] not in commands:
		print(__doc__)
		return 0 if argv and argv[0] in ("-h", "--help") else 2
	return commands[argv[0]](argv[1:])


if __name__ == "__main__":
	try:
		sys.exit(main(sys.argv[1:]))
	except (CheckError, subprocess.CalledProcessError) as e:
		print("source_check.py: %s" % e, file=sys.stderr)
		sys.exit(1)
