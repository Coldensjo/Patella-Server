-- lib/npcs/gen-t-furniture-decoration-s.lua: the rule block gen-t-furniture-decoration-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-furniture-decoration-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-furniture-decoration-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-furniture-decoration-s.ndb: Datenbank für generischen Möbelverkauf - Dekoration
-- Verwendete Topics: 81
--"offer"           -> "I sell statues, tables, chairs, flowers, pillows, pottery, instruments, decoration, tapestries and containers."
--"furniture"       -> *
--"goods"           -> *
--"do","you","sell" -> *
--"do","you","have" -> *

local behaviour = require("lib.npc")
local PREMIUM = behaviour.PREMIUM
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
	{{"decoration"}, {"I can offer water pipes, pendulum clock, telescopes, table lamps, rocking horses, globes and birdcages. I also sell wall hangings."}},
	{{"water", "pipe"}, {setType(2974), setAmount(1), setPrice(40), "You want to buy a water pipe for %P gold?", setTopic(81)}},
	{{"pendulum", "clock"}, {setType(2801), setAmount(1), setPrice(75), "You want to buy a pendulum clock for %P gold?", setTopic(81)}},
	{{"telescope"}, {setType(2799), setAmount(1), setPrice(70), "You want to buy a telescope for %P gold?", setTopic(81)}},
	{{"table", "lamp"}, {setType(2798), setAmount(1), setPrice(35), "You want to buy a table lamp for %P gold?", setTopic(81)}},
	{{"rocking", "horse"}, {setType(2800), setAmount(1), setPrice(30), "You want to buy a rocking horse for %P gold?", setTopic(81)}},
	{{"globe"}, {setType(2797), setAmount(1), setPrice(50), "You want to buy a globe for %P gold?", setTopic(81)}},
	{{"birdcage"}, {setType(2796), setAmount(1), setPrice(50), "You want to buy a birdcage for %P gold?", setTopic(81)}},
	{{"wall", "hangings"}, {"I can offer mirrors, paintings and cuckoo clocks."}},
	{{"cuckoo"}, {setType(2664), setAmount(1), setPrice(40), "You want to buy a cuckoo clock for %P gold?", setTopic(81)}},
	{{"painting"}, {"Would you like a landscape, a portrait or a still life. What is your choice?"}},
	{{"portrait"}, {setType(2641), setAmount(1), setPrice(50), "You want to buy a portrait picture for %P gold?", setTopic(81)}},
	{{"landscape"}, {setType(2639), setAmount(1), setPrice(50), "You want to buy a landscape picture for %P gold?", setTopic(81)}},
	{{"still", "life"}, {setType(2640), setAmount(1), setPrice(50), "You want to buy a still life picture for %P gold?", setTopic(81)}},
	{{"mirror"}, {"I sell round mirrors, oval mirrors and edged mirrors. Which one might it be?"}},
	{{"round", "mirror"}, {setType(2632), setAmount(1), setPrice(40), "You want to buy a round mirror for %P gold?", setTopic(81)}},
	{{"oval", "mirror"}, {setType(2638), setAmount(1), setPrice(40), "You want to buy a oval mirror for %P gold?", setTopic(81)}},
	{{"edged", "mirror"}, {setType(2635), setAmount(1), setPrice(40), "You want to buy a edged mirror for %P gold?", setTopic(81)}},

	{{eq(TOPIC, 81), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 81), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 81)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},
}
