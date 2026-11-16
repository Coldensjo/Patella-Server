-- npcs/zaidal.lua: the behaviour of the NPC Zaidal, converted from
-- zaidal.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of zaidal.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- zaildal.npc: Datenbank für den Bambusmöbelhändler Zaidal

local behaviour = require("lib.npc")
local genTFurnitureJungleS = require("lib.npcs.gen-t-furniture-jungle-s")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hello."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Please wait.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am selling furniture, especially bamboo made furniture. I also buy elephant tusks to create my famous tusk tables and ivory chairs."}},
	{{"name"}, {"I am known as Zaidal."}},
	{{"time"}, {"Sorry, I have no idea."}},
	{{"king"}, {"Perhaps one day even the king will use our furniture."}},
	{{"venore"}, {"Well, Venore is the centre of commerce."}},
	{{"thais"}, {"One day I might visit Thais."}},
	{{"carlin"}, {"Carlin is very far to the north. I have never been there."}},
	{{"edron"}, {"An isle with interesting forests. With the right organisation the furniture and shipbuilding business could prosper enormously."}},
	{{"jungle"}, {"You call it a jungle, I call it furniture in the making."}},

	{{"offer"}, {"I sell tables, chairs and drawers, all handmade with the material that the jungle has to offer."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},
	{{"special"}, {"My offers are permanently extraordinarily cheap."}},

	{{"tibia"}, {"The world is a big treasure chest for those who know to turn resources into profit."}},

	{{"kazordoon"}, {"The dwarves of Kazordoon are experts in mining resources like gems and ore."}},
	{{"dwarvs"}, {previous}},
	{{"dwarfes"}, {previous}},
	{{"ab'dendriel"}, {"Those elves are a bit complicated when it comes to trees and environmental matters. They must learn that you have to crush an egg to bake a cake."}},
	{{"elves"}, {previous}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"The jungle is rich in resources and who knows what profit lies hidden in the desert? Of course not for a carpenter, but there are other resources."}},
	{{"darashia"}, {"People there seem to know little about the world of economy. Perhaps someone might teach them a lesson one day."}},
	{{"ankrahmun"}, {"I was there only once, but I left it with the certainty that I never want to return there ever again."}},
	{{"ferumbras"}, {"If we could guide his attention in useful directions, he wouldn't be the problem he poses nowadays."}},
	{{"excalibug"}, {"Even if it would exist, which I doubt, it would be only an extremely expensive weapon and nothing more."}},
	{{"apes"}, {"They live in the depth of the jungle, and their only visits here are annoying raids to steal and plunder."}},
	{{"lizard"}, {"I did not see much of the lizzards yet."}},
	{{"dworcs"}, {"If we could get rid of them, a whole new area which is rich in bamboo would be ours."}},

	{{"sell", "tusk"}, {setType(3044), setAmount(1), setPrice(100), "Do you want to sell a tusk for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "tusk"}, {setType(3044), setAmount(P1), setPrice(mul(100, P1)), "Do you want to sell %A tusks for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},

	genTFurnitureJungleS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
