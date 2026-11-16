-- lib/npcs/gen-t-weapon-s.lua: the rule block gen-t-weapon-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-weapon-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-weapon-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-weapon-s.ndb: Datenbank für generischen Waffenverkauf

local behaviour = require("lib.npc")
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney

return behaviour.rules {
	{{"weapon"}, {"I have hand axes, axes, spears, maces, battle hammers, battle axes, sickles, short swords, swords, carlin swords, two handed swords, rapiers, daggers, clubs, morning stars and sabres. What's your choice?"}},

	{{"sickle"}, {setType(3293), setAmount(1), setPrice(8), "Do you want to buy a sickle for %P gold?", setTopic(33)}},
	{{"short", "sword"}, {setType(3294), setAmount(1), setPrice(30), "Do you want to buy a short sword for %P gold?", setTopic(33)}},
	{{"dagger"}, {setType(3267), setAmount(1), setPrice(5), "Do you want to buy a dagger for %P gold?", setTopic(33)}},
	{{"hand", "axe"}, {setType(3268), setAmount(1), setPrice(8), "Do you want to buy a hand axe for %P gold?", setTopic(33)}},
	{{"spear"}, {setType(3277), setAmount(1), setPrice(10), "Do you want to buy a spear for %P gold?", setTopic(33)}},
	{{"rapier"}, {setType(3272), setAmount(1), setPrice(15), "Do you want to buy a rapier for %P gold?", setTopic(33)}},
	{{"axe"}, {setType(3274), setAmount(1), setPrice(20), "Do you want to buy an axe for %P gold?", setTopic(33)}},
	{{"sabre"}, {setType(3273), setAmount(1), setPrice(35), "Do you want to buy a sabre for %P gold?", setTopic(33)}},
	{{"sword"}, {setType(3264), setAmount(1), setPrice(85), "Do you want to buy a sword for %P gold?", setTopic(33)}},
	{{"mace"}, {setType(3286), setAmount(1), setPrice(90), "Do you want to buy a mace for %P gold?", setTopic(33)}},
	{{"club"}, {setType(3270), setAmount(1), setPrice(5), "Do you want to buy a club for %P gold?", setTopic(33)}},
	{{"battle", "hammer"}, {setType(3305), setAmount(1), setPrice(350), "Do you want to buy a battle hammer for %P gold?", setTopic(33)}},
	{{"battle", "axe"}, {setType(3266), setAmount(1), setPrice(235), "Do you want to buy a battle axe for %P gold?", setTopic(33)}},
	{{"morning", "star"}, {setType(3282), setAmount(1), setPrice(430), "Do you want to buy a morning star for %P gold?", setTopic(33)}},
	{{"two", "handed", "sword"}, {setType(3265), setAmount(1), setPrice(950), "Do you want to buy a two handed sword for %P gold?", setTopic(33)}},
	{{"carlin", "sword"}, {setType(3283), setAmount(1), setPrice(473), "Do you want to buy one of the excellent carlin swords for %P gold?", setTopic(33)}},

	{{eq(TOPIC, 33), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 33), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 33)}, {"Maybe you will buy it another time."}},
}
