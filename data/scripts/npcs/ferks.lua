-- npcs/ferks.lua: the behaviour of the NPC Ferks, converted from
-- ferks.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ferks.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ferks.npc: Datenbank für den Bankier Ferks

local behaviour = require("lib.npc")
local genBank = require("lib.npcs.gen-bank")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"A good day to you."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Wait, please.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a banker, my job is to exchange coins."}},
	{{"name"}, {"My name is Ferks <bows slightly>."}},
	{{"time"}, {"It is exactly %T right now."}},
	{{"king"}, {"His royal highness might visit this little but promising community one day if we develop well."}},
	{{"venore"}, {"Our city and society could be a role model for all Tibians."}},
	{{"thais"}, {"Thais is still a power that must be taken into account. Power is not everything of course."}},
	{{"carlin"}, {"Carlin is nothing but a nuisance."}},
	{{"edron"}, {"Edron has little to offer. It's overestimated. Let those silly knights have it. In the end it will be us who clean up the mess and bring order into the chaos that will be left when they finally leave."}},
	{{"jungle"}, {"This jungle is dangerous. The more area we can finally cultivate, the better. Only if we chop and burn enough of it down and create new farmland, we can build a new centre of commerce."}},

	{{"tibia"}, {"Sometimes I wonder why the gods don't put more effort into bringing us order but I am a banker, not a priest."}},

	{{"kazordoon"}, {"Dwarves are disciplined people, which I appreciate."}},
	{{"dwarves"}, {previous}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"Those elves are erratic and unreliable. They need to be taught some manners."}},
	{{"elves"}, {previous}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"This wilderness and the dry desert need to be colonised by civilised people like us. We bring the light of order and prosperity to this continent."}},
	{{"darashia"}, {"This town has little that we are interested in."}},
	{{"ankrahmun"}, {"I have to admit that I somehow admire the strong guidance of this pharao. Most fail to acknowledge that his subjects are in need of such a strict leadership."}},
	{{"ferumbras"}, {"A servant of chaos. I wonder why all the Thaian military power can't stop him for good."}},
	{{"excalibug"}, {"A fictitious weapon is of no use at all."}},
	{{"apes"}, {"They mindlessly attack our settlement to steal tools and everything else they get into their hairy hands."}},
	{{"lizard"}, {"They live so far away in the jungle that we know only little of them. Perhaps this primitive race can be used in some way."}},
	{{"dworcs"}, {"Evil little bastards they are."}},

	genBank,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
