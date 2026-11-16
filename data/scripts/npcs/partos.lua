-- npcs/partos.lua: the behaviour of the NPC Partos, converted from
-- partos.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of partos.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- partos.npc: Datenbank für den Sträfling Partos

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local questValue = behaviour.questValue
local setQuestValue = behaviour.setQuestValue
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome to my little kingdom, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N, don't go away, I am ready soon and don't get visitors often.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"I wish I could do that, too."}},

	{{"bye"}, {"Good bye, visit me again. I will be here, promised.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"Guess it! I give you a hint: I am not in this cell to clean it up! ...", "I wished, I would have never left Ankrahmun."}},
	{{"news"}, {"I hardly hear any news down here."}},
	{{"name"}, {"My name is Partos, but you can call me Party."}},
	{{"party"}, {"Yeah! Come in and let's have a party."}},
	{{"thais"}, {"I love the city. I just wish I could see some other part of it now and then."}},
	{{"city"}, {previous}},
	{{"tibia"}, {"I love this world. I just wish I could see some other part of it now and then."}},
	{{"how", "are", "you"}, {"I am great! Free food, free room, and now and then someone coming down here to ask me silly questions. Wouldn't you love that, too?"}},
	{{"sell"}, {"I would like to sell you a secret, but I'm out of business for too long."}},
	{{"jail"}, {"You mean that's a JAIL? They told me it's the finest hotel in town! THAT explains the lousy roomservice!"}},
	{{"prison"}, {previous}},
	{{"crime"}, {"Bah, I did nothing serious. I just had a little fun. In Ankrahmun nobody would have cared about these kind of things..."}},
	{{"criminal"}, {previous}},
	{{"god$"}, {"The gods seldom show up down here, so don't ask me."}},
	{{"gods$"}, {previous}},
	{{"citizen"}, {"Rich enough to spare a little, don't you agree? Well, they didn't agree."}},
	{{"king"}, {"Yeah, a king is a man that can rob people by law, and not by night like me."}},
	{{"monster"}, {"At least I am safe from them down here."}},
	{{"gold"}, {"Gold got me in here."}},
	{{"money"}, {previous}},
	{{"fight"}, {"Hey, most people I killed were even worse than me."}},
	{{"slay"}, {previous}},
	{{"noodles"}, {"I bet one could get some fine ransom, if he dognappes this furball."}},
	{{"quentin"}, {"By the gods, he visits us 'criminals' now and then to 'save' us. Who is going to save me from this boredom on two legs?"}},
	{{"army"}, {"Bah, the king's pawns. I spit on them."}},
	{{"time"}, {"Geee, someone stole my watch. Bad company down here."}},
	{{"ankrahmun"}, {"Yes, I've lived in Ankrahmun for quite some time. Ahh, good old times! ...", "Unfortunately I had to relocate. <sigh> ...", "Business reasons - you know."}},

	{{"waterpipe"}, {"My waterpipe? I lost it. But it doesn't matter. I quit smoking anyway."}},
	{{"djinn", eq(questValue(286), 1), FORCE}, {"What!? I bet, Baa'leal sent you! ...", "I won't tell you anything! Shove off!", setQuestValue(286, 2), idle}},
	{{"baa'leal", eq(questValue(286), 1), FORCE}, {previous}},
	{{"supplies", eq(questValue(286), 1), FORCE}, {previous}},
	{{"mal'ouquah", eq(questValue(286), 1), FORCE}, {previous}},

	{{"djinn"}, {"I won't talk about that."}},
	{{"baa'leal"}, {previous}},
	{{"supplies"}, {previous}},
	{{"mal'ouquah"}, {previous}},

	--"excalibug"      -> "Excalibug? No way that I tell you something about it!"
	--"grapes"         -> Type=3592, Amount=1, "Do you have any grapes with you?", Topic=1

	--Topic=1,"yes",Count(Type)>=Amount -> "What do you want for that ...ohhh... tasty ...uhm... sweet ...drool... delicous ...hmm... grapes?", Delete(Type), Topic=2
	--Topic=1,"yes"                             -> "Go away, if you don't have any grapes."
	--Topic=1                                   -> *
	--Topic=2,"excalibug"                       -> "My late mentor once told me he found a wallcarving about this sword in a cave beneath the castle.", Topic=2
	--Topic=2,"wallcarving"                     -> "That part of the dungeon was recently blocked by a cave-in. It was unsecure before, and only a fool would have entered there. I stayed out and alive."
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
