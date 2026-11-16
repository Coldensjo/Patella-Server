-- npcs/arito.lua: the behaviour of the NPC Arito, converted from
-- arito.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of arito.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- arito.npc: Datenbank fuer den Wirt Arito

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", "frodo", FORCE}, {"Be mourned, pilgrim in flesh."}},
	{{ADDRESS, "hi$", "frodo", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please show some patience, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Please visit us again."}},

	{{"bye"}, {"Do visit us again.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the owner of this tavern."}},
	{{"tavern"}, {"This tavern is called the 'Old Scarab's Shell'."}},

	{{"name"}, {"My name is Arito."}},
	{{"time"}, {"It is exactly %T."}},
	{{"pharaoh"}, {"Blessed be our saviour."}},
	{{"tibianus"}, {"A foolish king who resides over foolish mortals."}},

	{{"army"}, {"Our army is strong and unyielding."}},
	{{"ferumbras"}, {"This servant of evil won't even dare to enter our city and to call the wrath of our pharaoh upon him."}},
	{{"arena"}, {"In the arena life challenges death. Death will be victorious in the end, but in the meantime there is much for the living to learn in preparation."}},
	{{"excalibug"}, {"Our pharaoh does not have any use for such a weapon. Powerful though it may be, it is nothing compared to his divine power."}},
	{{"thais"}, {"Thais is the capital of an insolent realm. Its people embrace life without understanding the alternative."}},
	{{"tibia"}, {"Why, this is our world of course."}},
	{{"carlin"}, {"Carlin is the twin sister of Thais. Another city that has not found the true path yet."}},

	{{"news"}, {"I've heard some blasphemous adventurers have excavated one of the ancient burial sites in the desert."}},
	{{"rumors"}, {previous}},

	{{"darama"}, {"This is our continent. Ankrahmun is its biggest and most marvelous city."}},
	{{"darashia"}, {"A city of the lost."}},
	{{"daraman"}, {"I know little about his heretic teachings."}},
	{{"ankrahmun"}, {"Our city is a marvel. It is the envy of the whole world."}},
	{{"city"}, {previous}},

	{{"pharaoh"}, {"Our pharaoh is our father, shepherd and teacher."}},
	{{"arkhothep"}, {previous}},
	{{"mortality"}, {"Mortality keeps us from finding our way to ascension."}},

	{{"ascension"}, {"For us mortals ascension is but a distant dream."}},
	{{"Akh'rah", "Uthun"}, {"The Akh'rah Uthun is what constitutes our self."}},
	{{"Akh"}, {"The Akh is our body, both in death and in life."}},

	{{"undead"}, {"Undeath is a blessing."}},
	{{"undeath"}, {previous}},
	{{"Rah"}, {"The Rah is our lifeforce. It is the source of our inner light."}},
	{{"uthun"}, {"The Uthun is what we learn and remember."}},
	{{"mourn"}, {"Mortality is a curse. That is why mortals have to be mourned."}},

	{{"arena"}, {"The arena is located close to the centre of Ankrahmun."}},
	{{"palace"}, {"The residence of our beloved pharaoh can be found to the south of the arena."}},
	{{"temple"}, {"The temple is to the east, not far from the shore."}},

	{{"buy"}, {"I can offer you bread, cheese, ham or meat."}},
	{{"offer"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"Looking for food? I have lemonade, wine, water, bread, cheese, ham, meat and fish."}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(8), "Would you like to buy bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(12), "Would you like to buy cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(10), "Would you like to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(16), "Would you like to buy a ham for %P gold?", setTopic(1)}},
	{{"fish"}, {setType(3578), setAmount(1), setPrice(6), "Would you like to buy a fish for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(8, P1)), "Would you like to buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(12, P1)), "Would you like to buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(10, P1)), "Would you like to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(16, P1)), "Would you like to buy %A ham for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "fish"}, {setType(3578), setAmount(P1), setPrice(mul(6, P1)), "Would you like to buy %A fishes for %P gold?", setTopic(1)}},

	{{"lemonade"}, {setType(2880), setData(12), setAmount(1), setPrice(3), "Do you want to buy a mug of lemonade for %P gold?", setTopic(1)}},
	{{"wine"}, {setType(2880), setData(2), setAmount(1), setPrice(4), "Do you want to buy a mug of wine for %P gold?", setTopic(1)}},
	{{"water"}, {setType(2880), setData(1), setAmount(1), setPrice(2), "Do you want to buy a mug of water for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
