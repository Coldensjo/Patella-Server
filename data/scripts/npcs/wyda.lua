-- npcs/wyda.lua: the behaviour of the NPC Wyda, converted from
-- wyda.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of wyda.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- wyda.npc: Datenbank für die Hexe Wyda (Swamp)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local SORCERER = behaviour.SORCERER
local DRUID = behaviour.DRUID
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local count = behaviour.count
local setTopic = behaviour.setTopic
local create = behaviour.create
local delete = behaviour.delete
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", DRUID, ge(count(3065), 1), FORCE}, {"Welcome back, %N. Hey, nice wand you have there!"}},
	{{ADDRESS, "greet", DRUID, ge(count(3065), 1), FORCE}, {previous}},
	{{ADDRESS, "hello$", DRUID, FORCE}, {"Welcome to my hut, %N! It's always nice to see a druid here."}},
	{{ADDRESS, "greet", DRUID, FORCE}, {previous}},
	{{ADDRESS, "hello$", SORCERER, FORCE}, {"What do you want, %N?"}},
	{{ADDRESS, "greet", SORCERER, FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Good day, %N."}},
	{{ADDRESS, "greet", FORCE}, {previous}},
	{{ADDRESS, "good", "day", FORCE}, {previous}},
	{{ADDRESS, "hi$", FORCE}, {"What? Talking to me, %N?", idle}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"%N, just a moment, please.", queue}},
	{{BUSY, "greet", FORCE}, {previous}},
	{{BUSY, "good", "day", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Farewell."}},

	{{"bye"}, {"Good luck on your journeys.", idle}},
	{{"farewell"}, {previous}},
	{{"see", "you"}, {previous}},
	{{"job"}, {"I am a witch. Didn't you notice?"}},
	{{"name"}, {"My name is Wyda, and what's yours?"}},
	{{"my", "name", "is"}, {"Nice to meet you."}},
	{{"time"}, {"I think it is the fourth year after Queen Eloise's crowning, but I cannot tell you date or time."}},

	{{"sorcerer"}, {"Sorcerers have forgotten about the root of all beings: nature."}},
	{{"druid"}, {"Druids are mostly fine people. I'm always happy when I meet one."}},
	{{"knight"}, {"Knights succumb to the blindness of rage and the desire for violence and blood."}},
	{{"paladin"}, {"Paladins can use bows, but not brains."}},
	{{"queen"}, {"Eloise is Queen of Carlin. I don't care about royals much, as long as they don't try to tax me."}},
	{{"i$", "live"}, {"That's nice."}},
	{{"carlin"}, {"Carlin is a beautiful town, but far from here. Do you live there?"}},
	{{"thais"}, {"I've heard stories about that city. It's nowhere near here, that's all I can tell you about it."}},
	{{"stories"}, {"Thais is an overcrowded place inhabited by brutal murderers. At least that's what I've been told."}},
	{{"tibia$"}, {"Tibia is the name of our continent."}},
	{{"tibianus"}, {"Haha, that's a stupid name. Who's that?"}},
	{{"ferumbras"}, {previous}},
	{{"king"}, {"There are too many royals on this continent if you ask me..."}},
	{{"evil"}, {"Evilness doesn't scare me."}},
	{{"aureus"}, {"Aureus is a good friend who spends much time in this area!"}},
	{{"bridge"}, {"There's a bridge to the west, but it's guarded by dwarfs."}},
	{{"plains"}, {"Many tales exist about some so-called Plains of Havoc. It seems to be a dangerous place."}},
	{{"havoc"}, {previous}},
	{{"help"}, {"I can only help with knowledge. What do you want me to tell you about?"}},
	{{"hunter"}, {"To the east, there is a little settlement of hunters. They are cruel humans who attack everything they see."}},
	{{"buy"}, {"I'm currently not selling anything."}},
	{{"offer"}, {previous}},
	{{"sell"}, {"There's nothing I need right now, thanks."}},
	{{"key"}, {"I keep my keys where they belong - in my pocket."}},
	{{"monster"}, {"Many creatures live in, around, and beneath the swamp. Be careful!"}},
	{{"creature"}, {previous}},
	{{"swamp"}, {"Be careful of the swamp water, it's poisonous!"}},
	{{"nature"}, {"There are many swamp plants, mushrooms, and herbs around here."}},
	{{"plant"}, {"There are many kinds of swamp plants, some can be used for potions, some not."}},
	{{"potion"}, {"The recipe of the potions is one of the witches' secrets!"}},
	{{"secret"}, {previous}},
	{{"recipe"}, {previous}},
	{{"sister"}, {"Some sisters of mine are having a meeting nearby. Don't disturb them, or they will get angry and attack you."}},
	{{"witches"}, {previous}},
	{{"mushroom"}, {"Mushrooms taste good and are useful for potions."}},
	{{"heal"}, {"I do not have any potions for healing available right now."}},
	{{"giant", "spider"}, {"Yes, there is such a thing in the east, on a small island. It's very powerful."}},
	{{"beholder"}, {"Beholders? Strange creatures that have mysterious magical abilities."}},
	{{"slime"}, {"There's lots of slime around. It is said that they live from the swamp water."}},
	{{"god"}, {"I believe that nature itself is God."}},
	{{"magic"}, {"The magic of the witches is one of our secrets!"}},
	{{"spell"}, {previous}},
	{{"weatherwax"}, {"I think I've heard that name before..."}},
	{{"ogg"}, {previous}},
	{{"voodoo"}, {"I don't practice such nonsense, that's just a rumour."}},
	{{"coffin"}, {"That's none of your business."}},
	{{"dwarf"}, {"The little bearded fellows have a town somewhere in the northwest."}},
	{{"dwarves"}, {previous}},
	{{"little", "fellows"}, {previous}},
	{{"kazordoon"}, {"Isn't that the name of the little bearded fellows' town?"}},
	{{"gold"}, {"Money means nothing to me."}},
	{{"platin"}, {previous}},
	{{"cookie"}, {"I bake cookies now and then in my spare time."}},
	{{"orange"}, {"I love exotic fruits. I have oranges imported from the south sometimes, but that's very expensive."}},
	{{"fly", "broom", FORCE}, {"Haha, no... where did you get that idea? I use it to sweep my platform."}},
	{{"ride", "broom", FORCE}, {previous}},
	{{"broom", "fly", FORCE}, {previous}},
	{{"broom"}, {"What about it?"}},
	{{"platform"}, {"This platform and house were built by my mother, long ago."}},
	{{"mother"}, {"Of course my mother was also a witch!"}},
	{{"crystal", "ball"}, {"It's a magical item that only witches can use."}},
	{{"black", "knight"}, {"A black knight? Black is the color of witches, why whould any knight carry black?"}},
	{{"earthquake"}, {"The earth in this region shakes now and then. Foolish people think that this is because the Gods are angry."}},

	{{"become", "witch", FEMALE, FORCE}, {"You can't just become a witch. Either you are or you aren't - and YOU obviously aren't!"}},
	{{"become", "witch", MALE, FORCE}, {"You're a MAN!"}},
	{{"witch$"}, {"Aye, I am a witch."}},
	{{"man$"}, {"There are only female witches."}},
	{{"men$"}, {previous}},

	{{"power", "wand", FORCE}, {"The power of the wand can only be used by witches."}},
	{{"use", "wand", FORCE}, {previous}},
	{{"what", "do", "wand", FORCE}, {previous}},
	{{"wand"}, {"I use a wooden spellwand. Why are you asking?"}},
	{{"spellwand"}, {previous}},

	{{"quest"}, {"A quest? Well, if you're so keen on doing me a favour... Why don't you try to find a blood herb?"}},
	{{"herbs"}, {"The swamp is home to a wide variety of herbs, but the most famous is the blood herb."}},
	{{"blood", "herb$", ge(count(3734), 1), FORCE}, {"Do you have a blood herb for me?", setTopic(1)}},
	{{"blood", "herb$"}, {"The blood herb is very rare. This plant would be very useful for me, but I don't know any accessible places to find it."}},

	{{eq(TOPIC, 1), "yes", lt(count(3734), 1), FORCE}, {"Well, do you own one or not?"}},
	--Topic=1,"yes",Sorcerer -> Price=400, "Hmm, thanks. Take this.", Delete(3734), CreateMoney
	--Topic=1,"yes",Druid    -> "Thank you so much! Here, let me give you a reward...", Delete(3734), Create(3065)
	--Topic=1,"yes"          -> Price=300, "Thank you! Here are some coins for your help.", Delete(3734), CreateMoney
	{{eq(TOPIC, 1), "yes"}, {"Thank you so much! Here, let me give you a reward...", delete(3734), create(3211)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
