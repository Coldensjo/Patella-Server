#!/usr/bin/env python3
"""Prints a client trace readably. usage: show_trace.py FILE.jsonl [--maps] [--hex]
Without --maps only the first line of map descriptions is shown."""
import json
import sys


def main():
	path = sys.argv[1]
	maps = "--maps" in sys.argv
	hexes = "--hex" in sys.argv
	with open(path) as f:
		for line in f:
			r = json.loads(line)
			text = r["text"].split("\n")
			print("%8.3f r%-4s %s %-4s %s" % (r["t"], r["round"], r["dir"], r["op"] or "", text[0]))
			if maps:
				for t in text[1:]:
					print("%22s%s" % ("", t))
			if hexes and r["hex"]:
				print("%22shex %s" % ("", r["hex"]))


if __name__ == "__main__":
	main()
