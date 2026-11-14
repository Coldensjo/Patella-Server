"""Scenarios for run_scenario.py. Each is a function of the context; ctx.at(n)
waits for step round n (relative to the start round), ctx.login(key) logs a
test client in. tools/trace_check.py runs them (run_scenario.py --list).

The test characters are new (no player file): they start in the Rookgaard
temple, a at [32097,32219,7] and b next to it at [32098,32219,7]. The temple
is a protection zone; the NPC Cipfried walks around in it.
"""

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
