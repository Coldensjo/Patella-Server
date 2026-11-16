-- npcs/kawill.lua: the behaviour of the NPC Kawill, converted from
-- kawill.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of kawill.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- kawill.npc: Datenbank für den Geomancer Kawill (Zwergenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PVPENFORCED = behaviour.PVPENFORCED
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local TOPIC = behaviour.TOPIC
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N! May earth protect you!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "heal$", gt(BURNING, 0)}, {"You are burning. Let me help you.", burning(0, 0), effectOpp(15)}},
	{{BUSY, "heal$", gt(POISON, 0)}, {"You are poisoned. Let me help you.", poison(0, 0), effectOpp(14)}},
	{{BUSY, "heal$", lt(HP, 40)}, {"You are hurt, my child, let me heal your wounds.", setHp(40), effectOpp(13)}},
	{{BUSY, "heal$"}, {"You aren't looking really bad. Sorry, I can't help you."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Earth under your feet, pilgrim."}},

	{{"bye"}, {"Earth under your feet, %N!", idle}},
	{{"farewell"}, {previous}},

	{{"fire", PVPENFORCED, FORCE}, {"The lifeforce of this world is wanig. There are no more blessings avaliable on this world."}},
	{{"suns", PVPENFORCED, FORCE}, {previous}},
	{{"fire", "suns"}, {"You can get the blessing of the two suns in the suntower near Ab'Dendriel."}},

	{{"job"}, {"I am the great geomancer of dwarvenkind."}},
	{{"name"}, {"I am Kawill Marbleeye, Son of Earth, from the Molten Rock."}},
	{{"tibia"}, {"Nice world in general. It's a shame there is so much water ruining the general impression."}},
	{{"kazordoon"}, {"By using the powers of fire and earth we forced the river that once wound its way through the big old one in other directions, and created our home."}},
	{{"big", "old"}, {"The mountain we live in is called the big old one. It's the mountain of mountains, and it isand like a friend and protector to our race."}},
	{{"elves"}, {"Who cares for that silly people."}},
	{{"humans"}, {"We are allied with this young race, though they seldom have the wisdom to listen to us."}},
	{{"orcs"}, {"Stupid beasts. Their savagery is only rivalled by their smell."}},
	{{"minotaurs"}, {"They lost their way long ago. Now they are lost and doomed. It should be a warning to all of us."}},
	{{"geomancer"}, {"We investigate the will of the earth. It is our duty to make sure things to work in their natural way."}},
	{{"god"}, {"The gods are treacherous and vain. They want to use us like they did in the past. Only the elements can be trusted, because all they want is for nature to run its set course."}},
	{{"earth"}, {"The lifegiving earth protects us, feeds us and takes us home after death."}},
	{{"fire"}, {"Where earth is giving, fire is taking. That is the way of the elements."}},
	{{"life"}, {"Life is born by earth and fed by earth."}},
	{{"plant"}, {"Plants are minor messengers of earth. If you understand the soil you understand the plants."}},
	{{"citizen"}, {"Many people are living in the embracement of earth in Kazordoon."}},
	{{"kroox"}, {"He is a fine smith and his armour may save your neck one day."}},
	{{"jimbin"}, {"He is a jolly fellow and one of the oldest dwarves alive."}},
	{{"maryza"}, {"She is a fine cook, jawoll."}},
	{{"bezil"}, {"Bezil and Nezil have pawn and equpiment shop with an amazing stock."}},
	{{"nezil"}, {previous}},
	{{"uzgod"}, {"Uzgod is a blacksmith and understands the ways of his element well."}},
	{{"etzel"}, {"I fear the sorcerers focus on the destructive forces of fire. They forget about the protection earth could provide."}},
	{{"motos"}, {"The scars in this dwarf's face tell the tale of many a great battle."}},
	{{"durin"}, {"The celestial paladin, the protector of our race. The only divine being we care for and the only one who still cares for dwarfs."}},
	{{"duria"}, {"The first knight of dwarvenkind is a fine woman."}},
	{{"emperor"}, {"The emperor has rarely visited the temple district in the last years. He should care more about spirituality then about politics. Jawoll."}},
	{{"kruzak"}, {previous}},
	{{"pyromancer"}, {"They are the followers of the great flame."}},
	{{"technomancer"}, {"FOOLS! FOOLS! ALL OF THEM! MAY EARTH SWALLOW THEM ALL!"}},
	{{"motos"}, {"He finally made his peace with his own heart."}},
	{{"general"}, {previous}},
	{{"army"}, {"Our fortresses are strong and easy to defend. Any aggressor will be smashed by our armies. Most intruders will not get manage to pass the colossus."}},
	{{"colossus"}, {"The big fortress that guards our realm is shaped like a dwarf."}},
	{{"ferumbras"}, {"The day will come when he finally bites the dust."}},
	{{"excalibug"}, {"Ah, a weapon to be feared by man, beast and god alike, jawoll. He who wields it will be both blessed and cursed at the same time."}},
	{{"news"}, {"There will be nothing new, but every pain and every pleasure will return to you, and all in the same order. The eternal hour glass of existence will be turned again and again, and you will be turned with it, little speck of dust."}},
	{{"Nietzsche"}, {"In his mind he might have been a giant, but in his heart he was a dwarf."}},
	{{"monster"}, {"May the earth swallow them all!"}},
	{{"stone", "golem"}, {"These beings are filled with the power of earth. Therefore they, too, are sacred in a way."}},
	{{"help"}, {"I am a mere diviner of earth's will and not allowed to help you."}},
	{{"quest"}, {"There's nothing I need, better ask others."}},
	{{"task"}, {previous}},
	{{"what", "do"}, {previous}},
	{{"gold"}, {"Gold is one of earth's treasures. To have gold is to be blessed by earth. To be rich is to be favoured by earth."}},
	{{"money"}, {previous}},
	{{"equipment"}, {"You can buy equiment in Bezil's and Nezil's little shop."}},
	{{"fight"}, {"Never forget your defence when fighting."}},

	{{"heal$", gt(BURNING, 0)}, {"You are burning. Let me help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. Let me help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are hurt, my child. Let me heal your wounds.", setHp(40), effectOpp(13)}},

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

	{{"pilgrimage"}, {"Whenever you recive a lethal wound your lifeforce is damaged. With every single of the five blessings you have this damage will be reduced."}},
	{{"ashes"}, {previous}},
	{{"blessing"}, {"There are five different blessings available in five sacred places. These blessings are: the spiritual shielding, the spark of the phoenix, the embrace of tibia, the fire of the suns and the wisdom of solitude."}},
	{{"sacred", "places"}, {"Just ask in which of the five blessings you are interested in."}},
	{{"spiritual"}, {"You can receive the spiritual shielding which in the whiteflower temple south of thais."}},
	{{"shielding"}, {previous}},
	{{"spark"}, {"The spark of the phoenix is given by me and by the great pyromancer in the nearby firetemple. Do you wish to receive my part of the blessing of the phoenix?", setTopic(1)}},
	{{"phoenix"}, {previous}},
	{{"embrace"}, {"The druids north of Carlin will provide you with the embrace of tibia."}},
	-- "fire","suns"     -> "You can get the blessing of the two suns in the suntower near Ab'Dendriel."
	-- "suns"      -> *
	-- nach oben gestellt wg antwort auf fire
	{{"wisdom"}, {"Talk to the hermit Eremo on the isle of Cormaya about this blessing."}},
	{{"solitude"}, {previous}},

	{{eq(TOPIC, 1), "yes", gt(questValue(199), 0), FORCE}, {"You already possess my blessing."}},
	{{eq(TOPIC, 1), "yes", gt(questValue(102), 0), FORCE}, {"You already possess my blessing."}},
	{{eq(TOPIC, 1), "yes"}, {"So receive the blessing of the live-giving earth, pilgrim.", effectOpp(13), setQuestValue(199, 1)}},
	{{eq(TOPIC, 1)}, {"Ok. If you don't want it ... ."}},

	{{"time"}, {"Time is not of importance."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
