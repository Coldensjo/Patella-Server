-- npcs/blood.lua: the behaviour of the NPC Harkath Bloodblade, converted from
-- blood.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of blood.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- blood.npc: Datenbank für General Harkath Bloodblade

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local TOPIC = behaviour.TOPIC
local setTopic = behaviour.setTopic
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hail$", "general", FORCE}, {"Salutations, commoner %N!"}},
	{{ADDRESS, "salutations$", "general", FORCE}, {"Salutations, commoner %N!!"}},
	{{ADDRESS, "hello$", FORCE}, {"Address me properly %N!", idle}},
	{{ADDRESS, "hi$", FORCE}, {"Address me properly %N!", idle}},
	{{ADDRESS, "hail$", FORCE}, {"Address me with my title, commoner %N!", idle}},
	{{ADDRESS, "salutations$", FORCE}, {"Address me with my title, commoner %N!", idle}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"SILENCE! I am busy!"}},
	{{BUSY, "hi$", FORCE}, {"SILENCE! I am busy!"}},
	{{BUSY, "hail$", FORCE}, {"SILENCE! I am busy!"}},
	{{BUSY, "salutations$", FORCE}, {"SILENCE! I am busy!"}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"LONG LIVE THE KING!"}},

	{{"bye"}, {"LONG LIVE THE KING!", idle}},
	{{"farewell"}, {previous}},
	{{"news"}, {"No news are good news."}},
	{{"king"}, {"HAIL TO KING TIBIANUS, OUR WISE LEADER!"}},
	{{"leader"}, {"King Tibianus III is our wise and just leader."}},
	{{"job", FEMALE}, {"My Lady, I am the general of the king's army."}},
	{{"job", MALE}, {"I am the general of the king's army."}},
	{{"how", "are", "you"}, {"I am in perfect condition, commoner."}},
	{{"sell"}, {"Are you suggesting I am corruptible?", setTopic(2)}},
	{{"army"}, {"The army protects our city. I divided it into three battlegroups."}},
	{{"guard"}, {previous}},
	{{"general"}, {"It is my duty to lead the armed forces of our beloved city into battle against our enemies."}},
	{{"enemies"}, {"Evil has many faces. The servants of evil cannot always be recognized as easily as Ferumbras, for instance."}},
	{{"enemy"}, {previous}},
	{{"battlegroup"}, {"The battlegroups are the 'dogs of war', the 'red guards', and the 'silver guards'."}},
	{{"castle"}, {"The castle is prepared to withstand any direct assault."}},
	{{"subject"}, {"We all live under the rule of our beloved king."}},
	{{"dogs", "of", "war"}, {"They are our main army."}},
	{{"red", "guard"}, {"They are our special forces. Some serve as city guards, others as secret police."}},
	{{"secret", "police"}, {"The branch of the red guard that serves as secret police is known as the TBI."}},
	{{"tbi$"}, {"The Tibian Bureau of Investigation. Kind of secret police. I don't bother much about such things, I prefer my fights eye to eye."}},
	{{"chester"}, {"I don't know much about him. He is a very secretive person."}},
	{{"silver", "guard"}, {"The best sorcerers, paladins, knights, and druids of our forces are chosen to serve as silver guards. They are the bodyguards of the king."}},
	{{"city"}, {"The rapid growth of the city makes it hard to patrol and vulnerable to attacks."}},
	{{"scum"}, {"We will eliminate all resistance against law and order!"}},
	{{"stutch"}, {"He is one of our best men and serves in the silver guard."}},
	{{"harsky"}, {previous}},
	{{"bozo"}, {"I hardly know him."}},
	{{"sam"}, {"Sam is responsible to supply our troops with weapons and armor."}},
	{{"weapon"}, {previous}},
	{{"armor"}, {previous}},
	{{"elane"}, {"AH! WHAT A WOMAN!"}},
	{{"gorn"}, {"He was an adventurer once. He was a fine fighter but lacked the discipline to serve in our army."}},
	{{"benjamin"}, {"He was the king's general before I was promoted. Poor guy, lost his mind in a battle against the evil Ferumbras."}},
	{{"ferumbras"}, {"He is allied with evil itself! Each time we kill him he returns to take revenge."}},
	{{"join"}, {"Join what?"}},
	{{"join", "army"}, {"Sorry, we don't recruit today. Perhaps you can join by doing a quest for the king."}},
	{{"quest"}, {"Sometimes the king calls for heroes. Keep eyes and ears open! I also heared Baxter has some work for young adventurers."}},
	{{"mission"}, {previous}},
	{{"god"}, {"I whorship Banor, the first warrior!"}},
	{{"banor"}, {"He is the idol of all knights and paladins."}},
	{{"zathroth"}, {"Don't mention the dark one!"}},
	{{"brog"}, {"The orcs, trolls, and cyclopses sacrificed more than one of my men to Brog, the raging one."}},
	{{"monster"}, {"They seldom dare to attack the city itself."}},
	{{"excalibug"}, {"In the legends it is told, that this weapon made its wielder able to fight the mightiest demons hand to hand."}},
	{{"rebellion"}, {"Ask Chester of the T.B.I. about that."}},

	{{"fuck"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"idiot"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"asshole"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"ass$"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"fag$"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"stupid"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"tyrant"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"shit"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"lunatic"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},

	{{eq(TOPIC, 2), "yes"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{eq(TOPIC, 2)}, {"You should be careful with your words!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
