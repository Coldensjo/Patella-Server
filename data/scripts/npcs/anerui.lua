-- npcs/anerui.lua: the behaviour of the NPC Anerui, converted from
-- anerui.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of anerui.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- anerui.npc: Datenbank für die Jägerin Anerui

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
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Ashari %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "Ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi.", idle}},
	{{"farewell"}, {previous}},
	{{"asha", "thrazi"}, {previous}},
	{{"name"}, {"I am Anerui Mourningleaf."}},
	{{"job"}, {"I am the mistress of the hunt. At this place you may buy the food our hunts provide."}},
	{{"time"}, {"Watch the sky, it will tell you."}},

	{{"carlin"}, {"Carlin needs our protection and resources. Of course they will turn on us as soon as they feel strong enough."}},
	{{"thais"}, {"Thais is to far away to prove a threat but also is of little help if problems should occur."}},
	{{"venore"}, {"Venore profits greatly from the trade with Ab'Denriel. I see those traders as leeches that suck away our resources."}},
	{{"roderick"}, {"He is our contact person to the thaian kingdom and a necessary evil."}},
	{{"olrik"}, {"He would love to be an elf but still is more kind of a pale orc."}},

	{{"hunter"}, {"Hunters live a life of freedom and closeness to nature, unlike a simple farmer or bugherder."}},
	{{"hunt"}, {"Hunting is an art, practiced too often by diletantes. Every fool with a bow or a spear considers himself a hunter."}},
	{{"game"}, {previous}},
	{{"prey"}, {previous}},
	{{"forest"}, {"The forests are the gardens of life. Nature provides enough for everyone's need, but not enough for everyone's greed."}},
	{{"nature"}, {"Nature is not a friend but an unforgiving teacher, and the lessons we have to learn are endless."}},
	{{"teacher"}, {"Most lessons nature teaches are about life and death."}},
	{{"life"}, {"Life and death are significant parts of the balance."}},
	{{"death"}, {previous}},
	{{"balance"}, {"The balance of nature, of course. It's everywhere, so don't ask but observe and learn."}},
	{{"bugherder"}, {"Well, a person who herds bugs of course."}},
	{{"bugs"}, {"The bugs provide us with chitin for equipment, bugmilk, and bugmeat."}},
	{{"bugmilk"}, {"It's delicious. Brasith sells it in his store."}},
	{{"bow"}, {"Bow, arrow, and spear are the hunters' best friends. In the northeast of the town one of us may sell such tools."}},
	{{"arrow"}, {previous}},
	{{"spear"}, {previous}},
	{{"elf"}, {"That is the race to which I belong."}},
	{{"elves"}, {previous}},
	{{"dwarf"}, {"I will never understand these little people of the mountains."}},
	{{"human"}, {"The humans are a loud and ugly race. They lack any grace and are more kin to the orcs then to us."}},
	{{"troll"}, {"I despise their presence in our town, but it may be a necessary evil."}},
	{{"cenath"}, {"The magic they wield is all that matters to them."}},
	{{"kuridai"}, {"The Kuridai are too agressive not only to people but also to the enviroment. They lack any understanding of the balance that we know as nature."}},
	{{"deraisim"}, {"We try to live in harmony with the forces of nature, may they be living or unliving."}},
	{{"abdaisim"}, {"The Abdaisim are our brothers and sisters in spirit. We stay in contact with them, exchanging news and items."}},
	{{"teshial"}, {"If they ever existed they are gone now."}},
	{{"ferumbras"}, {"The defiler. I will not talk about him."}},
	{{"crunor"}, {"I guess it's a human god for the human sight of nature. I have not much knowledge of this entity."}},

	{{"offer"}, {"I sell meat and ham."}},
	{{"goods"}, {previous}},
	{{"buy"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {previous}},

	{{"meat"}, {setType(3577), setAmount(1), setPrice(4), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(6), "Do you want to buy ham for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(4, P1)), "Do you wanna buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(6, P1)), "Do you wanna buy %A ham for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
