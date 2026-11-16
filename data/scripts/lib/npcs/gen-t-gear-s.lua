-- lib/npcs/gen-t-gear-s.lua: the rule block gen-t-gear-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-gear-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-gear-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- genTgearV.ndb:  Datenbank für generischen Ausrüstungsverkauf

local behaviour = require("lib.npc")
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local previous = behaviour.previous

return behaviour.rules {
	{{"torch"}, {setType(2920), setAmount(1), setPrice(2), "Do you want to buy a torch for %P gold?", setTopic(25)}},
	{{"candelab"}, {setType(2911), setAmount(1), setPrice(8), "Do you want to buy a candelabrum for %P gold?", setTopic(25)}},
	{{"candlestick"}, {setType(2917), setAmount(1), setPrice(2), "Do you want to buy a candlestick for %P gold?", setTopic(25)}},
	{{"bag"}, {setType(2862), setAmount(1), setPrice(4), "Do you want to buy a bag for %P gold?", setTopic(25)}},
	{{"scroll"}, {setType(2815), setAmount(1), setPrice(5), "Do you want to buy a scroll for %P gold?", setTopic(25)}},
	{{"document"}, {setType(2834), setAmount(1), setPrice(12), "Do you want to buy a document for %P gold?", setTopic(25)}},
	{{"parchment"}, {setType(2835), setAmount(1), setPrice(8), "Do you want to buy a parchment for %P gold?", setTopic(25)}},
	{{"shovel"}, {setType(3457), setAmount(1), setPrice(10), "Do you want to buy a shovel for %P gold?", setTopic(25)}},
	{{"backpack"}, {setType(2870), setAmount(1), setPrice(10), "Do you want to buy a backpack for %P gold?", setTopic(25)}},
	{{"scythe"}, {setType(3453), setAmount(1), setPrice(12), "Do you want to buy a scythe for %P gold?", setTopic(25)}},
	{{"pick"}, {setType(3456), setAmount(1), setPrice(50), "Do you want to buy a pick for %P gold?", setTopic(25)}},
	{{"watch"}, {setType(2906), setAmount(1), setPrice(20), "Do you want to buy one of my high quality watches for %P gold?", setTopic(25)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(50), "Do you want to buy a rope for %P gold?", setTopic(25)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(150), "Do you want to buy a fishing rod for %P gold?", setTopic(25)}},
	{{"crowbar"}, {setType(3304), setAmount(1), setPrice(260), "Do you want to buy a dwarfish steel crowbar for %P gold?", setTopic(25)}},
	{{"water", "hose"}, {setType(2901), setAmount(1), setPrice(10), setData(1), "Do you want to' buy a water hose for %P gold?", setTopic(25)}},
	{{"present"}, {setType(2856), setAmount(1), setPrice(10), "Do you want to buy a present for %P gold?", setTopic(25)}},
	{{"bucket"}, {setType(2873), setAmount(1), setPrice(4), setData(0), "Do you want to buy a bucket for %P gold?", setTopic(25)}},
	{{"bottle"}, {setType(2875), setAmount(1), setPrice(3), setData(0), "Do you want to buy a bottle for %P gold?", setTopic(25)}},
	{{P1, lt(0, P1), "torches"}, {setType(2920), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A torches for %P gold?", setTopic(25)}},

	{{"oil"}, {setType(2874), setData(7), setAmount(1), setPrice(20), "Do you want to buy oil for %P gold?", setTopic(26)}},

	{{"deposit"}, {"I will give you 5 gold for every empty vial. Ok?", setData(0), setTopic(27)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 25), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is!", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 25), "yes"}, {"Sorry, you have no money!"}},
	{{eq(TOPIC, 25)}, {"Perhaps another time."}},

	{{eq(TOPIC, 26), "yes", ge(COUNTMONEY, PRICE)}, {"Ok, take it. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 26), "yes"}, {"Sorry, you have no money!"}},
	{{eq(TOPIC, 26)}, {"Perhaps another time."}},

	{{eq(TOPIC, 27), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 27), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 27)}, {"Hmm, but please keep this place litter free."}},
}
