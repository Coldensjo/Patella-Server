-- npcs/pydar.lua: the behaviour of the NPC Pydar, converted from
-- pydar.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of pydar.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- pydar.npc: Datenbank für den Pyromancer Pydar (Zwergenstadt)

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
local PRICE = behaviour.PRICE
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setHp = behaviour.setHp
local poison = behaviour.poison
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", eq(questValue(44), 1), FORCE}, {"Be greeted %N! I can smell the scent of a phoenix on you!"}},
	{{ADDRESS, "hi$", eq(questValue(44), 1), FORCE}, {previous}},

	{{ADDRESS, "hello$", FORCE}, {"Welcome, pilgrim %N! May the flame guide you!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May the fire in your heart never die."}},

	{{"scent", eq(questValue(44), 1)}, {"The phoenix seems to be fond of you! If you had a real phoenix egg on you, I could provide you the blessing of the spark of the phoenix more easy and cheaper!"}},
	{{"phoenix", "egg"}, {previous}},

	{{"bye"}, {"May the fire in your heart never die, %N!", idle}},
	{{"farewell"}, {previous}},

	{{"suns"}, {"You can ask for the blessing of the two suns in the suntower near Ab'Dendriel."}},

	{{"job"}, {"I am the head pyromancer of Kazordoon."}},
	{{"name"}, {"My name is Pydar Firefist, Son of Fire, from the Savage Axes."}},
	{{"tibia"}, {"That is our world."}},
	{{"kazordoon"}, {"Our city was founded in ancient times. Abandoned by the gods we once fought for, we created a secure haven for our people."}},
	{{"big", "old"}, {"This mountain is said to be the oldest in the world. It is the place where fire and earth meet and separate at the same time."}},
	{{"elves"}, {"Stupid race. They have no understanding of the ways of the world."}},
	{{"humans"}, {"They took the place dwarves once held in the world. They don't see that they are destined to fall just like we did."}},
	{{"orcs"}, {"The arch enemy. We could have destroyed them long ago, but this would have meant doing a favour to the gods which betrayed us."}},
	{{"minotaurs"}, {"Another pawn the gods do not care for any longer. A discarded toy like all of the elder races."}},
	{{"pyromancer"}, {"We are the keepers and shepherds of the elemental force of fire."}},
	{{"god"}, {"The ways of the gods are imprehensible to mortals. On the other hand, the elements are raw forces and can be understood and tamed."}},
	{{"keeper"}, {previous}},
	{{"shepherd"}, {previous}},
	{{"fire"}, {"Unlike the gods, the elements don't use mortals as toys, A skilled mind can understand and even control them to some extent."}},
	{{"flame"}, {previous}},
	{{"durin"}, {"Though we are through with the so-called gods, Durin, the first dwarf to aquire divine powers of his own, is considered a protector of our race."}},
	{{"life"}, {"Life feeds on fire and ultimately fire will feed on life."}},
	{{"plant"}, {"I don't care much about plants."}},
	{{"citizen"}, {"Many brave people are citizens of our town."}},
	{{"kroox"}, {"He is a smith. If you are looking for exquisite weapons and armour just talk to him."}},
	{{"jimbin"}, {"He and his wife are running the Jolly Axeman tavern."}},
	{{"maryza"}, {"She and her husband are running the Jolly Axeman tavern."}},
	{{"bezil"}, {"Bezil and Nezil are buying and selling equipment of all kinds."}},
	{{"nezil"}, {previous}},
	{{"uzgod"}, {"Uzgod is a weaponsmith just like those in the old legends."}},
	{{"etzel"}, {"Etzel is a true master of the elements. He is a role-model for our youngsters, jawoll."}},
	{{"gregor"}, {"The leader of the Thaian Knights' guild is a man of few words."}},
	{{"duria"}, {"She is the first knight of Kazordoon. She is responsible for teaching our young warriors how to handle an axe."}},
	{{"emperor"}, {"Our emperor has his halls in the upper caves."}},
	{{"kruzak"}, {previous}},
	{{"geomancer"}, {"They are followers of the path of earth."}},
	{{"technomancer"}, {"Those heretics believe they have discovered a new elemental force they can control easily. These fools, they'll bring doom on us all!"}},
	{{"motos"}, {"He is the fiercest axefighter of our times and a fine strategist."}},
	{{"general"}, {previous}},
	{{"army"}, {"Our armies can defend Kazordoon against any threat by means of its strong  fortifications."}},
	{{"ferumbras"}, {"If he ever dares enter Kazordoon I will gladly dump him into the lava. Tthe sacred flame shall bring justice upon him."}},
	{{"excalibug"}, {"A weapon too powerful to be wielded by mortals. It has to be returned to the fire which gave birth to it."}},
	{{"news"}, {"I am a busy man. I have no time for idle chitchat."}},
	{{"monster"}, {"May the great flame devour them all!"}},
	{{"fire", "devil"}, {"They mock the great flame by their existence. BLAST THEM ALL! Jawoll!"}},
	{{"help"}, {"I an not here to help; you have to help yourself."}},
	{{"quest"}, {"Ask around. There's a lot to do, jawoll."}},
	{{"task"}, {previous}},
	{{"what", "do"}, {previous}},
	{{"gold"}, {"Gold has been given birth to by the great flame. So it is wise to give some back to the fire now and then."}},
	{{"money"}, {previous}},
	{{"equipment"}, {"Bezil and Nezil are runing a shop where you can buy all the stuff you need."}},
	{{"fight"}, {"You should fight like fire, fearless and without mercy."}},

	{{"heal$", gt(BURNING, 0)}, {"You are burning. Take it as a blessing and don't cry like a baby, jawoll."}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"Weakling! If you are not prepared to face the heat, stay out of the fire! Oh all right, I will heal you a little.", setHp(40), effectOpp(13)}},

	{{"heal$", PVPENFORCED}, {"You aren't looking that bad."}},
	{{"heal$"}, {"You aren't looking that bad. Sorry, I can't help you. But if you are looking for additional protection you should go on the pilgrimage of ashes."}},

	{{"blessing", PVPENFORCED, FORCE}, {"The lifeforce of this world is wannig. There are no more blessings avaliable on this world."}},
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
	{{"blessing"}, {"There are five different blessings available in five sacred places. These blessings  are: the spiritual shielding, the spark of the phoenix, the embrace of tibia, the fire of the suns and the wisdom of solitude."}},
	{{"sacred", "places"}, {"Just ask in which of the five blessings you are interested in."}},
	{{"spiritual"}, {" You can receive the spiritual shielding in the whiteflowertemple south of thais."}},
	{{"shielding"}, {previous}},

	{{"spark", eq(questValue(44), 1), gt(count(3215), 0)}, {"Since the phoenix smiles upon you, you might receive this blessing for 9.000 gold while you have a phoenix egg with you. So are you ready?", setPrice(9000), setTopic(6)}},
	{{"phoenix", eq(questValue(44), 1), gt(count(3215), 0)}, {previous}},
	{{eq(TOPIC, 6), "yes", gt(questValue(102), 0), FORCE}, {"You already possess this blessing."}},
	{{eq(TOPIC, 6), "yes", lt(questValue(199), 1), FORCE}, {"You need the blessing of the great geomancer first."}},
	{{eq(TOPIC, 6), "yes", gt(count(3215), 0), lt(COUNTMONEY, PRICE), FORCE}, {"Oh. You do not have enough money."}},
	{{eq(TOPIC, 6), "yes", gt(count(3215), 0), FORCE}, {"So receive the mark of the flame and be blessed by the phoenix, pilgrim", deleteMoney, effectOpp(13), setQuestValue(102, 1), setQuestValue(199, 0)}},
	{{eq(TOPIC, 6), FORCE}, {"Perhaps another time."}},

	{{"spark", eq(questValue(44), 1), lt(count(3215), 1)}, {"Since the phoenix smiles upon you, could have received this blessing cheaper if you had a phoenix egg with you. But because you don't have it with you, its still 10.000 gold. Is that ok?", setPrice(10000), setTopic(7)}},
	{{"phoenix", eq(questValue(44), 1), lt(count(3215), 1)}, {previous}},
	{{eq(TOPIC, 7), "yes", gt(questValue(102), 0), FORCE}, {"You already possess this blessing."}},
	{{eq(TOPIC, 7), "yes", lt(questValue(199), 1), FORCE}, {"You need the blessing of the great geomancer first."}},
	{{eq(TOPIC, 7), "yes", lt(COUNTMONEY, PRICE), FORCE}, {"Oh. You do not have enough money."}},
	{{eq(TOPIC, 7), "yes", FORCE}, {"So receive the mark of the flame and be blessed by the phoenix, pilgrim", deleteMoney, effectOpp(13), setQuestValue(102, 1), setQuestValue(199, 0)}},
	{{eq(TOPIC, 7), FORCE}, {"Perhaps another time."}},

	{{"spark"}, {"The spark of the phoenix is given by me and by the great geomancer of the local earthtemple. Do you wish to receive my part of blessing of the phoenix for 10.000 gold?", setPrice(10000), setTopic(5)}},
	{{"phoenix"}, {previous}},

	{{"embrace"}, {"The druids north of Carlin will provide you with the embrace of tibia."}},
	-- "suns"     -> "You can ask for the blessing of the two suns in the suntower near Ab'Dendriel."
	-- nach oben gestellt wg. antwort auf fire
	{{"wisdom"}, {"Talk to the hermit Eremo on the isle of Cormaya about this blessing."}},
	{{"solitude"}, {previous}},

	{{eq(TOPIC, 5), "yes", gt(questValue(102), 0), FORCE}, {"You already possess this blessing."}},
	{{eq(TOPIC, 5), "yes", lt(questValue(199), 1), FORCE}, {"You need the blessing of the great geomancer first."}},
	{{eq(TOPIC, 5), "yes", lt(COUNTMONEY, PRICE), FORCE}, {"Oh. You do not have enough money."}},
	{{eq(TOPIC, 5), "yes", FORCE}, {"So receive the mark of the flame and be blessed by the phoenix, pilgrim.", deleteMoney, effectOpp(13), setQuestValue(102, 1), setQuestValue(199, 0)}},
	{{eq(TOPIC, 5), FORCE}, {"Perhaps another time."}},

	{{"time"}, {"It's the fourth age of the yellow flame."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
