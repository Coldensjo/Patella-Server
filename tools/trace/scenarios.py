"""Scenarios for run_scenario.py. Each is a function of the context; ctx.at(n)
waits for step round n (relative to the start round), ctx.login(key) logs a
test client in. tools/trace_check.py runs them (run_scenario.py --list).

The test characters are new (no player file): they start in the Rookgaard
temple, a at [32097,32219,7] and b next to it at [32098,32219,7]. The temple
is no protection zone (its sectors do not flag it); the NPC Cipfried walks
around in it.
"""

import re

SCENARIOS = {}

INV = 0xFFFF            # x of an inventory position: (0xFFFF, slot, 0)
SLOT_BACKPACK = 3
SLOT_LEFT = 6           # the torch
SLOT_RIGHT = 5          # the club
TORCH = 2920
CLUB = 3270
BAG = 2853


def scenario(start_round=20):
	def deco(f):
		f.start_round = start_round
		SCENARIOS[f.__name__] = f
		return f
	return deco


@scenario()
def smoke(ctx):
	"""Login of both new characters, a ping, logout."""
	ctx.at(0)
	a = ctx.login("a")
	ctx.at(3)
	b = ctx.login("b")
	ctx.at(5)
	a.ping()
	ctx.at(7)
	a.logout()
	b.logout()
	ctx.at(9)


class Steps:
	"""Hands out step rounds: s.next(k) waits k rounds after the last step."""

	def __init__(self, ctx):
		self.ctx = ctx
		self.n = 0

	def __call__(self, k=2):
		self.n += k
		self.ctx.at(self.n)
		return self.n


def movement(ctx, s, a, b):
	a.note("=== look")
	a.look(32097, 32219, 7)                 # own tile
	s(1)
	a.look(32098, 32219, 7)                 # b
	b.look(32097, 32219, 7)                 # a
	s(1)
	a.look(32093, 32219, 7)                 # coal basin, 4 fields away
	a.look(32097, 32217, 7)                 # where Cipfried started
	s(1)
	a.look(INV, SLOT_LEFT, 0)               # the torch in the hand
	a.look(INV, 0, 0)                       # invalid inventory position
	s(1)
	a.refresh_field(32096, 32219, 7)
	s(1)

	a.note("=== turn")
	for d in ("n", "e", "s", "w"):
		a.turn(d)
		s(1)

	a.note("=== walk all directions, into obstacles")
	for d, k in (("s", 2), ("s", 2), ("s", 2), ("sw", 3), ("w", 2), ("nw", 3), ("n", 2), ("nw", 3),
			("ne", 3), ("e", 2), ("se", 3), ("ne", 3)):
		a.walk(d)
		s(k)
	a.note("=== walk twice without waiting (the second cancels the first)")
	a.walk("w")
	a.walk("e")
	s(3)

	a.note("=== auto-walk")
	a.autowalk(["w", "w", "n", "e", "e"])
	s(5)
	a.autowalk(["s", "s", "s", "s"])        # ends at the coal basin
	s(5)
	a.autowalk(["n", "n", "n"])
	s(1)
	a.stop()
	s(3)
	a.autowalk([])
	s(1)
	b.note("=== b back to its own start if it was moved")


def social(ctx, s, a, b):
	a.note("=== say, whisper, yell")
	a.say("hello there")
	s(2)
	b.whisper("psst")
	s(2)
	a.yell("HELLO")                         # level 1: refused
	s(2)
	a.note("=== NPC")
	a.say("hi")
	s(3)
	a.say("name")
	s(3)
	a.say("bye")
	s(3)

	a.note("=== channels")
	a.get_channels()
	s(1)
	a.join_channel(4)
	b.join_channel(4)
	s(1)
	a.channel_say(4, "on the game channel")  # level 1: refused
	s(1)
	a.join_channel(3)                        # request queue: not allowed
	a.join_channel(1)                        # gamemaster channel: not allowed
	s(1)
	a.leave_channel(4)
	s(1)
	a.open_own_channel()
	s(1)

	a.note("=== private messages")
	a.private("Testb Beta", "hi b, this is a")
	s(2)
	b.private("Testa Alpha", "hi a")
	s(2)
	a.private("Nobody Here At All", "anyone?")
	s(2)
	a.private_channel("Testb Beta")
	a.private_channel("Testa Alpha")         # with yourself
	a.private_channel("Nobody Here At All")
	s(2)

	a.note("=== spells")
	a.say("exura")
	s(2)
	a.say("utevo lux")
	s(2)
	a.say("exevo vis hur")
	s(2)
	b.say("exura vita")
	s(2)

	a.note("=== outfit")
	a.get_outfit()
	s(1)
	a.set_outfit(129, [10, 20, 30, 40])
	s(2)
	a.set_outfit(134, [1, 2, 3, 4])          # premium outfit, free account: refused
	b.set_outfit(128, [1, 2, 3, 4])          # male outfit for a female character: refused
	s(2)
	b.set_outfit(137, [132, 0, 5, 132])
	s(2)

	a.note("=== buddies")
	a.add_buddy("Testb Beta")
	s(2)
	a.add_buddy("Nobody Here At All")
	s(2)
	a.ping()
	b.ping()
	s(1)


def interaction(ctx, s, a, b):
	a.note("=== fight modes")
	a.tactics(1, 0, 1)
	s(1)
	a.tactics(3, 1, 0)
	s(1)
	a.tactics(4, 0, 0)                       # invalid
	s(1)

	a.note("=== attack and follow b in the protection zone")
	a.attack(b.id)
	s(2)
	a.cancel()
	s(1)
	a.follow(b.id)
	s(2)
	b.walk("e")
	s(3)
	b.walk("w")
	s(3)
	a.cancel()
	s(2)
	a.attack(123456789)                      # no such creature
	s(1)

	a.note("=== party")
	a.party_invite(b.id)
	s(2)
	b.party_join(a.id)
	s(2)
	a.party_pass(b.id)
	s(2)
	a.party_leave()
	s(2)
	b.party_leave()
	s(2)
	a.party_invite(b.id)
	s(2)
	a.party_revoke(b.id)
	s(2)
	b.party_join(a.id)                       # no longer invited
	s(2)
	a.party_leave()                          # not in a party
	s(2)

	a.note("=== containers and moving objects")
	a.use((INV, SLOT_BACKPACK, 0), BAG, 0, 0)
	s(2)
	a.move((INV, SLOT_LEFT, 0), TORCH, 0, (INV, 64, 0), 1)       # torch into the bag
	s(2)
	a.move((INV, 64, 0), TORCH, 0, (INV, SLOT_LEFT, 0), 1)       # and back
	s(2)
	p = a.pos
	a.move((INV, SLOT_LEFT, 0), TORCH, 0, p, 1)                   # torch onto the own field
	s(2)
	a.move(p, TORCH, 1, (INV, SLOT_LEFT, 0), 1)                   # and back into the hand
	s(2)
	a.move((INV, SLOT_RIGHT, 0), CLUB, 0, (INV, 1, 0), 1)         # club onto the head: refused
	s(2)
	a.close_container(0)
	s(1)

	a.note("=== trade")
	a.trade((INV, SLOT_LEFT, 0), TORCH, 0, b.id)
	s(2)
	b.trade((INV, SLOT_LEFT, 0), TORCH, 0, a.id)
	s(2)
	b.inspect_trade(0, 0)
	b.inspect_trade(1, 0)
	s(1)
	a.accept_trade()
	s(1)
	b.accept_trade()
	s(2)
	a.trade((INV, SLOT_RIGHT, 0), CLUB, 0, b.id)
	s(2)
	b.reject_trade()
	s(2)
	a.trade((INV, SLOT_BACKPACK, 0), BAG, 0, b.id)                # a container with its content
	s(2)
	a.reject_trade()
	s(2)

	a.note("=== buddy removal")
	a.remove_buddy(b.id)
	s(2)


@scenario()
def full(ctx):
	"""Two new characters: look, turn, walk, auto-walk, talk, channels,
	private messages, spells, outfit, buddies, fight modes, attack/follow,
	party, containers, moving objects, trade, logout and a second login
	with the saved player files."""
	s = Steps(ctx)
	ctx.at(0)
	a = ctx.login("a")
	s(2)
	b = ctx.login("b")
	s(2)
	movement(ctx, s, a, b)
	social(ctx, s, a, b)
	interaction(ctx, s, a, b)

	a.note("=== buddy goes offline")
	a.add_buddy("Testb Beta")
	s(2)
	b.logout()
	s(3)
	a.logout()
	s(4)

	a.note("=== second login, with the player files written at logout")
	a = ctx.login("a")
	s(2)
	b = ctx.login("b")
	s(2)
	a.say("back again")
	s(2)
	a.walk("s")
	s(2)
	a.remove_buddy(b.id)
	s(1)
	a.logout()
	b.logout()
	s(4)


# The commands (commands.toml), each said by every character of the scenario
# commands: what is said, a note of what it shows. The words given make a
# command answer when the right it checks is there, and change little: no
# player of these names is online, the hands hold the torch and the club, there
# is no mark or raid of that name.
COMMANDS = [
	('alevo "no such thing" 5', "Get Item with a count: CREATE_OBJECTS"),
	('alevo "no such thing"', "Get Item: CREATE_OBJECTS"),
	("alani fast", "Move, speed: MODIFY_GOSTRENGTH"),
	("alani normal", "Move, speed: MODIFY_GOSTRENGTH"),
	("alani 99999,99999,7", "Move to coordinates: TELEPORT_TO_COORDINATE"),
	("alani nosuchmark", "Move to a mark: TELEPORT_TO_MARK"),
	("alani up", "Move up: TELEPORT_VERTICAL"),
	("alani down", "Move down: TELEPORT_VERTICAL"),
	("alevo cogni 1", "Get Experience: CHANGE_SKILLS"),
	("alevo cogni 1 1", "Get Skill Experience: CHANGE_SKILLS"),
	("aleta 5", "Change Data: CREATE_OBJECTS"),
	('alani sio "Nobody Here At All"', "Teleport to Friend: TELEPORT_TO_CHARACTER"),
	('alliber sio "Nobody Here At All"', "Retrieve Friend: RETRIEVE"),
	("alevo res nosuchcreature", "Summon Wild Creature, unknown race: refused before the right"),
	("alevo res rat", "Summon Wild Creature: CREATE_MONSTERS"),
	("aliva tera", "Get Position: SHOW_COORDINATE"),
	('omani "Nobody Here At All"', "Temple Teleport: HOME_TELEPORT"),
	("alevo vis 0", "Create Gold: CREATE_MONEY"),
	("aleta vita male", "Change Profession or Sex: CHANGE_PROFESSION"),
	('omana "Nobody Here At All"', "Kick Player: KICK"),
	("aliva cogni 1", "Get Quest Value: CHANGE_SKILLS"),
	("aleta cogni 1 5", "Set Quest Value: CHANGE_SKILLS"),
	("alana cogni", "Clear Quest Values: CHANGE_SKILLS"),
	("alani hur up", "Levitate Gamemaster: LEVITATE"),
	("alito mas res", "Kill All Creatures: CREATE_MONSTERS"),
	("alevo mas res nosuchraid", "Start Monsterraid: CREATE_MONSTERS"),
	('omana liber "Nobody Here At All" 1 x', "Banish Account: no right"),
	('omana gran liber "Nobody Here At All" x', "Delete Account: no right"),
	('omamo "Nobody Here At All"', "Set Namerule: no right"),
	('omisa "Nobody Here At All" x', "Entry in Criminal Record: no right"),
	('omana hora "Nobody Here At All"', "Namelock: no right"),
	('omana gran res "Nobody Here At All" x', "Delete Character: no right"),
	('omana vis "Nobody Here At All" x', "Banish IP Address: no right"),
	('omana res "Nobody Here At All" 1 x', "Banish Character: no right"),
	("aleta sio", "Invite Guests: no right, not in a house"),
	("aleta som", "Invite Subowners: no right, not in a house"),
	('alana sio "Nobody Here At All"', "Kick Guest: no right"),
	("aleta grav", "Edit Door: no right, no door"),
	("alito tera", "Desintegrate Spell: CLEANUP_FIELDS (blocks the logout)"),
]


@scenario()
def commands(ctx):
	"""The commands of commands.toml, said by a player without rights (a)
	and by five gamemasters (c to g), each with some of the rights the
	commands check (trace_check.py GM_RIGHTS), so that every right check
	answers some of them and not the others."""
	s = Steps(ctx)
	ctx.at(0)
	keys = ["a", "c", "d", "e", "f", "g"]
	who = {}
	for key in keys:
		who[key] = ctx.login(key)
		s(2)
	# Each character in rounds of its own, three commands a round, so that
	# the order in which the game takes them is the same in both runs.
	for i in range(0, len(COMMANDS), 3):
		for key in keys:
			c = who[key]
			for text, what in COMMANDS[i:i + 3]:
				c.note("=== " + what)
				c.say(text)
			s(1)
	s(2)
	for key in keys:
		who[key].logout()
		s(1)
	s(3)


# The scenarios spells and runes: the caster h ('Testh Theta', trace_check.py
# CASTER_RIGHTS: every spell, unlimited mana, premium, attacks everywhere, no
# damage taken, Get Item, monsters, Move to coordinates, Change Profession)
# casts every spell of the classes ex and ut, and makes and uses every rune
# (class ad). An attack spell keeps the caster from casting for 2 seconds, the
# other spells for 1 second (SpendManaAndSoulPoints and its callers), so a cast
# follows the one before by 3 rounds after an attack, by 2 otherwise; a
# refused cast (thrown before the mana is spent) sets no delay. One action of
# h per round: a second one in the same round could cancel the first, which
# the game may not have done yet (TCreature::ToDoAdd).
CREATURE = 99                       # the type of a creature on a field
HAND = (INV, SLOT_RIGHT, 0)
BLANK_RUNE = 3147
STAFF = 3289
# Open grass west of the temple, at least 3 fields free around it, no roof,
# no house, no protection zone: the attack spells, fields and walls.
CASTER_SPOT = (32070, 32213, 7)
# Under a free sky with a walkable floor 6 to the north: Levitate up there and
# down again (facing south); then Wild Growth on the path to the east (on the
# open grass sheep may stand in the way).
LEVITATE_SPOT = (32085, 32216, 7)
# A rope spot in a cave under the town: Magic Rope leads up to 32104,32171,8.
ROPE_SPOT = (32104, 32170, 9)
NOBODY = '"Nobody Here At All"'
ADD_FIELD = re.compile(r"AddField \[(\d+),(\d+),(\d+)\] (\d+)\(")


def free_hands(s, c):
	"""Puts the club and the torch into the bag, so that both hands are free
	(a staff takes both, a blank rune from Get Item lands in the right one)."""
	c.note("=== club and torch into the bag: both hands free")
	c.use((INV, SLOT_BACKPACK, 0), BAG, 0, 0)
	s(1)
	c.move((INV, SLOT_RIGHT, 0), CLUB, 0, (INV, 64, 0), 1)
	s(1)
	c.move((INV, SLOT_LEFT, 0), TORCH, 0, (INV, 64, 0), 1)
	s(1)


def teleport(s, c, p, what):
	c.note("=== Move to %d,%d,%d: %s" % (p + (what,)))
	c.say("alani %d,%d,%d" % p)
	s(2)


def offset(p, dx, dy):
	return (p[0] + dx, p[1] + dy, p[2])


@scenario()
def spells(ctx):
	"""The caster h casts each spell of the classes ex and ut (spells.toml)
	where it does something: the healing, light, haste, illusion, food,
	arrow, shield and invisibility spells, Find Person and Heal Friend on b,
	Mass Healing and Enchant Staff in the temple; Levitate up and down,
	Wild Growth next to it, Magic Rope at a rope spot; the attack spells,
	Challenge, Summon Creature and Undead Legion (on corpses) on open grass
	(a sheep pasture). Refusals: MAGICITEM (no staff), PLAYERNOTONLINE,
	NOTACCESSIBLE (no rope spot, a roof), CREATURENOTEXISTING, EXHAUSTED,
	and SPELLUNKNOWN for b."""
	s = Steps(ctx)
	ctx.at(0)
	h = ctx.login("h")
	s(2)
	b = ctx.login("b")
	s(2)
	free_hands(s, h)
	s(1)

	def cast(text, what, k=2, c=h):
		c.note("=== " + what)
		c.say(text)
		s(k)

	cast("exeta vis", "Enchant Staff without a staff in a hand: MAGICITEM")
	cast("alevo %d" % STAFF, "Get Item: a staff, into the free hands")
	cast("exeta vis", "Enchant Staff: the staff becomes an enchanted staff")
	cast("exura", "Light Healing")
	cast("exura gran", "Intense Healing")
	cast("exura vita", "Ultimate Healing")
	cast("utani hur", "Haste")
	cast("utevo lux", "Light")
	cast("utevo gran lux", "Great Light")
	cast('exiva "Testb Beta"', "Find Person: b is standing next to h")
	cast("exiva %s" % NOBODY, "Find Person, nobody of that name: PLAYERNOTONLINE")
	cast('exura sio "Testb Beta"', "Heal Friend: b")
	cast("exura sio %s" % NOBODY, "Heal Friend, nobody of that name: PLAYERNOTONLINE")
	cast("exana pox", "Antidote")
	cast("utevo res ina rat", "Creature Illusion: a rat")
	cast("utevo res ina nosuchcreature", "Creature Illusion, unknown race: CREATURENOTEXISTING")
	cast("utani gran hur", "Strong Haste")
	cast("exevo pan", "Food")
	cast("utamo vita", "Magic Shield")
	cast("utana vid", "Invisible")
	cast("exevo con pox", "Poisoned Arrow")
	cast("exevo con flam", "Explosive Arrow")
	cast("exevo con", "Conjure Arrow")
	cast("exevo con mort", "Conjure Bolt")
	cast("exevo con vis", "Power Bolt")
	cast("utevo vis lux", "Ultimate Light")
	cast("exura gran mas res", "Mass Healing: h and b")
	cast("exana ina", "Cancel Invisibility (not on the caster's own field)")
	cast("exani tera", "Magic Rope without a rope spot: NOTACCESSIBLE")
	cast("exani hur up", "Levitate up under the temple roof: NOTACCESSIBLE")
	cast("exura vita", "Ultimate Healing said by b, who knows no spell: SPELLUNKNOWN", c=b)

	teleport(s, h, LEVITATE_SPOT, "under a free sky, a floor 6 to the north")
	h.turn("n")
	s(1)
	cast("exani hur up", "Levitate up, to the north")
	h.turn("s")
	s(1)
	cast("exani hur down", "Levitate down, to the south")
	h.turn("e")
	s(1)
	cast("exevo grav vita", "Wild Growth: rush wood on the path to the east", 3)
	teleport(s, h, ROPE_SPOT, "a rope spot")
	cast("exani tera", "Magic Rope: up through the hole")

	teleport(s, h, CASTER_SPOT, "open grass for the attack spells")
	h.turn("n")
	s(1)
	cast("exevo mort hur", "Energy Wave", 3)
	cast("exevo flam hur", "Fire Wave", 1)
	cast("exevo vis lux", "Energy Beam a round after Fire Wave: EXHAUSTED", 3)
	cast("exevo vis lux", "Energy Beam", 3)
	cast("exevo gran vis lux", "Great Energy Beam", 3)
	cast("exevo gran mas vis", "Ultimate Explosion", 3)
	cast("exevo gran mas pox", "Poison Storm", 3)
	cast("exori", "Berserk", 3)
	cast("exori mort", "Force Strike")
	cast("exori vis", "Energy Strike")
	cast("exori flam", "Flame Strike")
	cast("exeta res", "Challenge")
	cast("utevo res nosuchcreature", "Summon Creature, unknown race: CREATURENOTEXISTING")
	cast("utevo res rat", "Summon Creature: a rat for h", 3)
	cast("alevo res rat", "Summon Wild Creature: a rat", 1)
	cast("alito mas res", "Kill All Creatures: corpses next to h", 1)
	cast("exana mas mort", "Undead Legion: skeletons of the corpses", 3)

	h.logout()
	s(2)
	b.logout()
	s(3)


@scenario()
def runes(ctx):
	"""The caster h makes each rune of spells.toml (class ad) of a blank rune
	in its right hand (Get Item) and uses it on a fitting target: b (in the
	temple), itself, a wild elephant, a pig, empty fields, a field with a
	field on it, the bag, the field with the leftover runes, a corpse.
	Refusals: NOCREATURE (a healing rune and a missile on an empty field),
	NOROOM (a field rune on the bag), EXHAUSTED, NOTMOVABLE (Chameleon on
	the ground), OUTOFRANGE (Desintegrate two fields away), NOTUSABLE
	(Paralyze by a caster who is no druid, Animate Dead without a corpse);
	h becomes a druid (Change Profession) for Paralyze."""
	s = Steps(ctx)
	ctx.at(0)
	h = ctx.login("h")
	s(2)
	b = ctx.login("b")
	s(2)
	free_hands(s, h)
	s(1)
	junk = None

	def make(words, what):
		h.note("=== %s: Get Item, a blank rune into the right hand" % what)
		h.say("alevo %d" % BLANK_RUNE)
		s(1)
		h.note("=== %s: %s makes the rune" % (what, words))
		h.say(words)
		s(2)

	def on_field(rune, p, what, k=3):
		h.note("=== " + what)
		tid = h.type_at(p)
		ctx.check(tid is not None, "runes: nothing known of the field %s" % (p,))
		h.use_with(HAND, rune, 0, p, tid or 0, 0)
		s(k)

	def on_self(rune, what, k=3):
		h.note("=== " + what)
		h.use_with(HAND, rune, 0, h.pos, CREATURE, 0)
		s(k)

	def on_creature(rune, cid, what, k=3):
		h.note("=== " + what)
		ctx.check(cid is not None, "runes: no creature for %s" % what)
		h.use_on_creature(HAND, rune, 0, cid or 0)
		s(k)

	def drop(rune):
		# What is left of the rune goes onto the field west of h, which
		# Desintegrate clears later.
		h.note("=== the rest of the rune onto the field west of h")
		h.move(HAND, rune, 0, junk, 1)
		s(1)

	def summon(race):
		m = h.mark()
		h.note("=== Summon Wild Creature: %s, a target" % race)
		h.say("alevo res %s" % race)
		s(1)
		cid = h.wait_creature(race, m, 3)
		ctx.check(cid is not None, "runes: the %s was not seen" % race)
		return cid

	# In the temple, next to b.
	make("adura gran", "Intense Healing Rune")
	on_field(3152, offset(h.pos, 0, 1), "Intense Healing Rune on an empty field: NOCREATURE", 2)
	h.note("=== Intense Healing Rune on b")
	h.use_with(HAND, 3152, 0, b.pos, CREATURE, 0)
	s(2)

	teleport(s, h, CASTER_SPOT, "open grass for the runes")
	c = CASTER_SPOT
	junk = offset(c, -1, 0)
	elephant = summon("elephant")

	make("adura vita", "Ultimate Healing Rune")
	on_self(3160, "Ultimate Healing Rune on h")

	make("adori", "Light Magic Missile")
	on_field(3174, offset(c, 3, 0), "Light Magic Missile on an empty field: NOCREATURE", 2)
	on_creature(3174, elephant, "Light Magic Missile on the elephant", 1)
	on_creature(3174, elephant, "Light Magic Missile again a round later: EXHAUSTED", 1)
	drop(3174)

	make("adori gran", "Heavy Magic Missile")
	on_creature(3198, elephant, "Heavy Magic Missile on the elephant", 1)
	drop(3198)

	make("adori flam", "Fireball")
	on_field(3189, offset(c, 3, -2), "Fireball on a field", 1)
	drop(3189)

	make("adori gran flam", "Great Fireball")
	on_field(3191, offset(c, -3, -3), "Great Fireball on a field", 1)
	drop(3191)

	make("adevo mas hur", "Explosion")
	on_field(3200, offset(c, 2, 3), "Explosion on a field", 1)
	drop(3200)

	make("adori vita vis", "Sudden Death")
	on_creature(3155, elephant, "Sudden Death on the elephant")

	make("adevo res flam", "Soulfire")
	on_creature(3195, elephant, "Soulfire on the elephant", 1)
	drop(3195)

	make("adevo res pox", "Envenom")
	on_creature(3179, elephant, "Envenom on the elephant")

	make("adevo grav flam", "Fire Field")
	h.note("=== Fire Field on the bag (not on the map): NOROOM")
	h.use_with(HAND, 3188, 0, (INV, SLOT_BACKPACK, 0), BAG, 0)
	s(2)
	fire = offset(c, 2, -2)
	on_field(3188, fire, "Fire Field on an empty field", 1)
	drop(3188)

	make("adevo grav pox", "Poison Field")
	on_field(3172, offset(c, -2, 2), "Poison Field on an empty field", 1)
	drop(3172)

	make("adevo grav vis", "Energy Field")
	on_field(3164, offset(c, -2, -2), "Energy Field on an empty field", 1)
	drop(3164)

	make("adito grav", "Destroy Field")
	on_field(3148, fire, "Destroy Field on the fire field", 1)
	drop(3148)

	make("adevo mas grav flam", "Fire Wall")
	on_field(3190, offset(c, 4, 0), "Fire Wall to the east (north-south)", 1)
	drop(3190)

	make("adevo mas grav pox", "Poison Wall")
	on_field(3176, offset(c, -3, 3), "Poison Wall to the south-west (diagonal)", 1)
	drop(3176)

	make("adevo mas grav vis", "Energy Wall")
	on_field(3166, offset(c, 0, -4), "Energy Wall to the north (east-west)", 1)
	drop(3166)

	make("adevo mas flam", "Firebomb")
	on_field(3192, offset(c, 4, 4), "Firebomb on a field", 1)
	drop(3192)

	make("adevo mas vis", "Energybomb")
	on_field(3149, offset(c, 0, 5), "Energybomb on a field", 1)
	drop(3149)

	make("adevo mas pox", "Poisonbomb")
	on_field(3173, offset(c, -4, -4), "Poisonbomb on a field", 1)
	drop(3173)

	make("adevo grav tera", "Magic Wall")
	on_field(3180, offset(c, 1, -3), "Magic Wall on an empty field", 1)
	drop(3180)

	make("adevo ina", "Chameleon")
	on_field(3178, offset(c, 0, 2), "Chameleon on the ground: NOTMOVABLE", 2)
	h.note("=== Chameleon on the bag: h looks like a bag")
	h.use_with(HAND, 3178, 0, (INV, SLOT_BACKPACK, 0), BAG, 0)
	s(2)

	make("adana pox", "Antidote Rune")
	on_self(3153, "Antidote Rune on h", 2)

	make("adito tera", "Desintegrate")
	on_field(3197, offset(c, 2, 0), "Desintegrate two fields away: OUTOFRANGE", 2)
	on_field(3197, junk, "Desintegrate the leftover runes west of h", 1)
	drop(3197)

	make("adana ani", "Paralyze")
	on_creature(3165, elephant, "Paralyze by a caster who is no druid: NOTUSABLE", 2)
	h.note("=== Change Profession: h becomes a druid")
	h.say("aleta vita druid")
	s(1)
	on_creature(3165, elephant, "Paralyze by a druid: the elephant")

	pig = summon("pig")
	make("adeta sio", "Convince Creature")
	on_creature(3177, pig, "Convince Creature: the pig follows h")

	make("adana mort", "Animate Dead")
	m = h.mark()
	h.note("=== Kill All Creatures: the elephant and the pig next to h leave corpses")
	h.say("alito mas res")
	s(1)

	def is_corpse(rec):
		found = ADD_FIELD.match(rec["text"])
		return found is not None and ctx.types.has(int(found.group(4)), "corpse")

	rec = h.wait_for(is_corpse, m, 3)
	ctx.check(rec is not None, "runes: no corpse after Kill All Creatures")
	if rec is not None:
		x, y, z, tid = map(int, ADD_FIELD.match(rec["text"]).groups())
		corpse, corpse_type = (x, y, z), tid
	else:
		corpse, corpse_type = offset(c, 1, 1), 0
	teleport(s, h, corpse, "onto the corpse")
	empty = offset(h.pos, 1, 0)
	if empty == corpse:
		empty = offset(h.pos, -1, 0)
	on_field(3203, empty, "Animate Dead on a field without a corpse: NOTUSABLE", 2)
	h.note("=== Animate Dead on the corpse: a skeleton for h")
	h.use_with(HAND, 3203, 0, corpse, corpse_type, 0)
	s(3)

	h.logout()
	s(2)
	b.logout()
	s(3)


# The scenarios triggers and triggers_busy: the trigger rules of triggers.toml
# (src/triggers.cc HandleEvent). The game tries the rules of an event in their
# order and runs the actions of the first one whose conditions all hold; an
# event where none holds makes a use NOTUSABLE ("You cannot use this object.")
# and changes nothing otherwise. The caster h (Get Item by type number, Move
# to coordinates, no damage taken) sets them off; the gamemasters c, d and e
# (Teleport to Friend, e also Move to coordinates) join in where a second
# character is needed. Get Item puts an object into the first free hand (the
# right one, HAND, then the left one, LEFT), one with the movement_event flag
# into the bag (CreateAtCreature). Positions are of the original map: the
# temple of Rookgaard and the cave under it, the stairs of a house north of
# it, the rope spot of the scenario spells, the bridge to the west of the
# town, and the depot of Thais.
LEFT = (INV, SLOT_LEFT, 0)
SLOT_RING = 9
RING = (INV, SLOT_RING, 0)
BAG_FIRST = (INV, 64, 0)            # the first object in the bag (window 0)
TEMPLE = (32097, 32219, 7)
PANDA_TEDDY = 5080                  # use: says "Hug me ^^"
CLOSED_TRAP = 3481                  # use: becomes a trap (outside a protection zone)
TRAP = 3482                         # moved or stepped on: closes again
STEALTH_RING = 3049                 # put on: becomes 3086, taken off: 3049 again
STEALTH_RING_WORN = 3086
KNIFE = 3469                        # on a pumpkin: a pumpkinhead
PUMPKIN = 3594
PUMPKINHEAD = 2977                  # on a candlestick: a lit pumpkinhead
LIT_PUMPKINHEAD = 2978
CANDLESTICK = 2917
FISHING_ROD = 3483                  # on water: a splash
ROPE = 3003                         # on a rope spot: up; on a sewer grate: pulls up what lies below
BLUEBERRY_BUSH = 3699
DRAW_WELL = 1930
SEWER_GRATE = 435
LADDER = 1948
LEVER = 2772
LEVER_PULLED = 2773
WATER = 622
ROPE_SPOT_GROUND = 386
# The bush north of the temple and where h stands to use it.
BUSH = (32100, 32210, 7)
BUSH_STAND = (32100, 32211, 7)
# A draw well that no rule names (the rules of draw wells name their places).
WELL = (32093, 32202, 7)
WELL_STAND = (32093, 32203, 7)
# The sewer grate north of the temple; the cave below it has a ladder (on the
# field under the grate), a lever pair that lays a drawbridge over the water
# (Use/Levers/Rookgaard) and water fields.
GRATE = (32097, 32205, 7)
GRATE_STAND = (32097, 32206, 7)
CAVE_LADDER = (32097, 32205, 8)
CAVE_LEVER = (32098, 32204, 8)
CAVE_WATER = (32098, 32207, 8)      # water with a dirt floor border on it
CAVE_POOL = (32098, 32208, 8)       # water and nothing else
CAVE_WATER_EDGE = (32099, 32206, 8)  # water with a dirt floor border: three objects with a thrown one
# The rope spot (Use/Special Rights, MultiUse/Roping) and the hole above it
# (Collision/Teleporters Relative).
ROPE_SPOT_FIELD = (32104, 32170, 9)
ROPE_SPOT_STAND = (32104, 32171, 9)
# Stairs down (437) and up (1947) of a house north of the temple.
STAIRS_BELOW = (32096, 32191, 8)    # south of the stairs up at 32096,32190,8
# The bridge west of the town (Collision/Rookgaard Bridge): the wooden floor at
# x 32057 sends a player without a premium account back east to x 32060.
BRIDGE_NORTH = (32058, 32192, 7)
BRIDGE_SOUTH = (32058, 32193, 7)
# The depot of Thais, ground floor (Collision and Separation/Depots/Depot
# Thais): a player on a depot tile (431, 430 while stepped on) gets the locker
# north or south of it filled with the depot; leaving it saves it. Dustbins
# delete what is thrown into them (Collision/Dustbins).
DEPOT_STAND = (32352, 32227, 7)     # south of the depot tile 32352,32226
DEPOT_TILE = (32352, 32226, 7)
DEPOT_DUSTBIN = (32350, 32229, 7)


@scenario()
def triggers(ctx):
	"""The trigger rules of triggers.toml, each event type with its actions
	run and some where no rule holds. use: a panda teddy, a closed trap in
	the hand and on the floor, a blueberry bush, a draw well (no rule), a
	sewer grate, a lever pair and a ladder in the cave under the temple, a
	rope spot (no rule: the right special_moveuse is missing); multi_use: a
	knife on a pumpkin and on the pumpkinhead (no rule), the pumpkinhead on
	a candlestick, a fishing rod on water, a rope on a sewer grate and on a
	rope spot; movement: a trap thrown, a stealth ring put on, taken off and
	put into a hand (no rule); collision: a trap and a hole stepped on,
	stairs down and up, a fishing rod thrown into water (and an object onto
	water with a border, no rule), the bridge west of the town by e (no
	premium account) and h, a depot tile, a dustbin, an object thrown onto a
	depot tile (no rule); separation: leaving the depot tile, the object
	taken off it (no rule)."""
	s = Steps(ctx)
	ctx.at(0)
	h = ctx.login("h")
	s(2)
	free_hands(s, h)
	s(1)
	junk = offset(TEMPLE, -1, 0)
	trap_field = offset(TEMPLE, 0, 1)

	def get(tid, what):
		h.note("=== Get Item: %s" % what)
		h.say("alevo %d" % tid)
		s(1)

	def use(p, tid, what, k=2):
		h.note("=== use: " + what)
		h.use(p, tid, 0)
		s(k)

	def use_with(p, tid, p2, tid2, what, k=2):
		h.note("=== multi_use: " + what)
		h.use_with(p, tid, 0, p2, tid2, 0)
		s(k)

	def move(p, tid, dest, what, k=2):
		h.note("=== " + what)
		h.move(p, tid, 0, dest, 1)
		s(k)

	def walk(c, d, what, k=3):
		c.note("=== " + what)
		c.walk(d)
		s(k)

	# In the temple of Rookgaard (no protection zone).
	get(PANDA_TEDDY, "a panda teddy")
	use(HAND, PANDA_TEDDY, "the panda teddy says Hug me (Use/Fun)")
	move(HAND, PANDA_TEDDY, junk, "the panda teddy onto the field west of h", 1)
	get(CLOSED_TRAP, "a closed trap")
	use(HAND, CLOSED_TRAP, "the closed trap in the hand: not in a protection zone, it becomes a trap (Use/Fun)")
	move(HAND, TRAP, trap_field, "movement: the trap thrown south of h closes with an effect (Movement)")
	use(trap_field, CLOSED_TRAP, "the closed trap on the floor becomes a trap again (Use/Fun)")
	walk(h, "s", "collision: h steps onto the trap, which closes (Collision/Trap Damage)")
	walk(h, "n", "back to the temple's middle")

	get(STEALTH_RING, "a stealth ring (movement_event: into the bag)")
	move(BAG_FIRST, STEALTH_RING, RING, "movement: the stealth ring put on becomes the worn one (Movement/Magic Rings)")
	move(RING, STEALTH_RING_WORN, BAG_FIRST, "movement: the worn stealth ring into the bag becomes the plain one again "
		"(Movement/Magic Rings)")
	move(BAG_FIRST, STEALTH_RING, HAND, "movement: the stealth ring into the hand, not worn: no rule holds")
	move(HAND, STEALTH_RING, junk, "the stealth ring onto the field west of h", 1)

	get(KNIFE, "a knife")
	get(PUMPKIN, "a pumpkin, into the left hand")
	use_with(HAND, KNIFE, LEFT, PUMPKIN, "the knife on the pumpkin carves a pumpkinhead (MultiUse/Fun)")
	use_with(HAND, KNIFE, LEFT, PUMPKINHEAD, "the knife on the pumpkinhead: no rule holds, NOTUSABLE")
	move(HAND, KNIFE, junk, "the knife onto the field west of h", 1)
	get(CANDLESTICK, "a candlestick")
	use_with(LEFT, PUMPKINHEAD, HAND, CANDLESTICK, "the pumpkinhead on the candlestick: a lit pumpkinhead, the "
		"candlestick gone (MultiUse/Fun)")
	move(LEFT, LIT_PUMPKINHEAD, junk, "the lit pumpkinhead onto the field west of h", 1)

	# North of the temple.
	teleport(s, h, BUSH_STAND, "south of a blueberry bush")
	use(BUSH, BLUEBERRY_BUSH, "the blueberry bush: picked, three blueberries (Use/Fun)")
	teleport(s, h, WELL_STAND, "south of a draw well")
	use(WELL, DRAW_WELL, "the draw well: the rules of draw wells name other places, no rule holds, NOTUSABLE")
	teleport(s, h, GRATE_STAND, "south of the sewer grate")
	use(GRATE, SEWER_GRATE, "the sewer grate: h climbs down into the cave (Use/Teleporters)")

	# In the cave under the temple, h on the ladder's field.
	use(CAVE_LEVER, LEVER, "the lever: both levers pulled, a drawbridge over the water (Use/Levers/Rookgaard)")
	use(CAVE_LEVER, LEVER_PULLED, "the lever back: the drawbridge is water again (Use/Levers/Rookgaard)")
	get(FISHING_ROD, "a fishing rod")
	use_with(HAND, FISHING_ROD, CAVE_WATER, WATER, "the fishing rod on the water: a splash (MultiUse/Fishing)")
	move(HAND, FISHING_ROD, CAVE_POOL, "collision: the fishing rod thrown into the water is gone with a splash "
		"(Collision/Liquid Deletions)")
	get(PUMPKIN, "a pumpkin")
	move(HAND, PUMPKIN, CAVE_WATER_EDGE, "collision: the pumpkin thrown onto water with a border on it (three "
		"objects): no rule holds, it stays")
	get(KNIFE, "a knife")
	move(HAND, KNIFE, CAVE_LADDER, "the knife onto h's own field, for the rope later", 1)
	use(CAVE_LADDER, LADDER, "the ladder: h climbs up next to the sewer grate (Use/Teleporters)")
	get(ROPE, "a rope")
	use_with(HAND, ROPE, GRATE, SEWER_GRATE, "the rope on the sewer grate pulls up the knife from below "
		"(MultiUse/Roping)")

	# The rope spot of the scenario spells.
	teleport(s, h, ROPE_SPOT_STAND, "south of a rope spot")
	use(ROPE_SPOT_FIELD, ROPE_SPOT_GROUND, "the rope spot itself: the rule wants the right special_moveuse, no rule "
		"holds, NOTUSABLE")
	use_with(HAND, ROPE, ROPE_SPOT_FIELD, ROPE_SPOT_GROUND, "the rope on the rope spot: h climbs up (MultiUse/Roping)")
	walk(h, "n", "collision: h steps into the hole and falls onto the rope spot (Collision/Teleporters Relative)")

	# Stairs of a house north of the temple.
	teleport(s, h, STAIRS_BELOW, "south of stairs up")
	walk(h, "n", "collision: h steps onto the stairs up (Collision/Teleporters Relative)")
	walk(h, "s", "collision: h steps onto the stairs down (Collision/Teleporters Relative)")

	# The bridge west of the town; e logs in now that the temple is free.
	e = ctx.login("e")
	s(2)
	teleport(s, e, BRIDGE_NORTH, "e onto the bridge")
	walk(e, "w", "collision: e, level 1 without a premium account, on the wooden floor at the bridge's end is sent back "
		"east (Collision/Rookgaard Bridge)")
	teleport(s, h, BRIDGE_SOUTH, "h onto the bridge")
	walk(h, "w", "collision: h, with the right premium_account, on the wooden floor at the bridge's end: the rule "
		"that lets it pass (nop) (Collision/Rookgaard Bridge)")

	# The depot of Thais.
	teleport(s, h, DEPOT_STAND, "south of a depot tile in Thais")
	move(HAND, ROPE, DEPOT_DUSTBIN, "collision: the rope thrown into a dustbin is gone (Collision/Dustbins)")
	walk(h, "n", "collision: h steps onto the depot tile, the locker gets the depot (Collision/Depots/Depot Thais)")
	walk(h, "s", "separation: h leaves the depot tile, the depot is saved (Separation/Depots/Depot Thais)")
	get(PANDA_TEDDY, "a panda teddy")
	move(HAND, PANDA_TEDDY, DEPOT_TILE, "collision: the panda teddy thrown onto the depot tile: no player, no rule "
		"holds")
	move(DEPOT_TILE, PANDA_TEDDY, HAND, "separation: the panda teddy taken off the depot tile: no rule holds")
	move(HAND, PANDA_TEDDY, DEPOT_DUSTBIN, "collision: the panda teddy thrown into the dustbin is gone "
		"(Collision/Dustbins)")

	e.logout()
	s(2)
	h.logout()
	s(3)


# The scenario triggers_busy: four characters step on and off a depot tile
# each, every one of them a step per round for BUSY_ROUNDS rounds, each step a
# collision (the locker gets the depot) or a separation (the depot is saved)
# whose rule the game finds after the rules before it. Each is alone in the
# depot of another town (out of sight of the others), so that steps in the
# same round cannot reach a client in another order in the other run. h and e
# go there by Move to coordinates, c and d by Teleport to Friend to h (who
# waits in their depot first) and a walk to their field.
BUSY_ROUNDS = 150
# Each walker: the field it waits on, the direction of its depot tile from
# there, and for c and d the field h waits on for them and the free field
# next to their own that they walk to first (the fields between are free).
BUSY_WALKERS = [
	("h", (32352, 32227, 7), "n", None, None),                                   # Thais
	("e", (32926, 32080, 7), "s", None, None),                                   # Venore
	("c", (32336, 31783, 8), "s", (32335, 31781, 8), (32336, 31782, 8)),         # Carlin
	("d", (33215, 32464, 8), "s", (33214, 32462, 8), (33215, 32463, 8)),         # Darashia
]
BACK = {"n": "s", "s": "n"}
STEP = {(0, -1): "n", (1, -1): "ne", (1, 0): "e", (1, 1): "se", (0, 1): "s", (-1, 1): "sw", (-1, 0): "w",
	(-1, -1): "nw"}


def path(src, dst):
	"""The steps from src to dst, diagonal while both coordinates differ, no
	obstacles considered."""
	x, y = src[0], src[1]
	steps = []
	while (x, y) != (dst[0], dst[1]):
		dx = (dst[0] > x) - (dst[0] < x)
		dy = (dst[1] > y) - (dst[1] < y)
		steps.append(STEP[(dx, dy)])
		x += dx
		y += dy
	return steps


@scenario()
def triggers_busy(ctx):
	"""Many collision and separation events in a short time, for the CPU
	time of HandleEvent: h, e, c and d, each alone in a depot (Thais,
	Venore, Carlin, Darashia), step on and off a depot tile, every one a
	step per round for BUSY_ROUNDS rounds; each step runs a rule with
	actions (the depot loaded into a locker, or saved)."""
	s = Steps(ctx)
	ctx.at(0)
	who = {}
	for key in ("h", "e", "c", "d"):
		who[key] = ctx.login(key)
		s(2)
	h = who["h"]
	for key, _, _, meet, _ in BUSY_WALKERS:
		if meet is None:
			continue
		teleport(s, h, meet, "h waits for %s in its depot" % key)
		who[key].note("=== Teleport to Friend: next to h")
		who[key].say('alani sio "Testh Theta"')
		s(2)
	for key, stand, d, meet, _ in BUSY_WALKERS:
		if meet is None:
			teleport(s, who[key], stand, "%s next to a depot tile" % key)
	for key, stand, d, meet, near in BUSY_WALKERS:
		if meet is None:
			continue
		c = who[key]
		p = c.pos
		ctx.check(p is not None and p[2] == meet[2] and max(abs(p[0] - meet[0]), abs(p[1] - meet[1])) == 1,
			"triggers_busy: %s is not next to where h waited: %s" % (key, p))
		steps = path(p, near) + path(near, stand) if p else []
		c.note("=== %s walks to the field next to its depot tile" % key)
		c.autowalk(steps)
		s(len(steps) + 3)
	for key, stand, _, _, _ in BUSY_WALKERS:
		ctx.check(who[key].pos == stand, "triggers_busy: %s is at %s, not at %s" % (key, who[key].pos, stand))
		who[key].note("=== %d rounds: a step onto the depot tile (collision) and back (separation), one a round"
			% BUSY_ROUNDS)
	for i in range(BUSY_ROUNDS):
		for key, _, d, _, _ in BUSY_WALKERS:
			who[key].walk(d if i % 2 == 0 else BACK[d])
		s(1)
	s(2)
	for key in ("h", "e", "c", "d"):
		who[key].logout()
		s(1)
	s(3)
