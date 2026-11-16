-- npcs/quentin.lua: the behaviour of the NPC Quentin, converted from
-- quentin.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of quentin.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- quentin.npc: Datenbank für den Mönch Quentin

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PVPENFORCED = behaviour.PVPENFORCED
local FORCE = behaviour.FORCE
local lt = behaviour.lt
local gt = behaviour.gt
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local questValue = behaviour.questValue
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local effectOpp = behaviour.effectOpp
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome, adventurer %N! If you are new in Tibia, ask me for help."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "heal$", gt(BURNING, 0)}, {"You are burning, %N. I will help you.", burning(0, 0), effectOpp(15)}},
	{{BUSY, "heal$", gt(POISON, 0)}, {"You are poisoned, %N. I will help you.", poison(0, 0), effectOpp(14)}},
	{{BUSY, "heal$", lt(HP, 40)}, {"You are looking really bad, %N. Let me heal your wounds.", setHp(40), effectOpp(13)}},
	{{BUSY, "heal$"}, {"You aren't looking that bad, %N. Sorry, I can't help you."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Remember: If you are heavily wounded or poisoned, I will heal you."}},

	{{"bye"}, {"Good bye, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"Job? I have no job. I just live for the gods of Tibia."}},
	{{"name"}, {"My name is Quentin."}},
	{{"tibia"}, {"That is where we are. The world of Tibia. Admire it's beauty."}},
	{{"god"}, {"They created Tibia and all life on it."}},
	{{"life"}, {"On Tibia there are many forms of life. There are plants and people and monsters."}},
	{{"plant"}, {"Just walk around, you will see grass, trees, and bushes."}},
	{{"people"}, {"I am a simple monk. I just know Sam, Frodo, and Gorn. They all live in the main street to the north."}},
	{{"sam"}, {"He is our blacksmith. He sells weapons and armour."}},
	{{"frodo"}, {"He is the owner of Frodo's Hut, the tavern north of this temple."}},
	{{"gorn"}, {"He is selling equipment. If you still have no backpack you should go and ask him for one."}},
	{{"elane"}, {"She is the leader of the local Paladins' guild."}},
	{{"muriel"}, {"Muriel is a famous sorcerer. She is the keeper of arcane secrets that are known only to few mortals."}},
	{{"gregor"}, {"The leader of the Knights' guild is a man of few words."}},
	{{"marvik"}, {"I admire the healing skills of Marvik."}},
	{{"king"}, {"Our king resides in the castle to the west."}},
	{{"tibianus"}, {previous}},
	{{"lynda"}, {"She is a highly competent priest."}},
	{{"harkath"}, {"A hard man but his heart is in the right right place."}},
	{{"army"}, {"I don't know much about the Tibian army. Ask general Harkath Bloodblade about that."}},
	{{"ferumbras"}, {"Hush! Do not mention the Evil One in these walls."}},
	{{"general"}, {"Harkath Bloodblade is his name."}},
	{{"bozo"}, {"He is the king's jester, but he believes himself to be the king of fools."}},
	{{"baxter"}, {"He is the guard of the royal castle."}},
	{{"oswald"}, {"This man is spreading horrible rumours all the time."}},
	{{"sherry"}, {"The McRonalds run the local farm."}},
	{{"mcronald"}, {previous}},
	{{"donald"}, {previous}},
	{{"lugri"}, {"Please do not mention the fallen one."}},
	{{"excalibug"}, {"Legends tell us that that Excalibug is a gift of the gods. Banor used in his battles.  They say it was passed on to one of his followers."}},
	{{"news"}, {"Sorry, I know nothing new. Please ask Frodo about that topic."}},
	{{"monster"}, {"There are really too many of them in Tibia. But who am I to challenge the wisdom of the gods?"}},
	{{"help"}, {"First you should try to get some gold to buy better equipment."}},
	{{"quest"}, {previous}},
	{{"task"}, {previous}},
	{{"what", "do"}, {previous}},
	{{"gold"}, {"If you need money you should slay monsters and take their gold. Look for spiders and rats."}},
	{{"money"}, {previous}},
	{{"spider"}, {"There are spiders' nests beyond our city near Gorn's shop and at the McRonalds' farm in the east."}},
	{{"rat"}, {"There are sewers underneath the city. They say these sewers are brimming with rats."}},
	{{"sewer"}, {"You can enter the sewers thorugh a sewer grate. But watch out. There are many rats. And don't forget to bring a torch."}},
	{{"equipment"}, {"First you should buy a bag or backpack. That way your hands will be free to hold a weapon and a shield."}},
	{{"fight"}, {"Take a weapon into your hand and select a target. If you are wounded you should eat some food to heal your wounds."}},
	{{"slay"}, {"Take a weapon into your hand and select a target. If you are wounded you should eat some food to heal your wounds."}},
	{{"eat"}, {"If you would like to heal your wounds you should eat some food. Frodo sells excellent meals. But if you are very weak you can also come to me. I will heal you."}},
	{{"food"}, {previous}},
	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are looking really bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},

	{{"heal$", PVPENFORCED}, {"You aren't looking that bad."}},
	{{"heal$"}, {"You aren't looking that bad. Sorry, I can't help you. But if you are looking for additional protection you should go on the pilgrimage of ashes."}},

	{{"blessing", PVPENFORCED, FORCE}, {"The lifeforce of this world is waning. There are no more blessings avaliable on this world."}},
	{{"pilgrimage", PVPENFORCED, FORCE}, {previous}},
	{{"ashes", PVPENFORCED, FORCE}, {previous}},

	{{"spiritual", PVPENFORCED, FORCE}, {previous}},
	{{"shielding", PVPENFORCED, FORCE}, {previous}},
	{{"sacred", "places", PVPENFORCED, FORCE}, {previous}},
	{{"spark", PVPENFORCED, FORCE}, {previous}},
	{{"phoenix", PVPENFORCED, FORCE}, {previous}},
	{{"embrace", PVPENFORCED, FORCE}, {previous}},
	{{"fire", PVPENFORCED, FORCE}, {previous}},
	{{"suns", PVPENFORCED, FORCE}, {previous}},
	{{"wisdom", PVPENFORCED, FORCE}, {previous}},
	{{"solitude", PVPENFORCED, FORCE}, {previous}},

	{{"pilgrimage"}, {"Whenever you receive a lethal wound your lifeforce is damaged. With every single of the five blessings you have this damage will be reduced."}},
	{{"ashes"}, {previous}},

	{{"blessing"}, {"There are five different blessings available in five sacred places. These blessings are: the spiritual shielding, the spark of the phoenix, the embrace of tibia, the fire of the suns and the wisdom of solitude."}},
	{{"sacred", "places"}, {"Just ask in which of the five blessings you are interested in."}},

	{{"spiritual", gt(questValue(104), 0)}, {"I see you received the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding", gt(questValue(104), 0)}, {previous}},
	{{"spiritual"}, {"You can receive the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding"}, {previous}},

	{{"spark", gt(questValue(102), 0)}, {"I an sense that the spark of the phoenix already was given to you by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix", gt(questValue(102), 0)}, {previous}},
	{{"spark"}, {"The spark of the phoenix is given by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix"}, {previous}},

	{{"embrace", gt(questValue(105), 0)}, {"I can sense the druids north of Carlin provided you with the embrace of tibia."}},
	{{"embrace"}, {"The druids north of Carlin can provide you with the embrace of tibia."}},

	{{"fire", "suns", gt(questValue(103), 0)}, {"I can see you recieved the blessing of the two suns in the suntower near Ab'Dendriel ."}},
	{{"suns", gt(questValue(103), 0)}, {previous}},
	{{"fire", "suns"}, {"Ask for the blessing of the two suns in the suntower near Ab'Dendriel ."}},
	{{"suns"}, {previous}},

	{{"wisdom", gt(questValue(101), 0)}, {"I can sense you already talked to the hermit Eremo on the isle of Cormaya and recieved this blessing."}},
	{{"solitude", gt(questValue(101), 0)}, {previous}},
	{{"wisdom"}, {"Talk to the hermit Eremo on the isle of Cormaya about this blessing."}},
	{{"solitude"}, {previous}},

	{{"time"}, {"Now, it is %T. Ask Gorn for a watch, if you need one."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
