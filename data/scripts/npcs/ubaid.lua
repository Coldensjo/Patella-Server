-- npcs/ubaid.lua: the behaviour of the NPC Ubaid, converted from
-- ubaid.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ubaid.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ubaid.npc: Datenbank für den Efreetpförtner Ubaid

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local gt = behaviour.gt
local TOPIC = behaviour.TOPIC
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setQuestValue = behaviour.setQuestValue
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", eq(questValue(278), 3), FORCE}, {"Still alive, %N?"}},
	{{ADDRESS, "hi$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, "greetings$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, "djanni'hah$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, "hello$", gt(questValue(278), 0), FORCE}, {"Shove off, little one! Humans are not welcome here, %N!", idle}},
	{{ADDRESS, "hi$", gt(questValue(278), 0), FORCE}, {previous}},
	{{ADDRESS, "greetings$", gt(questValue(278), 0), FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "greetings$", FORCE}, {previous}},
	{{ADDRESS, "djanni'hah$", gt(questValue(278), 0), FORCE}, {"What? You know the word, %N? All right then - I won't kill you. At least, not now."}},
	{{ADDRESS, "djanni'hah$", FORCE}, {"Hmmm? Is this human %N trying to say something? I don't think so.", idle}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", eq(questValue(278), 3), FORCE}, {"%N again. You have to wait.", queue}},
	{{BUSY, "hi$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, "greetings$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, "djanni'hah$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, "hello$", gt(questValue(278), 0), FORCE}, {"Shove off, little one! You are not welcome here, %N!"}},
	{{BUSY, "hi$", gt(questValue(278), 0), FORCE}, {previous}},
	{{BUSY, "greetings$", gt(questValue(278), 0), FORCE}, {previous}},
	{{BUSY, "hello$", FORCE}, {previous}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "greetings$", FORCE}, {previous}},
	{{BUSY, "djanni'hah$", gt(questValue(278), 0), FORCE}, {"Oh no! More of you humans! I'm busy, %N, so you better hold your tongue until I have time for you.", queue}},
	{{BUSY, "djanni'hah$", FORCE}, {"Hmmm? Is this human trying to say something? I don't think so."}},
	{{BUSY, FORCE}, {nop}},

	{{VANISH, FORCE}, {"Why did you waste my time?"}},

	{{"bye"}, {"Hail King Malor! See you on the battlefield, human worm.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Ubaid. Why do you want to know that, human? Hmm... suspicious."}},
	{{"ubaid"}, {"That is my name. I don't like it when a human pronounces it."}},
	{{"job"}, {"Well, what do you think? I keep watch around here to make sure people like you don't enter."}},

	{{"gate"}, {"Only the mighty Efreet, the true djinn of Tibia, may enter Mal'ouquah! ...", "All Marids and little worms like yourself should leave now or something bad may happen. Am I right?", setTopic(1)}},
	{{"pass"}, {previous}},
	{{"door"}, {previous}},
	{{"enter"}, {previous}},
	{{"join"}, {previous}},
	{{"follow"}, {previous}},

	{{"gate", eq(questValue(278), 3), FORCE}, {"You already pledged loyalty to king Malor!"}},
	{{"pass", eq(questValue(278), 3), FORCE}, {previous}},
	{{"door", eq(questValue(278), 3), FORCE}, {previous}},
	{{"enter", eq(questValue(278), 3), FORCE}, {previous}},
	{{"join", eq(questValue(278), 3), FORCE}, {previous}},
	{{"follow", eq(questValue(278), 3), FORCE}, {previous}},

	{{eq(TOPIC, 1), "no", eq(questValue(278), 2), FORCE}, {"Who do you think you are? A Marid? Shove off, moron.", idle}},
	{{eq(TOPIC, 1), "no", FORCE}, {"Of cour... Huh!? No!? I can't believe it! ...", "You... you got some nerves... Hmm. ...", "Maybe we have some use for someone like you. Would you be interested in working for us. Helping to fight the Marid?", setTopic(2)}},

	{{eq(TOPIC, 2), "yes", FORCE}, {"So you pledge loyalty to king Malor and you are willing to never ever set foot on Marids' territory, unless you want to kill them? Yes?", setTopic(3)}},
	{{eq(TOPIC, 3), "yes", FORCE}, {"Well then - welcome to Mal'ouquah. ...", "Go now to general Baa'leal and don't forget to greet him correctly! ...", "And don't touch anything!", setQuestValue(278, 3), idle}},
	{{eq(TOPIC, 1), FORCE}, {"Of course. Then don't waste my time and shove off.", idle}},
	{{eq(TOPIC, 2), FORCE}, {previous}},
	{{eq(TOPIC, 3), FORCE}, {previous}},

	{{"king"}, {"Well, Malor is not officially king of all djinn yet, but now our beloved leader is back that is a mere formality."}},
	{{"malor"}, {previous}},
	{{"djinn"}, {"We are a race of rulers and dominators! Or at least we, the Efreet, are!"}},
	{{"efreet"}, {"The Efreet are the true djinn! Those namby-pamby milksops who call themselves the Marid and still follow Gabel, no longer deserve the honour to call themselves djinn."}},
	{{"marid"}, {"Marid? When? Where? How many? RED ALERT! ...", "Hey! There is nobody here! Don't do that again, human!"}},
	{{"gabel"}, {"I used to serve under Gabel, but he is no longer my king. If that wacky wimp should ever come here to Mal'ouquah I will personally... you know... turn him away. Yes!"}},
	{{"mal'ouquah"}, {"This place is our home, and as long as I'm here no meddler will trespass!"}},
	{{"ashta'daramai"}, {"The Marids' hideout, isn't it? I have never been there, but I am sure one day I will. That will be the day Ashta'daramai falls into our hands!"}},
	{{"human"}, {"You are an inferior race of feeble, scheming jerks. No offence."}},
	{{"zathroth"}, {"Zathroth is our father! Of course, the son always has a right to hate his father, right?"}},
	{{"tibia"}, {"This world is ours by right, and we will take it!"}},
	{{"daraman"}, {"How dare you utter that name in my presence, human. Don't strain my patience, worm! You may know the secret word, but... who knows... it is always possible that your head is torn off in some terrible accident."}},
	{{"darashia"}, {"A human settlement to the west? I have not been there yet, but when I do I'm sure I will be remembered."}},
	{{"scarab"}, {"They make good pets if you know how to keep them. Did you know they just adore human flesh?"}},
	{{"edron"}, {"Isn't that the name of some petty human settlement?"}},
	{{"thais"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {previous}},
	{{"carlin"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"ankrahmun"}, {"I know that damn city well. A long time ago we laid siege to it. ...", "We would have taken it, but those traitorous humans allowed a Marid garrison to entrench itself there, and we never managed to throw them out. Cowards and traitors the lot of them."}},
	{{"pharaoh"}, {"They say Ankrahmun is now ruled by a crazy pharaoh who wants to tell his whole people into drooling undead. That's humans. Sickos and weirdos the lot of them."}},
	{{"palace"}, {"One day we will sack that place and burn it to the ground."}},
	{{"temple"}, {previous}},
	{{"ascension"}, {"I think I've heard that term before. Has to do with that weirdo pharaoh, right?"}},
	{{"rah"}, {"Are you drunk?"}},
	{{"uthun"}, {previous}},
	{{"akh"}, {previous}},

	{{"kha'zeel"}, {"This mountain range is our home. Too bad we have to share it with the Marid. That will change, though. And pretty soon, believe me."}},
	{{"kha'labal"}, {"I like the desert. Just ruins and sand. And no human scum to be seen. The Kha'labal is a foretaste of what the djinn will do to the whole of Tibia!"}},
	{{"war"}, {"I don't know why I am stuck here! I should be at the front, killing Marid and humans. Well, perhaps I will kill you..."}},
	{{"baa'leal"}, {"General Baa'leal is our commander-in-chief of all his minions. He is as tough as an ancient scarab's buttocks and as sly a sand weasel."}},
	{{"alesar"}, {"I am not used to the sight of blueskins here in Mal'ouquah, and it does not make me too happy to see one. I am keeping an eye on this guy, and if I should ever find that he is playing games with us I will personally break his neck!"}},
	{{"fa'hradin"}, {"The old wizard is dangerous, but he will get what he deserves sooner or later."}},
	{{"lamp"}, {"I am not taking a nap! I am on duty!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
