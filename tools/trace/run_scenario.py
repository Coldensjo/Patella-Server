#!/usr/bin/env python3
"""Runs a scripted scenario with the test clients against a running game
server and writes one trace per client (JSON lines) to --out.

usage: run_scenario.py --scenario NAME --out DIR --rsa-key FILE
                       [--objects items.toml] [--console FILE]
                       [--host H] [--port P] [--list]

--rsa-key: the server's key file (key.pem) or RSA_PrivateKey.hh (the clients
need its modulus).
--objects: the object types, the game's items.toml (default dat/items.toml),
or an original objects.srv.
--console: the server's console file with @@ROUND n@@ lines (trace_check.py
runs the server under gdb); scenario steps then happen at fixed server
rounds. Without it, rounds are seconds since the start of the scenario.
"""
import argparse
import os
import sys
import time
import traceback

# No __pycache__ next to the sources.
sys.dont_write_bytecode = True
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import tclient  # noqa: E402
import scenarios  # noqa: E402

# Test accounts, created per run by trace_check.py (querymanager-seed
# --new-character); the characters have no player file.
ACCOUNTS = {
	"a": (900001, "testa", "Testa Alpha"),
	"b": (900002, "testb", "Testb Beta"),
}


class Context:
	def __init__(self, args, types, clock, outdir, rsa_n):
		self.args = args
		self.types = types
		self.clock = clock
		self.outdir = outdir
		self.rsa_n = rsa_n
		self.clients = {}
		self.logs = {}
		self.base = 0
		self.failures = []

	def client(self, key):
		if key not in self.clients:
			f = open(os.path.join(self.outdir, "%s.jsonl" % key), "w")
			self.logs[key] = f
			self.clients[key] = tclient.Client(key, self.types, self.clock, self.rsa_n, self.args.host, self.args.port, f)
		return self.clients[key]

	def login(self, key, timeout=10):
		c = self.client(key)
		acc, pw, name = ACCOUNTS[key]
		m = c.mark()
		c.login(acc, pw, name)
		rec = c.wait_text(r"^(InitGame|LoginError|LoginWait)", m, timeout)
		if rec is None or not rec["text"].startswith("InitGame"):
			raise RuntimeError("%s: login failed: %r" % (key, rec and rec["text"]))
		c.wait_text(r"^FullScreen", m, timeout)
		return c

	def at(self, n):
		"""Waits until server round base+n has begun."""
		target = self.base + n
		if not self.clock.wait(target):
			raise RuntimeError("round %d did not come" % target)
		for c in self.clients.values():
			c.note("--- round %d (step %d)" % (target, n))

	def check(self, cond, what):
		if not cond:
			self.failures.append(what)
			print("CHECK FAILED:", what, flush=True)


def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("--scenario")
	ap.add_argument("--out")
	ap.add_argument("--rsa-key")
	ap.add_argument("--objects", default="dat/items.toml")
	ap.add_argument("--console")
	ap.add_argument("--host", default="127.0.0.1")
	ap.add_argument("--port", type=int, default=7171)
	ap.add_argument("--start-round", type=int, default=0)
	ap.add_argument("--list", action="store_true")
	args = ap.parse_args()
	if args.list:
		for name, f in sorted(scenarios.SCENARIOS.items()):
			print("%-12s %s" % (name, (f.__doc__ or "").strip().splitlines()[0]))
		return 0
	if not args.scenario or not args.out or not args.rsa_key:
		ap.error("--scenario, --out and --rsa-key are needed")
	f = scenarios.SCENARIOS[args.scenario]
	os.makedirs(args.out, exist_ok=True)
	types = tclient.ObjectTypes(args.objects)
	clock = tclient.RoundClock(args.console)
	ctx = Context(args, types, clock, args.out, tclient.rsa_modulus(args.rsa_key))
	if args.console:
		# Start at a fixed round so that both servers see the same actions in
		# the same rounds.
		start = max(args.start_round, getattr(f, "start_round", 20))
		if clock.now() >= start:
			print("server is already at round %d, past the start round %d" % (clock.now(), start), flush=True)
			return 2
		ctx.base = start
	else:
		ctx.base = 0
	print("scenario %s, base round %d, server round now %d" % (args.scenario, ctx.base, clock.now()), flush=True)
	status = 0
	try:
		f(ctx)
	except Exception:
		traceback.print_exc()
		status = 1
	finally:
		for c in ctx.clients.values():
			if not c.closed:
				c.close()
		time.sleep(0.5)
		for log in ctx.logs.values():
			log.close()
	n = sum(len(c.records) for c in ctx.clients.values())
	print("done: status %d, %d records, %d check failures, last round %d" % (status, n, len(ctx.failures), clock.now()), flush=True)
	return status or (3 if ctx.failures else 0)


if __name__ == "__main__":
	sys.exit(main())
