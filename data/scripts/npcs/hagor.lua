-- npcs/hagor.lua: the behaviour of the NPC Hagor, converted from
-- hagor.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of hagor.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- hagor.npc: Datenbank für den alten Abenteurer Hagor (Desert)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local count = behaviour.count
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setData = behaviour.setData
local create = behaviour.create
local delete = behaviour.delete
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, adventurer %N. What leads you to me?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"name"}, {"My name is Hagor, the old hunter."}},
	{{"job"}, {"I travel through the lands of Tibia and now Jakundaf Desert since years."}},
	{{"tibia"}, {"The gods made this world full of fascinating secrets and I will search them till my end."}},
	{{"thais"}, {"Thais... It's a big city in Middle-Tibia. Lots of people live there."}},
	{{"carlin"}, {"Is this the city to the north? I heard rumours about it."}},
	{{"king"}, {"Tibianus is our king. To be honest, I didn't hear much of him till now."}},
	{{"weapon"}, {"Weapons? Do you have some? You better know how to use them!"}},
	{{"help"}, {"I'd really like to help you. Could you specify your request?"}},
	{{"time"}, {"Time has no meaning to me."}},
	{{"sword"}, {"Do you mean any sword in particular? Or just any sword?"}},
	{{"desert"}, {"Yes, it's big, isn't it?"}},
	{{"excalibug"}, {"I heared rumours that there is a sword called so. I don't know if it exists."}},
	{{"fight"}, {"Fighting is an art. Know it and you will be strong, ignore it and you will die soon!"}},
	{{"guild"}, {"There are many different guilds in Tibia. They come and go, come and go..."}},
	{{"god"}, {"There are a lot of gods we believe in. Maybe you should check out different books to find out something about them."}},
	{{"way"}, {"Which way are you looking for in particular?"}},
	{{"door"}, {"Which door are you talking about? If it is locked, maybe you should try to find a key for it!"}},
	{{"secret"}, {"There are many secrets. But I fear, I can't tell you much about them. They are also secret to me..."}},
	{{"treasure"}, {"Someone told me - I can't remember who it was - that there was a treasure hidden nearby."}},
	{{"book"}, {"Yes, I really can recommend reading books. It might help you find what you are looking for!"}},
	{{"gharonk"}, {"Gharonk is a very old language, only spoken by a few people. It's not a very complex language, but that does not mean that it is easy to understand!"}},
	{{"offer"}, {"Go ahead. I don't sell or buy anything!"}},
	{{"exit"}, {"Yes, there is an exit for these dungeons. Just find the teleporter."}},
	{{"library"}, {"There is a library in here, right. I assume you are talking about this library. It's locked, as far as I know. But somewhere there has to be a key... maybe the librarian knows more?"}},
	{{"roll"}, {"Oh, yes, I love them!"}},

	{{"morrin"}, {"Ah, I remember that man. We made a deal, guess about what.", setTopic(1)}},
	{{eq(TOPIC, 1), "key"}, {"Right! We can make the same deal if you give a fresh delicious roll. Do you have any?", setTopic(2)}},
	{{eq(TOPIC, 2), "yes", ge(count(3601), 1)}, {"Oh, fine! Here you are.", setAmount(1), delete(3601), setData(4022), create(2969)}},
	{{eq(TOPIC, 2), "yes"}, {"Hey, you do not have one!"}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
