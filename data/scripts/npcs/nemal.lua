-- npcs/nemal.lua: the behaviour of the NPC Nemal, converted from
-- nemal.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of nemal.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- nemal.npc: Datenbank für den Blinden Nemal (Desert)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local gt = behaviour.gt
local TOPIC = behaviour.TOPIC
local count = behaviour.count
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setData = behaviour.setData
local create = behaviour.create
local delete = behaviour.delete
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hello. How are you? Maybe you can help me: Do my shoes have the same colour?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Is there someone else? Step closer!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Where are you? Are you gone?"}},

	{{"bye"}, {"Farewell!", idle}},
	{{"farewell"}, {"May the light be with you!", idle}},
	{{"name"}, {"My name is Nemal."}},
	{{"job"}, {"I have no job. I'm a wanderer."}},
	{{"thais"}, {"Yes, I used to live there a while ago. Nice town."}},
	{{"carlin"}, {"Aaaahh, Carlin, yes. I know this town. Strange graveyard there, isn't it?"}},
	{{"king"}, {"King...king...yes, there is one. I still can remember the time when we had an other king. What was his name again?"}},
	{{"weapon"}, {"I always have weapons with me. You never know what's behind the next corner."}},
	{{"help"}, {"Hmm.. help? How can I help you?"}},
	{{"time"}, {"I had a watch. It was nice, but I can't see the hands anymore."}},
	{{"sword"}, {"I have a sword. It's very sharp. But I don't give it away, you never know."}},
	{{"desert"}, {"Desert? Where is one?"}},
	{{"excalibug"}, {"I heard the name, but I don't trust rumours."}},
	{{"fight"}, {"Better know how to fight!"}},
	{{"guild"}, {"Hmmm.. I wouldn't ever join a guild, but i know of the existance of some."}},
	{{"god"}, {"I don't believe in gods, but a lot of people do. I never saw a god, why should I trust in someone I never met?"}},
	{{"way"}, {"There are many ways. I don't know all of them."}},
	{{"door"}, {"Closed doors need keys. What a pity keys are not lying around like rubbish!"}},
	{{"library"}, {"I can't read or write. What use would I have of a library?"}},
	{{"secret"}, {"Secrets should remain secret. No need of making them public."}},
	{{"treasure"}, {"You can find some treasures here - some are bigger, some are smaller, some are of true value, some are of materialistic value."}},
	{{"book"}, {"I can't read. I've never learned it."}},
	{{"gharonk"}, {"My father knew this language."}},
	{{"offer"}, {"I don't sell things. If you really need something, better walk straight to one of the towns or ask another adventurer."}},
	{{"blind"}, {"Yes, I seem to be blind. But I am not sure - maybe the dungeons are too dark!"}},

	{{"potion", "regain", "vision"}, {"I heard of a potion of regained vision ... but I can't remember how to make it! Maybe you can help me. Do you know something about it?", setTopic(2)}},
	{{eq(TOPIC, 2), "yes"}, {"So, did you bring the ingredients with you, stranger?", setTopic(3)}},
	{{eq(TOPIC, 2)}, {"Oh. Maybe someone else could do it, then."}},
	{{eq(TOPIC, 3), "yes", gt(count(3601), 0), gt(count(3603), 0), gt(count(3604), 0), gt(count(3658), 0), gt(count(3590), 0)}, {"You seem to have them with you. Can you tell me, how many minutes I have to cook them?", setAmount(1), delete(3601), delete(3603), delete(3604), delete(3658), delete(3590), setTopic(4)}},
	{{eq(TOPIC, 3), "yes"}, {"It doesn't seem to me as if you have the correct ingredients with you, stranger!"}},
	{{eq(TOPIC, 3)}, {"Maybe you can find them!"}},
	{{eq(TOPIC, 4), "31"}, {"Ah. It seems to work. But what are the words I have to speak?", setTopic(5)}},
	{{eq(TOPIC, 4)}, {"Oh no, I don't think this is right."}},
	{{eq(TOPIC, 5), "nalus", "murtu"}, {"Thank you! NALUS MURTUUU! ... I can see again! To show you, how grateful I am, I'll give you a key. Be wise when using it. I can't tell you, where it matches, but ... take good care, it is useless without mental powers!", setData(4037), create(2969)}},
	{{eq(TOPIC, 5)}, {"Oh no, I don't think these are the right words."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
