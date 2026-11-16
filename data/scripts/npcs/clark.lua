-- npcs/clark.lua: the behaviour of the NPC Clark, converted from
-- clark.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of clark.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- clark.npc: Datenbank für den captain der Wache clark

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hello."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Wait.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Goodbye."}},

	{{"bye"}, {"Goodbye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the captain of the guards and responsible for upholding law and order in the colony."}},
	{{"name"}, {"I am called Clark."}},
	{{"guards"}, {"The guards of Port Hope are led by Lord Seamus, a nobleman from Thais."}},
	{{"seamus"}, {"He is ill and I am responsible for upholding the law now."}},
	{{"time"}, {"It is %T right now."}},
	{{"king"}, {"Our king can be proud of this colony."}},
	{{"venore"}, {"The Venorans are fine and generous people. It's a good thing that they put all this effort into this colony. I don't know what we would do if it weren't for them."}},
	{{"thais"}, {"Thais is a big city but if we do well enough here, Port Hope might one day rival its size."}},
	{{"carlin"}, {"It's a shame those women still get away with their independence. Friends told me that too many of Thais's resources are wasted in useless projects and the army is too weak to claim back what is ours."}},
	{{"edron"}, {"The knights of Edron are somewhat arrogant. They use the resources of the isle only for themselves instead of sharing it."}},
	{{"jungle"}, {"The jungle is our first and foremost enemy."}},

	{{"tibia"}, {"I would guess a good part of the world is under Thaian rule."}},

	{{"kazordoon"}, {"As long as the dwarves stay there and don't expand, I think it's ok to let them have this ugly piece of rock."}},
	{{"dwarves"}, {"Dwarves are impressive fighters. The Venorans talked about hiring some of them as mercenaries to help us out."}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"My guess is that those elves are secretly allied with Carlin. I would not trust an elf if I would meet one."}},
	{{"elves"}, {previous}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"This continent will be ours one day."}},
	{{"darashia"}, {"A village of harmless cultists. They pose no real threat and could easily been integrated into the Thaian realm."}},
	{{"ankrahmun"}, {"This city is a threat to Thais's domination of Darama. Yet, we are too weak to handle this threat."}},
	{{"ferumbras"}, {"He is the enemy number one to the kingdom and all good people."}},
	{{"excalibug"}, {"I heard the knights of Edron hide it somewhere instead of using it for the good of the country."}},
	{{"apes"}, {"Rest assured, we will handle that problem."}},
	{{"lizard"}, {"They pose no real threat to us."}},
	{{"dworcs"}, {"They fear our power and hide in some caves."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
