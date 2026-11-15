#!/usr/bin/env python3
"""Headless game-protocol client for the game server (the 2006 bin/game and
this rebuild). Python 3 standard library only.

Connects to the game port directly (no login server), logs in a character,
sends client commands and records every decrypted server->client message,
decoded into readable text, with its raw bytes.

Protocol source: the server side, src/communication.cc (login, framing),
src/sending.cc (every server message), src/receiving.cc (every command the
server accepts). tools/trace_check.py runs it through run_scenario.py.
"""
import base64
import json
import os
import re
import socket
import struct
import threading
import time

# The login block is raw RSA (no padding) with the server's key: the modulus
# of its key file (key.pem) or P*Q of an RSA_PrivateKey.hh, which rsa_modulus
# reads; exponent 65537.
RSA_E = 65537
MASK = 0xFFFFFFFF
DELTA = 0x9E3779B9

TALK_MODES = {
	1: "say", 2: "whisper", 3: "yell", 4: "private", 5: "channel", 6: "gm-request", 7: "gm-answer",
	8: "player-answer", 9: "gm-broadcast", 10: "gm-channel", 11: "gm-message", 12: "highlight-channel",
	13: "anon-broadcast", 14: "anon-channel", 15: "anon-message", 16: "animal-low", 17: "animal-loud",
	18: "admin", 19: "event", 20: "login", 21: "status", 22: "info", 23: "failure",
}
DIRECTIONS = {0: "N", 1: "E", 2: "S", 3: "W"}


# ---------------------------------------------------------------- crypto

def xtea_encrypt(data, key):
	k = struct.unpack("<4I", key)
	out = bytearray()
	for i in range(0, len(data), 8):
		v0, v1 = struct.unpack_from("<2I", data, i)
		s = 0
		for _ in range(32):
			v0 = (v0 + ((((v1 << 4) ^ (v1 >> 5)) + v1) ^ (s + k[s & 3]))) & MASK
			s = (s + DELTA) & MASK
			v1 = (v1 + ((((v0 << 4) ^ (v0 >> 5)) + v0) ^ (s + k[(s >> 11) & 3]))) & MASK
		out += struct.pack("<2I", v0, v1)
	return bytes(out)


def xtea_decrypt(data, key):
	k = struct.unpack("<4I", key)
	out = bytearray()
	for i in range(0, len(data), 8):
		v0, v1 = struct.unpack_from("<2I", data, i)
		s = (DELTA * 32) & MASK
		for _ in range(32):
			v1 = (v1 - ((((v0 << 4) ^ (v0 >> 5)) + v0) ^ (s + k[(s >> 11) & 3]))) & MASK
			s = (s - DELTA) & MASK
			v0 = (v0 - ((((v1 << 4) ^ (v1 >> 5)) + v1) ^ (s + k[s & 3]))) & MASK
		out += struct.pack("<2I", v0, v1)
	return bytes(out)


def rsa_primes(header):
	"""The primes (P, Q) of an RSA_PrivateKey.hh (RSA_PRIME_P and RSA_PRIME_Q
	as decimal strings, the format of the original and of the old rsakeygen)."""
	with open(header, encoding="latin-1") as f:
		text = f.read()
	primes = []
	for name in ("RSA_PRIME_P", "RSA_PRIME_Q"):
		m = re.search(name + r'\s*\[\s*\d*\s*\]\s*=\s*"(\d+)"', text)
		if m is None:
			raise ValueError("%s: no %s" % (header, name))
		primes.append(int(m.group(1)))
	return primes[0], primes[1]


def der_element(data, pos, tag):
	"""(content start, content end) of the DER element with the tag at pos."""
	if pos + 2 > len(data) or data[pos] != tag:
		raise ValueError("not the DER element 0x%02x" % tag)
	length = data[pos + 1]
	pos += 2
	if length & 0x80:
		count = length & 0x7F
		length = int.from_bytes(data[pos:pos + count], "big")
		pos += count
	if pos + length > len(data):
		raise ValueError("DER element runs past the end")
	return pos, pos + length


def pem_modulus(path):
	"""The modulus of an RSA private key file in PEM, PKCS#1 ("RSA PRIVATE
	KEY") or PKCS#8 ("PRIVATE KEY")."""
	with open(path, encoding="ascii") as f:
		lines = [line.strip() for line in f if line.strip()]
	label = lines[0][len("-----BEGIN "):-len("-----")]
	if label not in ("RSA PRIVATE KEY", "PRIVATE KEY") or lines[-1] != "-----END %s-----" % label:
		raise ValueError("%s: not an RSA private key in PEM" % path)
	data = base64.b64decode("".join(lines[1:-1]), validate=True)
	start, end = der_element(data, 0, 0x30)
	if label == "PRIVATE KEY":
		_, pos = der_element(data, start, 0x02)        # version
		_, pos = der_element(data, pos, 0x30)          # algorithm
		start, end = der_element(data, pos, 0x04)      # the PKCS#1 key
		start, end = der_element(data, start, 0x30)
	_, pos = der_element(data, start, 0x02)            # version
	start, end = der_element(data, pos, 0x02)
	return int.from_bytes(data[start:end], "big")


def rsa_modulus(path):
	"""The modulus of the server's key: a key file (PEM) or an
	RSA_PrivateKey.hh."""
	with open(path, "rb") as f:
		pem = f.read(64).lstrip().startswith(b"-----BEGIN ")
	if pem:
		return pem_modulus(path)
	p, q = rsa_primes(path)
	return p * q


def tstr(s):
	b = s.encode("latin-1")
	return struct.pack("<H", len(b)) + b


# ---------------------------------------------------------------- object types

def flag_key(name):
	"""A flag as the client keeps it: lower case without underscores, so that
	objects.srv's "LiquidContainer" and items.toml's "liquid_container" are
	the same."""
	return name.replace("_", "").lower()


class ObjectTypes:
	"""Reads the object types from items.toml (the game's data/items.toml) or,
	for a path that does not end in .toml, from the original objects.srv:
	names and the flags that decide whether the server writes an extra byte
	after a type id (SendMapObject and the inventory/container senders:
	LiquidContainer, LiquidPool, Cumulative)."""

	def __init__(self, path):
		self.names = {}
		self.flags = {}
		if path.endswith(".toml"):
			import tomllib
			with open(path, "rb") as f:
				for item in tomllib.load(f).get("item", []):
					tid = item["id"]
					if "name" in item:
						self.names[tid] = item["name"]
					self.flags[tid] = set(flag_key(x) for x in item.get("flags", []))
			return
		tid = None
		with open(path, "rb") as f:
			for raw in f:
				line = raw.decode("latin-1").strip()
				m = re.match(r"TypeID\s*=\s*(\d+)", line)
				if m:
					tid = int(m.group(1))
					self.flags[tid] = set()
					continue
				if tid is None:
					continue
				m = re.match(r'Name\s*=\s*"(.*)"', line)
				if m:
					self.names[tid] = m.group(1)
					continue
				m = re.match(r"Flags\s*=\s*\{(.*)\}", line)
				if m:
					self.flags[tid] = set(flag_key(x.strip()) for x in m.group(1).split(",") if x.strip())

	def extra(self, tid):
		f = self.flags.get(tid, ())
		return int("liquidcontainer" in f) + int("liquidpool" in f) + int("cumulative" in f)

	def has(self, tid, flag):
		return flag_key(flag) in self.flags.get(tid, ())

	def name(self, tid):
		return self.names.get(tid, "?")


# ---------------------------------------------------------------- decoding

class DecodeError(Exception):
	pass


class Reader:
	def __init__(self, data):
		self.d = data
		self.p = 0

	def left(self):
		return len(self.d) - self.p

	def u8(self):
		if self.p + 1 > len(self.d):
			raise DecodeError("end of data")
		v = self.d[self.p]
		self.p += 1
		return v

	def u16(self):
		if self.p + 2 > len(self.d):
			raise DecodeError("end of data")
		v = struct.unpack_from("<H", self.d, self.p)[0]
		self.p += 2
		return v

	def peek16(self):
		if self.p + 2 > len(self.d):
			raise DecodeError("end of data")
		return struct.unpack_from("<H", self.d, self.p)[0]

	def u32(self):
		if self.p + 4 > len(self.d):
			raise DecodeError("end of data")
		v = struct.unpack_from("<I", self.d, self.p)[0]
		self.p += 4
		return v

	def raw(self, n):
		if self.p + n > len(self.d):
			raise DecodeError("end of data")
		v = self.d[self.p:self.p + n]
		self.p += n
		return v

	def string(self):
		n = self.u16()
		return self.raw(n).decode("latin-1")

	def pos(self):
		x = self.u16()
		y = self.u16()
		z = self.u8()
		return (x, y, z)


def fmt_pos(p):
	return "[%d,%d,%d]" % p


class Decoder:
	"""Splits a decrypted server packet into messages and decodes each one.
	Keeps the state a client needs for that: its own position (map
	descriptions are relative to it) and its creature id."""

	def __init__(self, types):
		self.types = types
		self.pos = None
		self.own_id = None
		self.tiles = {}          # position -> list of things, from map descriptions only

	# --- things
	def item(self, r, with_name=True):
		tid = r.u16()
		extra = []
		for _ in range(self.types.extra(tid)):
			extra.append(r.u8())
		s = "%d" % tid
		if with_name:
			s += "(%s)" % self.types.name(tid)
		if extra:
			s += "x" + ",".join(str(e) for e in extra)
		return s, tid, extra

	def creature_tail(self, r):
		hp = r.u8()
		d = r.u8()
		outfit = r.u16()
		if outfit == 0:
			look = "obj%d" % r.u16()
		else:
			look = "-".join(str(c) for c in r.raw(4))
		light = r.u8()
		color = r.u8()
		speed = r.u16()
		skull = r.u8()
		party = r.u8()
		return "hp=%d dir=%s outfit=%d/%s light=%d/%d speed=%d skull=%d party=%d" % (
			hp, DIRECTIONS.get(d, d), outfit, look, light, color, speed, skull, party)

	def thing(self, r, with_name=False):
		"""One object of SendMapObject. Returns (text, kind, value)."""
		w = r.peek16()
		if w == 0x61:
			r.u16()
			remove = r.u32()
			cid = r.u32()
			name = r.string()
			return "cr-new(id=%d remove=%d name=%r %s)" % (cid, remove, name, self.creature_tail(r)), "cr", cid
		if w == 0x62:
			r.u16()
			cid = r.u32()
			return "cr-upd(id=%d %s)" % (cid, self.creature_tail(r)), "cr", cid
		if w == 0x63:
			r.u16()
			cid = r.u32()
			d = r.u8()
			return "cr-known(id=%d dir=%s)" % (cid, DIRECTIONS.get(d, d)), "cr", cid
		s, tid, extra = self.item(r, with_name)
		return s, "it", (tid, extra)

	def map_description(self, r, positions):
		"""Reads the tiles of SendMapPoint/SkipFlush in the order given;
		returns the lines for the non-empty tiles."""
		lines = []
		skip = 0
		for p in positions:
			if skip > 0:
				skip -= 1
				self.tiles[p] = []
				continue
			things = []
			texts = []
			while True:
				w = r.peek16()
				if w >= 0xFF00:
					r.u16()
					skip = w & 0xFF
					break
				t, kind, val = self.thing(r)
				texts.append(t)
				things.append((kind, val))
			self.tiles[p] = things
			if texts:
				lines.append("  %s %s" % (fmt_pos(p), " ".join(texts)))
		return lines

	def floor_positions(self, z, x0, y0, xs, ys):
		px, py, pz = self.pos
		out = []
		for x in xs:
			for y in ys:
				out.append((x0 + x + (pz - z), y0 + y + (pz - z), z))
		return out

	def screen_floors(self):
		pz = self.pos[2]
		if pz <= 7:
			return list(range(7, -1, -1))
		return [z for z in range(pz - 2, pz + 3) if z <= 15]

	def row_floors(self):
		pz = self.pos[2]
		if pz <= 7:
			return list(range(7, -1, -1))
		return list(range(pz - 2, min(pz + 3, 16)))

	# --- one message
	def message(self, r):
		"""Decodes the message at the reader's position. Returns
		(opcode, text). Raises DecodeError if it cannot be read."""
		op = r.u8()
		f = getattr(self, "op_%02x" % op, None)
		if f is None:
			raise DecodeError("unknown opcode 0x%02x" % op)
		return op, f(r)

	def op_0a(self, r):
		cid = r.u32()
		beat = r.u16()
		bug = r.u8()
		self.own_id = cid
		return "InitGame id=%d beat=%d bugreports=%d" % (cid, beat, bug)

	def op_0b(self, r):
		return "Rights " + r.raw(32).hex()

	def op_14(self, r):
		return "LoginError %r" % r.string()

	def op_15(self, r):
		return "LoginInfo %r" % r.string()

	def op_16(self, r):
		s = r.string()
		return "LoginWait %r wait=%d" % (s, r.u8())

	def op_1e(self, r):
		return "Ping"

	def op_64(self, r):
		self.pos = r.pos()
		px, py, pz = self.pos
		x0, y0 = px - 8, py - 6
		pos = []
		for z in self.screen_floors():
			pos += self.floor_positions(z, x0, y0, range(18), range(14))
		lines = self.map_description(r, pos)
		return "\n".join(["FullScreen pos=%s tiles=%d" % (fmt_pos(self.pos), len(lines))] + lines)

	def _row(self, r, name, dx, dy, which):
		px, py, pz = self.pos
		self.pos = (px + dx, py + dy, pz)
		px, py, pz = self.pos
		x0, y0 = px - 8, py - 6
		pos = []
		for z in self.row_floors():
			if which == 0:
				pos += self.floor_positions(z, x0, y0, range(18), [0])
			elif which == 1:
				pos += self.floor_positions(z, x0, y0, [17], range(14))
			elif which == 2:
				pos += self.floor_positions(z, x0, y0, range(18), [13])
			else:
				pos += self.floor_positions(z, x0, y0, [0], range(14))
		lines = self.map_description(r, pos)
		return "\n".join(["Row%s newpos=%s tiles=%d" % (name, fmt_pos(self.pos), len(lines))] + lines)

	def op_65(self, r):
		return self._row(r, "North", 0, -1, 0)

	def op_66(self, r):
		return self._row(r, "East", 1, 0, 1)

	def op_67(self, r):
		return self._row(r, "South", 0, 1, 2)

	def op_68(self, r):
		return self._row(r, "West", -1, 0, 3)

	def _floors(self, r, up):
		px, py, pz = self.pos
		if up:
			self.pos = (px + 1, py + 1, pz - 1)
		else:
			self.pos = (px - 1, py - 1, pz + 1)
		px, py, pz = self.pos
		if up:
			if pz > 7:
				zs = [pz - 2]
			elif pz == 7:
				zs = list(range(5, -1, -1))
			else:
				zs = []
		else:
			if pz > 8:
				zs = [z for z in [pz + 2] if z < 16]
			elif pz == 8:
				zs = [8, 9, 10]
			else:
				zs = []
		x0, y0 = px - 8, py - 6
		pos = []
		for z in zs:
			pos += self.floor_positions(z, x0, y0, range(18), range(14))
		lines = self.map_description(r, pos) if pos else []
		if not pos:
			# SkipFlush with Skip == -1 writes nothing.
			pass
		return "\n".join(["Floor%s newpos=%s tiles=%d" % ("Up" if up else "Down", fmt_pos(self.pos), len(lines))] + lines)

	def op_be(self, r):
		return self._floors(r, True)

	def op_bf(self, r):
		return self._floors(r, False)

	def op_69(self, r):
		p = r.pos()
		lines = self.map_description(r, [p])
		return "FieldData %s %s" % (fmt_pos(p), lines[0].split(" ", 3)[-1] if lines else "(empty)")

	def op_6a(self, r):
		p = r.pos()
		t, _, _ = self.thing(r, True)
		return "AddField %s %s" % (fmt_pos(p), t)

	def op_6b(self, r):
		p = r.pos()
		rn = r.u8()
		t, _, _ = self.thing(r, True)
		return "ChangeField %s rnum=%d %s" % (fmt_pos(p), rn, t)

	def op_6c(self, r):
		p = r.pos()
		return "DeleteField %s rnum=%d" % (fmt_pos(p), r.u8())

	def op_6d(self, r):
		a = r.pos()
		rn = r.u8()
		b = r.pos()
		return "MoveCreature %s rnum=%d -> %s" % (fmt_pos(a), rn, fmt_pos(b))

	def op_6e(self, r):
		nr = r.u8()
		tid = r.u16()
		name = r.string()
		cap = r.u8()
		up = r.u8()
		n = r.u8()
		items = [self.item(r)[0] for _ in range(n)]
		return "Container nr=%d type=%d name=%r cap=%d hasparent=%d items=[%s]" % (nr, tid, name, cap, up, " ".join(items))

	def op_6f(self, r):
		return "CloseContainer nr=%d" % r.u8()

	def op_70(self, r):
		nr = r.u8()
		return "CreateInContainer nr=%d %s" % (nr, self.item(r)[0])

	def op_71(self, r):
		nr = r.u8()
		rn = r.u8()
		return "ChangeInContainer nr=%d rnum=%d %s" % (nr, rn, self.item(r)[0])

	def op_72(self, r):
		nr = r.u8()
		return "DeleteInContainer nr=%d rnum=%d" % (nr, r.u8())

	def op_78(self, r):
		slot = r.u8()
		return "SetInventory slot=%d %s" % (slot, self.item(r)[0])

	def op_79(self, r):
		return "DeleteInventory slot=%d" % r.u8()

	def _trade(self, r, own):
		name = r.string()
		n = r.u8()
		items = [self.item(r)[0] for _ in range(n)]
		return "TradeOffer%s name=%r items=[%s]" % ("Own" if own else "Partner", name, " ".join(items))

	def op_7d(self, r):
		return self._trade(r, True)

	def op_7e(self, r):
		return self._trade(r, False)

	def op_7f(self, r):
		return "CloseTrade"

	def op_82(self, r):
		b = r.u8()
		return "Ambiente brightness=%d color=%d" % (b, r.u8())

	def op_83(self, r):
		p = r.pos()
		return "GraphicalEffect %s type=%d" % (fmt_pos(p), r.u8())

	def op_84(self, r):
		p = r.pos()
		c = r.u8()
		return "TextualEffect %s color=%d %r" % (fmt_pos(p), c, r.string())

	def op_85(self, r):
		a = r.pos()
		b = r.pos()
		return "MissileEffect %s -> %s type=%d" % (fmt_pos(a), fmt_pos(b), r.u8())

	def op_86(self, r):
		cid = r.u32()
		return "MarkCreature id=%d color=%d" % (cid, r.u8())

	def op_8c(self, r):
		cid = r.u32()
		return "CreatureHealth id=%d hp=%d" % (cid, r.u8())

	def op_8d(self, r):
		cid = r.u32()
		b = r.u8()
		return "CreatureLight id=%d light=%d/%d" % (cid, b, r.u8())

	def op_8e(self, r):
		cid = r.u32()
		o = r.u16()
		look = ("obj%d" % r.u16()) if o == 0 else "-".join(str(c) for c in r.raw(4))
		return "CreatureOutfit id=%d outfit=%d/%s" % (cid, o, look)

	def op_8f(self, r):
		cid = r.u32()
		return "CreatureSpeed id=%d speed=%d" % (cid, r.u16())

	def op_90(self, r):
		cid = r.u32()
		return "CreatureSkull id=%d skull=%d" % (cid, r.u8())

	def op_91(self, r):
		cid = r.u32()
		return "CreatureParty id=%d party=%d" % (cid, r.u8())

	def op_96(self, r):
		oid = r.u32()
		tid = r.u16()
		maxlen = r.u16()
		text = r.string()
		editor = r.string()
		return "EditText obj=%d type=%d maxlen=%d text=%r editor=%r" % (oid, tid, maxlen, text, editor)

	def op_97(self, r):
		t = r.u8()
		i = r.u32()
		return "EditList type=%d id=%d text=%r" % (t, i, r.string())

	def op_a0(self, r):
		hp = r.u16()
		maxhp = r.u16()
		cap = r.u16()
		exp = r.u32()
		lvl = r.u16()
		lp = r.u8()
		mana = r.u16()
		maxmana = r.u16()
		ml = r.u8()
		mp = r.u8()
		soul = r.u8()
		return "PlayerData hp=%d/%d cap=%d exp=%d level=%d(%d%%) mana=%d/%d mlevel=%d(%d%%) soul=%d" % (
			hp, maxhp, cap, exp, lvl, lp, mana, maxmana, ml, mp, soul)

	def op_a1(self, r):
		names = ["fist", "club", "sword", "axe", "dist", "shield", "fishing"]
		parts = []
		for n in names:
			v = r.u8()
			p = r.u8()
			parts.append("%s=%d(%d%%)" % (n, v, p))
		return "PlayerSkills " + " ".join(parts)

	def op_a2(self, r):
		return "PlayerState 0x%02x" % r.u8()

	def op_a3(self, r):
		return "ClearTarget"

	def op_aa(self, r):
		sid = r.u32()
		sender = r.string()
		mode = r.u8()
		m = TALK_MODES.get(mode, str(mode))
		if mode in (1, 2, 3, 16, 17):
			p = r.pos()
			return "Talk stmt=%d %r %s %s %r" % (sid, sender, m, fmt_pos(p), r.string())
		if mode in (5, 10, 12, 14):
			ch = r.u16()
			return "Talk stmt=%d %r %s channel=%d %r" % (sid, sender, m, ch, r.string())
		if mode == 6:
			data = r.u32()
			return "Talk stmt=%d %r %s data=%d %r" % (sid, sender, m, data, r.string())
		return "Talk stmt=%d %r %s %r" % (sid, sender, m, r.string())

	def op_ab(self, r):
		n = r.u8()
		chans = []
		for _ in range(n):
			i = r.u16()
			chans.append("%d:%r" % (i, r.string()))
		return "Channels " + " ".join(chans)

	def op_ac(self, r):
		i = r.u16()
		return "OpenChannel %d %r" % (i, r.string())

	def op_ad(self, r):
		return "PrivateChannel %r" % r.string()

	def op_ae(self, r):
		return "OpenRequestQueue %d" % r.u16()

	def op_af(self, r):
		return "DeleteRequest %r" % r.string()

	def op_b0(self, r):
		return "FinishRequest %r" % r.string()

	def op_b1(self, r):
		return "CloseRequest"

	def op_b2(self, r):
		i = r.u16()
		return "OpenOwnChannel %d %r" % (i, r.string())

	def op_b3(self, r):
		return "CloseChannel %d" % r.u16()

	def op_b4(self, r):
		mode = r.u8()
		return "Message %s %r" % (TALK_MODES.get(mode, str(mode)), r.string())

	def op_b5(self, r):
		d = r.u8()
		return "Snapback dir=%s" % DIRECTIONS.get(d, d)

	def op_c8(self, r):
		o = r.u16()
		c = "-".join(str(x) for x in r.raw(4))
		a = r.u16()
		return "OutfitDialog outfit=%d/%s range=%d..%d" % (o, c, a, r.u16())

	def op_d2(self, r):
		cid = r.u32()
		name = r.string()
		return "BuddyData id=%d %r online=%d" % (cid, name, r.u8())

	def op_d3(self, r):
		return "BuddyOnline id=%d" % r.u32()

	def op_d4(self, r):
		return "BuddyOffline id=%d" % r.u32()


# ---------------------------------------------------------------- rounds

class RoundClock:
	"""Round numbers of the server. With a console file (trace_check.py runs
	the server under gdb and prints @@ROUND n@@ at every SetRoundNr), the clock
	follows the server; otherwise it counts seconds from its start."""

	def __init__(self, console=None, start_round=0):
		self.console = console
		self.round = start_round
		self.t0 = time.time()
		self.cv = threading.Condition()
		if console:
			threading.Thread(target=self._tail, daemon=True).start()

	def _tail(self):
		rx = re.compile(rb"@@ROUND (\d+)@@")
		pos = 0
		buf = b""
		while True:
			try:
				with open(self.console, "rb") as f:
					f.seek(pos)
					chunk = f.read()
			except OSError:
				chunk = b""
			if chunk:
				pos += len(chunk)
				buf += chunk
				last = None
				for m in rx.finditer(buf):
					last = int(m.group(1))
				cut = buf.rfind(b"@@ROUND")
				buf = buf[cut:] if cut >= 0 and not buf.endswith(b"@@") else b""
				if last is not None:
					with self.cv:
						if last > self.round:
							self.round = last
						self.cv.notify_all()
			time.sleep(0.01)

	def now(self):
		if self.console:
			return self.round
		return int(time.time() - self.t0)

	def wait(self, n, timeout=120):
		if not self.console:
			d = self.t0 + n - time.time()
			if d > 0:
				time.sleep(d)
			return True
		end = time.time() + timeout
		with self.cv:
			while self.round < n:
				left = end - time.time()
				if left <= 0:
					return False
				self.cv.wait(left)
		return True


# ---------------------------------------------------------------- client

class Client:
	def __init__(self, label, types, clock, rsa_n, host="127.0.0.1", port=7171, log=None):
		self.label = label
		self.rsa_n = rsa_n
		self.types = types
		self.clock = clock
		self.host = host
		self.port = port
		self.records = []
		self.cv = threading.Condition()
		self.sock = None
		self.key = None
		self.dec = Decoder(types)
		self.t0 = time.time()
		self.log = log
		self.closed = True

	# --- recording
	def record(self, direction, op, text, raw):
		rec = {"t": round(time.time() - self.t0, 3), "round": self.clock.now(), "dir": direction,
			"op": "0x%02x" % op if op is not None else None, "text": text, "hex": raw.hex()}
		with self.cv:
			self.records.append(rec)
			self.cv.notify_all()
		if self.log:
			self.log.write(json.dumps(rec) + "\n")
			self.log.flush()

	def note(self, text):
		self.record("N", None, text, b"")

	def mark(self):
		with self.cv:
			return len(self.records)

	def wait_for(self, pred, start=0, timeout=5.0):
		end = time.time() + timeout
		with self.cv:
			while True:
				for rec in self.records[start:]:
					if rec["dir"] == "S" and pred(rec):
						return rec
				left = end - time.time()
				if left <= 0:
					return None
				self.cv.wait(left)

	def wait_text(self, pattern, start=0, timeout=5.0):
		rx = re.compile(pattern)
		return self.wait_for(lambda r: rx.search(r["text"]) is not None, start, timeout)

	# --- connection
	def login(self, account, password, name, ttype=2, version=772, gm=0):
		self.t0 = time.time() if not self.records else self.t0
		self.key = os.urandom(16)
		self.dec = Decoder(self.types)
		self.sock = socket.create_connection((self.host, self.port), timeout=10)
		self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
		plain = bytes([0]) + self.key + struct.pack("<HHBI", ttype, version, gm, account) + tstr(name) + tstr(password)
		plain += os.urandom(128 - len(plain))
		plain = bytes([0]) + plain[1:]
		block = pow(int.from_bytes(plain, "big"), RSA_E, self.rsa_n).to_bytes(128, "big")
		body = bytes([0x0A]) + block
		self.sock.sendall(struct.pack("<H", len(body)) + body)
		self.closed = False
		self.record("C", 0x0A, "Login account=%d name=%r type=%d version=%d" % (account, name, ttype, version), b"")
		threading.Thread(target=self._reader, daemon=True).start()

	def _recv_exact(self, n):
		buf = b""
		while len(buf) < n:
			chunk = self.sock.recv(n - len(buf))
			if not chunk:
				return None
			buf += chunk
		return buf

	def _reader(self):
		self.sock.settimeout(None)
		try:
			while True:
				head = self._recv_exact(2)
				if head is None:
					break
				n = struct.unpack("<H", head)[0]
				body = self._recv_exact(n)
				if body is None:
					break
				if n % 8 != 0:
					self.record("S", None, "BadPacket outer length %d" % n, body)
					continue
				plain = xtea_decrypt(body, self.key)
				inner = struct.unpack_from("<H", plain)[0]
				data = plain[2:2 + inner]
				self._decode_packet(data)
		except OSError as e:
			self.note("socket error %s" % e)
		self.closed = True
		self.note("connection closed")
		with self.cv:
			self.cv.notify_all()

	def _decode_packet(self, data):
		r = Reader(data)
		while r.left() > 0:
			start = r.p
			try:
				op, text = self.dec.message(r)
			except DecodeError as e:
				self.record("S", data[start], "UNDECODED (%s)" % e, data[start:])
				return
			self.record("S", op, text, data[start:r.p])

	def send(self, data, text):
		inner = struct.pack("<H", len(data)) + data
		while len(inner) % 8:
			inner += b"\x00"
		self.record("C", data[0], text, data)
		try:
			self.sock.sendall(struct.pack("<H", len(inner)) + xtea_encrypt(inner, self.key))
		except OSError as e:
			self.note("send failed: %s" % e)

	def close(self):
		try:
			self.sock.close()
		except OSError:
			pass

	def wait_closed(self, timeout=10):
		end = time.time() + timeout
		while not self.closed and time.time() < end:
			time.sleep(0.05)
		return self.closed

	# --- state
	@property
	def id(self):
		return self.dec.own_id

	@property
	def pos(self):
		return self.dec.pos

	def type_at(self, p):
		"""The type of the first object (the ground) the map descriptions
		showed on the field p, or None if they did not show it. Objects and
		creatures that came later (AddField, MoveCreature) are not kept."""
		for kind, val in self.dec.tiles.get(tuple(p), []):
			if kind == "it":
				return val[0]
		return None

	def wait_creature(self, name, start=0, timeout=5.0):
		"""The id of the first creature of this name the server showed as a
		new one (cr-new) after record start, or None."""
		rx = re.compile(r"cr-new\(id=(\d+) remove=\d+ name=%s " % re.escape(repr(name)))
		rec = self.wait_text(rx.pattern, start, timeout)
		return int(rx.search(rec["text"]).group(1)) if rec else None

	# --- commands (src/receiving.cc ReceiveData)
	def logout(self):
		self.send(b"\x14", "Logout")

	def ping(self):
		self.send(b"\x1e", "Ping")

	WALK = {"n": 0x65, "e": 0x66, "s": 0x67, "w": 0x68, "ne": 0x6A, "se": 0x6B, "sw": 0x6C, "nw": 0x6D}
	PATH = {"e": 1, "ne": 2, "n": 3, "nw": 4, "w": 5, "sw": 6, "s": 7, "se": 8}
	TURN = {"n": 0x6F, "e": 0x70, "s": 0x71, "w": 0x72}

	def walk(self, d):
		self.send(bytes([self.WALK[d]]), "Walk %s" % d)

	def autowalk(self, dirs):
		self.send(bytes([0x64, len(dirs)] + [self.PATH[d] for d in dirs]), "AutoWalk %s" % ",".join(dirs))

	def stop(self):
		self.send(b"\x69", "Stop")

	def turn(self, d):
		self.send(bytes([self.TURN[d]]), "Turn %s" % d)

	def look(self, x, y, z):
		self.send(struct.pack("<BHHB", 0x8C, x, y, z), "Look %s" % fmt_pos((x, y, z)))

	def refresh_field(self, x, y, z):
		self.send(struct.pack("<BHHB", 0xC9, x, y, z), "RefreshField %s" % fmt_pos((x, y, z)))

	def move(self, src, tid, rnum, dst, count):
		self.send(struct.pack("<BHHBHBHHBB", 0x78, src[0], src[1], src[2], tid, rnum, dst[0], dst[1], dst[2], count),
			"Move %s type=%d rnum=%d -> %s count=%d" % (fmt_pos(src), tid, rnum, fmt_pos(dst), count))

	def use(self, p, tid, rnum, dummy=0):
		self.send(struct.pack("<BHHBHBB", 0x82, p[0], p[1], p[2], tid, rnum, dummy),
			"Use %s type=%d rnum=%d dummy=%d" % (fmt_pos(p), tid, rnum, dummy))

	def use_with(self, p, tid, rnum, p2, tid2, rnum2):
		"""Uses an object with another one or a field (a rune, a key, a
		tool): the second object is the first one of type tid2 on a field, or
		the object at an inventory or container position. A creature on a field
		is type 99."""
		self.send(struct.pack("<BHHBHBHHBHB", 0x83, p[0], p[1], p[2], tid, rnum, p2[0], p2[1], p2[2], tid2, rnum2),
			"UseWith %s type=%d rnum=%d -> %s type=%d rnum=%d" % (fmt_pos(p), tid, rnum, fmt_pos(p2), tid2, rnum2))

	def use_on_creature(self, p, tid, rnum, cid):
		"""Uses an object on a creature, given by its id (the server ignores
		this for players)."""
		self.send(struct.pack("<BHHBHBI", 0x84, p[0], p[1], p[2], tid, rnum, cid),
			"UseOnCreature %s type=%d rnum=%d creature=%d" % (fmt_pos(p), tid, rnum, cid))

	def close_container(self, nr):
		self.send(bytes([0x87, nr]), "CloseContainer %d" % nr)

	def trade(self, p, tid, rnum, partner):
		self.send(struct.pack("<BHHBHBI", 0x7D, p[0], p[1], p[2], tid, rnum, partner),
			"Trade %s type=%d rnum=%d partner=%d" % (fmt_pos(p), tid, rnum, partner))

	def inspect_trade(self, side, position):
		self.send(bytes([0x7E, side, position]), "InspectTrade side=%d pos=%d" % (side, position))

	def accept_trade(self):
		self.send(b"\x7f", "AcceptTrade")

	def reject_trade(self):
		self.send(b"\x80", "RejectTrade")

	def talk(self, mode, text, addressee=None, channel=None):
		data = bytes([0x96, mode])
		if mode in (4, 7, 11, 15):
			data += tstr(addressee)
		if mode in (5, 10, 14):
			data += struct.pack("<H", channel)
		data += tstr(text)
		extra = (" to=%r" % addressee if addressee else "") + (" channel=%d" % channel if channel is not None else "")
		self.send(data, "Talk %s%s %r" % (TALK_MODES.get(mode, mode), extra, text))

	def say(self, text):
		self.talk(1, text)

	def whisper(self, text):
		self.talk(2, text)

	def yell(self, text):
		self.talk(3, text)

	def private(self, to, text):
		self.talk(4, text, addressee=to)

	def channel_say(self, channel, text):
		self.talk(5, text, channel=channel)

	def get_channels(self):
		self.send(b"\x97", "GetChannels")

	def join_channel(self, ch):
		self.send(struct.pack("<BH", 0x98, ch), "JoinChannel %d" % ch)

	def leave_channel(self, ch):
		self.send(struct.pack("<BH", 0x99, ch), "LeaveChannel %d" % ch)

	def private_channel(self, name):
		self.send(b"\x9a" + tstr(name), "PrivateChannel %r" % name)

	def open_own_channel(self):
		self.send(b"\xaa", "OpenOwnChannel")

	def tactics(self, attack, chase, secure):
		self.send(bytes([0xA0, attack, chase, secure]), "Tactics attack=%d chase=%d secure=%d" % (attack, chase, secure))

	def attack(self, cid):
		self.send(struct.pack("<BI", 0xA1, cid), "Attack %d" % cid)

	def follow(self, cid):
		self.send(struct.pack("<BI", 0xA2, cid), "Follow %d" % cid)

	def cancel(self):
		self.send(b"\xbe", "Cancel")

	def party_invite(self, cid):
		self.send(struct.pack("<BI", 0xA3, cid), "PartyInvite %d" % cid)

	def party_join(self, cid):
		self.send(struct.pack("<BI", 0xA4, cid), "PartyJoin %d" % cid)

	def party_revoke(self, cid):
		self.send(struct.pack("<BI", 0xA5, cid), "PartyRevoke %d" % cid)

	def party_pass(self, cid):
		self.send(struct.pack("<BI", 0xA6, cid), "PartyPassLeadership %d" % cid)

	def party_leave(self):
		self.send(b"\xa7", "PartyLeave")

	def get_outfit(self):
		self.send(b"\xd2", "GetOutfit")

	def set_outfit(self, outfit, colors):
		self.send(struct.pack("<BH", 0xD3, outfit) + bytes(colors), "SetOutfit %d %s" % (outfit, "-".join(map(str, colors))))

	def add_buddy(self, name):
		self.send(b"\xdc" + tstr(name), "AddBuddy %r" % name)

	def remove_buddy(self, cid):
		self.send(struct.pack("<BI", 0xDD, cid), "RemoveBuddy %d" % cid)

	def raw(self, data, text):
		self.send(data, text)
