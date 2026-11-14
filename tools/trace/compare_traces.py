#!/usr/bin/env python3
"""Compares the client traces of two scenario runs.

usage: compare_traces.py OLD_DIR NEW_DIR [--context N] [--no-hex] [--allow-clock]

Each trace is a list of records (client commands, notes and decoded server
messages). The sequences are compared message by message on the decoded
text; map descriptions are compared tile by tile. Rounds and times are not
compared but shown. Date and time strings in texts are normalised; the lines
where that hid a difference are counted as allowed differences.

--allow-clock: messages that follow the wall clock, not the game's rounds,
are left out of the comparison and compared on their own, as allowed
differences. Two runs at different times send them with other values or in
other rounds:
  light   the world light (message 0x82, SendAmbiente), from the time of day;
  record  the broadcast "New record: N players are logged in.", sent after
          the player list, which the game makes when the minute of the real
          time is a multiple of 5 (AdvanceGame).

The first output line is the summary. Exit status 0 when the traces match
apart from the allowed differences.
"""
import argparse
import difflib
import json
import os
import re
import sys

DATE_RX = [
	(re.compile(r"\d{1,2}\. ?[A-Z][a-z]{2} \d{4} \d{2}:\d{2}:\d{2}( [A-Z]{3,4})?"), "<DATE>"),
	(re.compile(r"\d{2}\.\d{2}\.\d{4} \d{2}:\d{2}(:\d{2})?"), "<DATE>"),
	(re.compile(r"[A-Z][a-z]{2} [A-Z][a-z]{2} +\d+ \d{2}:\d{2}:\d{2} \d{4}"), "<DATE>"),
]
RECORD_RX = re.compile(r"^Message \w+ 'New record: \d+ players are logged in\.'$")


def clock_kind(r):
	"""The kind of wall-clock message a record is (see --allow-clock), or None."""
	if r["dir"] != "S":
		return None
	if r["op"] == "0x82":
		return "light"
	if r["op"] == "0xb4" and RECORD_RX.match(r["text"]):
		return "record"
	return None


def norm(text):
	for rx, rep in DATE_RX:
		text = rx.sub(rep, text)
	return text


def load(path):
	recs = []
	with open(path) as f:
		for line in f:
			line = line.strip()
			if line:
				recs.append(json.loads(line))
	return recs


def lines_of(recs, allow_clock):
	"""Flattens records into comparable lines: [(normalised key, record index,
	key)], and the wall-clock messages left out: {kind: [(round, text)]}."""
	out = []
	clock = {}
	for i, r in enumerate(recs):
		kind = clock_kind(r) if allow_clock else None
		if kind:
			clock.setdefault(kind, []).append((r["round"], r["text"]))
			continue
		if r["dir"] == "N" and r["text"].startswith("connection closed"):
			key = "N connection closed"
		elif r["dir"] == "N" and r["text"].startswith("socket error"):
			key = "N connection closed"
		else:
			key = "%s %s" % (r["dir"], r["text"].split("\n")[0])
		out.append((norm(key), i, key))
		for t in r["text"].split("\n")[1:]:
			key = "%s   %s" % (r["dir"], t.strip())
			out.append((norm(key), i, key))
	return out, clock


def show(side, recs, items, want_hex):
	shown = set()
	res = []
	for key, i, _ in items:
		r = recs[i]
		res.append("  %s r%-4s %s" % (side, r["round"], key))
		if want_hex and i not in shown and r["dir"] == "S":
			shown.add(i)
			h = r["hex"]
			res.append("  %s       hex %s%s" % (side, h[:160], "..." if len(h) > 160 else ""))
	return res


def compare(old, new, context, want_hex, allow_clock):
	"""Returns the difference blocks, the line counts, the number of lines that
	differ only in a date, and the wall-clock messages of both sides."""
	lo, clock_old = lines_of(old, allow_clock)
	ln, clock_new = lines_of(new, allow_clock)
	sm = difflib.SequenceMatcher(None, [k for k, _, _ in lo], [k for k, _, _ in ln], autojunk=False)
	blocks = []
	dates = 0
	for tag, i1, i2, j1, j2 in sm.get_opcodes():
		if tag == "equal":
			dates += sum(1 for a, b in zip(lo[i1:i2], ln[j1:j2]) if a[2] != b[2])
			continue
		ctx = [k for k, _, _ in lo[max(0, i1 - context):i1]]
		# The last client command or step note before the difference.
		anchor = None
		for k, idx, _ in reversed(lo[:i1]):
			if k.startswith("N --- round") or k.startswith("C "):
				anchor = "%s (old r%s)" % (k, old[idx]["round"])
				break
		blocks.append((tag, anchor, ctx, show("<", old, lo[i1:i2], want_hex), show(">", new, ln[j1:j2], want_hex)))
	return blocks, len(lo), len(ln), dates, clock_old, clock_new


def compare_dirs(old_dir, new_dir, context=2, want_hex=True, allow_clock=False):
	"""Compares the traces of two folders. Returns (number of difference
	blocks, summary line, report lines, allowed-difference lines)."""
	names = sorted(set(os.listdir(old_dir)) | set(os.listdir(new_dir)))
	report = []
	allowed = []
	total = 0
	per = []
	for n in names:
		if not n.endswith(".jsonl"):
			continue
		po = os.path.join(old_dir, n)
		pn = os.path.join(new_dir, n)
		if not os.path.exists(po) or not os.path.exists(pn):
			report.append("%s: only in one run" % n)
			total += 1
			continue
		o = load(po)
		r = load(pn)
		blocks, no, nn, dates, clock_old, clock_new = compare(o, r, context, want_hex, allow_clock)
		total += len(blocks)
		per.append("%s %d" % (n[:-6], len(blocks)))
		for kind in sorted(set(clock_old) | set(clock_new)):
			a = clock_old.get(kind, [])
			b = clock_new.get(kind, [])
			sm = difflib.SequenceMatcher(None, a, b, autojunk=False)
			same = sum(size for _, _, size in sm.get_matching_blocks())
			allowed.append("client %s: %s: %d messages old, %d new, %d the same in value and round" % (
				n[:-6], kind, len(a), len(b), same))
			if same != len(a) or same != len(b):
				for k in range(min(max(len(a), len(b)), 6)):
					left = "r%-4s %s" % a[k] if k < len(a) else "-"
					right = "r%-4s %s" % b[k] if k < len(b) else "-"
					allowed.append("    old %-48s new %s" % (left, right))
		if dates:
			allowed.append("client %s: %d lines differ only in a date or time" % (n[:-6], dates))
		report.append("=" * 78)
		report.append("client %s: %d records old, %d new; %d lines old, %d new; %d difference blocks" % (
			n[:-6], len(o), len(r), no, nn, len(blocks)))
		for k, (tag, anchor, ctx, left, right) in enumerate(blocks, 1):
			report.append("-" * 78)
			report.append("[%s #%d] %s after: %s" % (n[:-6], k, tag, anchor))
			for c in ctx:
				report.append("    %s" % c)
			report.extend(left)
			report.extend(right)
	if total:
		state = "differ"
	elif allowed:
		state = "identical apart from the allowed differences"
	else:
		state = "identical"
	summary = "%s: %d difference blocks (%s)" % (state, total, ", ".join(per))
	return total, summary, report, allowed


def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("old")
	ap.add_argument("new")
	ap.add_argument("--context", type=int, default=2)
	ap.add_argument("--no-hex", action="store_true")
	ap.add_argument("--allow-clock", action="store_true")
	args = ap.parse_args()
	total, summary, report, allowed = compare_dirs(args.old, args.new, args.context, not args.no_hex, args.allow_clock)
	print(summary)
	if allowed:
		print("allowed differences:")
		print("\n".join("  " + line for line in allowed))
	print("\n".join(report))
	return 0 if total == 0 else 1


if __name__ == "__main__":
	sys.exit(main())
