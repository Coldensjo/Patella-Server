-- lib/npcs/gen-post.lua: the rule block gen-post.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-post.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-post.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-post.ndb:  Datenbank für generischen Postler

local behaviour = require("lib.npc")
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney

return behaviour.rules {
	{{"mail"}, {"Our mail system is unique! And everyone can use it. Do you want to know more about it?", setTopic(35)}},
	{{"depot"}, {"The depots are very easy to use. Just step in front of them and you will find your items in them. They are free for all tibian citizens."}},
	{{"offer"}, {"I'm selling letters and parcels."}},

	{{"letter", gt(questValue(250), 0)}, {setAmount(1), setPrice(5), "Do you want to buy a letter for %P gold?", setTopic(36)}},
	{{"parcel", gt(questValue(250), 0)}, {setAmount(1), setPrice(10), "Do you want to buy a parcel for %P gold?", setTopic(37)}},

	{{"letter"}, {setAmount(1), setPrice(8), "Do you want to buy a letter for %P gold?", setTopic(36)}},
	{{"parcel"}, {setAmount(1), setPrice(15), "Do you want to buy a parcel for %P gold?", setTopic(37)}},

	{{P1, lt(1, P1), "letter", gt(questValue(250), 0)}, {setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A letters for %P gold?", setTopic(36)}},
	{{P1, lt(1, P1), "parcel", gt(questValue(250), 0)}, {setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A parcels for %P gold?", setTopic(37)}},

	{{P1, lt(1, P1), "letter"}, {setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A letters for %P gold?", setTopic(36)}},
	{{P1, lt(1, P1), "parcel"}, {setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A parcels for %P gold?", setTopic(37)}},

	{{eq(TOPIC, 35), "yes"}, {"The Tibia Mail System enables you to send and receive letters and parcels. You can buy them here if you want."}},
	{{eq(TOPIC, 35)}, {"Is there anything else I can do for you?"}},

	{{eq(TOPIC, 36), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is. Don't forget to write the name of the receiver in the first line and the address in the second one before you put the letter in a mailbox.", deleteMoney, create(3505)}},
	{{eq(TOPIC, 36), "yes"}, {"Oh, you have not enough gold to buy a letter."}},
	{{eq(TOPIC, 36)}, {"Ok."}},

	{{eq(TOPIC, 37), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. Don't forget to write the name and the address of the receiver on the label. The label has to be in the parcel before you put the parcel in a mailbox.", deleteMoney, create(3503), create(3507)}},
	{{eq(TOPIC, 37), "yes"}, {"I am sorry, you have not enough gold to buy a parcel."}},
	{{eq(TOPIC, 37)}, {"Ok."}},
}
