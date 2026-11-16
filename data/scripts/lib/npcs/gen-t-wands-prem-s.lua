-- lib/npcs/gen-t-wands-prem-s.lua: the rule block gen-t-wands-prem-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-wands-prem-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-wands-prem-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-wands-prem-s.ndb: Datenbank für generischen Zauberstabverkauf - Premium Account Wands
-- Verwendete Topics: 90,91,92

local behaviour = require("lib.npc")
local SORCERER = behaviour.SORCERER
local DRUID = behaviour.DRUID
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney

return behaviour.rules {
	{{"wand"}, {"Wands can be wielded by sorcerers only and have a certain level requirement. There are five different wands, would you like to hear about them?", setTopic(90)}},
	{{"rod"}, {"Rods can be wielded by druids only and have a certain level requirement. There are five different rods, would you like to hear about them?", setTopic(91)}},

	{{eq(TOPIC, 90), "yes"}, {"The names of the wands are 'Wand of Vortex', 'Wand of Dragonbreath', 'Wand of Plague', 'Wand of Cosmic Energy' and 'Wand of Inferno'. Which one would you like to buy?"}},
	{{eq(TOPIC, 90), "no"}, {"Maybe another time."}},
	{{eq(TOPIC, 90)}, {"Maybe another time."}},

	{{eq(TOPIC, 91), "yes"}, {"The names of the rods are 'Snakebite Rod', 'Moonlight Rod', 'Volcanic Rod', 'Quagmire Rod', and 'Tempest Rod'. Which one would you like to buy?"}},
	{{eq(TOPIC, 91), "no"}, {"Maybe another time."}},
	{{eq(TOPIC, 91)}, {"Maybe another time."}},

	{{SORCERER, "wand", "of", "vortex", lt(questValue(333), 1)}, {"Oh, is this your first wand of vortex? Take this little present from me as a free sample!", setQuestValue(333, 1), setType(3074), setAmount(1), create(TYPE)}},
	{{DRUID, "snakebite", "rod", lt(questValue(333), 1)}, {"Oh, is this your first snakebite rod? Take this little present from me as a free sample!", setQuestValue(333, 1), setType(3066), setAmount(1), create(TYPE)}},

	{{"wand", "of", "vortex"}, {setType(3074), setAmount(1), setPrice(500), "This wand is only for sorcerers of level 7 and above. Would you like to buy a wand of vortex for %P gold?", setTopic(92)}},
	{{"wand", "of", "dragonbreath"}, {setType(3075), setAmount(1), setPrice(1000), "This wand is only for sorcerers of level 13 and above. Would you like to buy a wand of dragonbreath for %P gold?", setTopic(92)}},
	{{"wand", "of", "plague"}, {setType(3072), setAmount(1), setPrice(5000), "This wand is only for sorcerers of level 19 and above. Would you like to buy a wand of plague for %P gold?", setTopic(92)}},
	{{"wand", "of", "cosmic", "energy"}, {setType(3073), setAmount(1), setPrice(10000), "This wand is only for sorcerers of level 26 and above. Would you like to buy a wand of cosmic energy for %P gold?", setTopic(92)}},
	{{"wand", "of", "inferno"}, {setType(3071), setAmount(1), setPrice(15000), "This wand is only for sorcerers of level 33 and above. Would you like to buy a wand of inferno for %P gold?", setTopic(92)}},

	{{"snakebite", "rod"}, {setType(3066), setAmount(1), setPrice(500), "This rod is only for druids of level 7 and above. Would you like to buy a snakebite rod for %P gold?", setTopic(92)}},
	{{"moonlight", "rod"}, {setType(3070), setAmount(1), setPrice(1000), "This rod is only for druids of level 13 and above. Would you like to buy a moonlight rod for %P gold?", setTopic(92)}},
	{{"volcanic", "rod"}, {setType(3069), setAmount(1), setPrice(5000), "This rod is only for druids of level 19 and above. Would you like to buy a volcanic rod for %P gold?", setTopic(92)}},
	{{"quagmire", "rod"}, {setType(3065), setAmount(1), setPrice(10000), "This rod is only for druids of level 26 and above. Would you like to buy a quagmire rod for %P gold?", setTopic(92)}},
	{{"tempest", "rod"}, {setType(3067), setAmount(1), setPrice(15000), "This rod is only for druids of level 33 and above. Would you like to buy a tempest rod for %P gold?", setTopic(92)}},

	{{P1, lt(1, P1), "wand", "of", "vortex"}, {setType(3074), setAmount(P1), setPrice(mul(500, P1)), "This wand is only for sorcerers of level 7 and above. Would you like to buy %A wands of vortex for %P gold?", setTopic(92)}},
	{{P1, lt(1, P1), "wand", "of", "dragonbreath"}, {setType(3075), setAmount(P1), setPrice(mul(1000, P1)), "This wand is only for sorcerers of level 13 and above. Would you like to buy %A wands of dragonbreath for %P gold?", setTopic(92)}},
	{{P1, lt(1, P1), "wand", "of", "plague"}, {setType(3072), setAmount(P1), setPrice(mul(5000, P1)), "This wand is only for sorcerers of level 19 and above. Would you like to buy %A wands of plague for %P gold?", setTopic(92)}},
	{{P1, lt(1, P1), "wand", "of", "cosmic", "energy"}, {setType(3073), setAmount(P1), setPrice(mul(10000, P1)), "This wand is only for sorcerers of level 26 and above. Would you like to buy %A wands of cosmic energy for %P gold?", setTopic(92)}},
	{{P1, lt(1, P1), "wand", "of", "inferno"}, {setType(3071), setAmount(P1), setPrice(mul(15000, P1)), "This wand is only for sorcerers of level 33 and above. Would you like to buy %A wands of inferno for %P gold?", setTopic(92)}},

	{{P1, lt(1, P1), "snakebite", "rod"}, {setType(3066), setAmount(P1), setPrice(mul(500, P1)), "This rod is only for druids of level 7 and above. Would you like to buy %A snakebite rods for %P gold?", setTopic(92)}},
	{{P1, lt(1, P1), "moonlight", "rod"}, {setType(3070), setAmount(P1), setPrice(mul(1000, P1)), "This rod is only for druids of level 13 and above. Would you like to buy %A moonlight rods for %P gold?", setTopic(92)}},
	{{P1, lt(1, P1), "volcanic", "rod"}, {setType(3069), setAmount(P1), setPrice(mul(5000, P1)), "This rod is only for druids of level 19 and above. Would you like to buy %A volcanic rods for %P gold?", setTopic(92)}},
	{{P1, lt(1, P1), "quagmire", "rod"}, {setType(3065), setAmount(P1), setPrice(mul(10000, P1)), "This rod is only for druids of level 26 and above. Would you like to buy %A quagmire rods for %P gold?", setTopic(92)}},
	{{P1, lt(1, P1), "tempest", "rod"}, {setType(3067), setAmount(P1), setPrice(mul(15000, P1)), "This rod is only for druids of level 33 and above. Would you like to buy %A tempest rods for %P gold?", setTopic(92)}},

	{{eq(TOPIC, 92), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 92), "yes"}, {"Sorry, you don't have enough gold."}},
	{{eq(TOPIC, 92)}, {"You don't know what you're missing."}},
}
