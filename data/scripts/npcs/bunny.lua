-- npcs/bunny.lua: the behaviour of the NPC Bunny Bonecrusher, converted from
-- bunny.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of bunny.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- bunny.npc: Datenbank für die Generalin Bunny Bonecrusher

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
	{{ADDRESS, "hail", "general", FORCE}, {"Salutations, commoner %N!"}},
	{{ADDRESS, "salutations", "general", FORCE}, {"Salutations, commoner %N!"}},
	{{ADDRESS, "hello", FORCE}, {"Address me properly %N!", idle}},
	{{ADDRESS, "hi", FORCE}, {"Address me properly %N!", idle}},
	{{ADDRESS, "hail", FORCE}, {"Address me with my title, commoner %N!", idle}},
	{{ADDRESS, "salutations", FORCE}, {"Address me with my title, commoner %N!", idle}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"SILENCE! I am busy!"}},
	{{BUSY, "hi$", FORCE}, {"SILENCE! I am busy!"}},
	{{BUSY, "hail$", FORCE}, {"SILENCE! I am busy!"}},
	{{BUSY, "salutations$", FORCE}, {"SILENCE! I am busy!"}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {nop}},

	{{"bye"}, {"LONG LIVE THE QUEEN!", idle}},
	{{"farewell"}, {previous}},
	{{"news"}, {"Our reports are only for internal use."}},
	{{"report"}, {previous}},
	{{"queen"}, {"HAIL TO QUEEN ELOISE, OUR NOBLE LEADER!"}},
	{{"leader"}, {"Queen Eloise is a fine leader for our fair town, indeed!"}},
	{{"job", FEMALE}, {"I am the general of the queen's army! You really should consider to join, sister."}},
	{{"job", MALE}, {"I am the general of the queen's army and have not the time to explain this concept to you."}},
	{{"how", "are", "you"}, {"We are in constant training and in perfect health."}},
	{{"sell"}, {"Are you suggesting I am corruptible?", setTopic(2)}},
	{{"army"}, {"The army protects the defenceless males of our city. Our elite forces are the Green Ferrets."}},
	{{"guard"}, {previous}},
	{{"green", "ferrets"}, {"Our elite forces are trained by rangers and druids. In the woods they are only second to some elves."}},
	{{"castle"}, {"The castle is not meant for defence but as a residence for the royal family."}},
	{{"subject"}, {"Our citizens have the luck to live under the wise rule of our beloved queen!"}},
	{{"dogs", "of", "war"}, {"They are a men's club, mainly concerned about bragging and drinking alcohol."}},
	{{"knights", "of", "noodles"}, {"They are rumoured to be skilled fighters. Then again, in the land of the blind..."}},
	{{"druid"}, {"They are our main magic support and play a major role in our battletactics."}},
	{{"battletactics"}, {"Our tactic is to kiss."}},
	{{"tactics"}, {previous}},
	{{"kiss"}, {"K.I.S.S.! Keep It Simple, Stupid! Complicated tactics are to easy to be crushed by a twist of fate."}},
	{{"bloodblade"}, {"Old man. I can't tell what's worse for the shape of Thais' army."}},
	{{"thais"}, {"It's just a rotten hideout for drunks and men too lazy to do some serious work."}},
	{{"city"}, {"Our city blends in with the nature surrounding it. Our druids take care of that."}},
	{{"bonecrusher"}, {"Our family serves in the Carlin army since uncounted generations!"}},
	{{"sister"}, {previous}},
	{{"bambi"}, {"She is one of my beloved sisters and serves Carlin as a town guard."}},
	{{"blossom"}, {previous}},
	{{"busty"}, {previous}},
	{{"family"}, {previous}},
	{{"fenbala"}, {"She is one of our Green Ferrets and one of the queen's bodyguards."}},
	{{"barbara"}, {previous}},
	{{"cornelia"}, {"Cornelia forges the armor necessary for our troops."}},
	{{"armor"}, {previous}},
	{{"rowenna"}, {"Rowenna is responsible for our troops' supply with weapons."}},
	{{"weapon"}, {previous}},
	{{"legola"}, {"She is a distant cousin of mine and my sisters."}},
	{{"ferumbras"}, {"Believe it or not. I killed him two times with my own bow, but some unholy forces rise him again and again."}},
	{{"join"}, {"Join what?"}},
	{{"join", "army"}, {"Sorry, we dont recruit foreigners. Perhaps you can join by doing a quest for the queen."}},
	{{"quest", FEMALE}, {"Sometimes the queen calls for heroines. Keep eyes and ears open!"}},
	{{"mission", FEMALE}, {previous}},
	{{"quest", MALE}, {"Yeah. Entrusting a male with an important quest. Get serious!"}},
	{{"mission", MALE}, {previous}},
	{{"god"}, {"I whorship Banor, the first warrior!"}},
	{{"banor"}, {"He is the idol for all fighting women and a reminder of what a man could become, if he could jump over his own shadow!"}},
	{{"zathroth"}, {"Don't mention the dark one in the city of life!"}},
	{{"monster"}, {"We cleared the woods around Carlin from most of them. But lately more and more showed up again."}},
	{{"excalibug"}, {"I am sure only a woman could muster the courage and strength to wield this weapon of myth."}},
	{{"graveyard"}, {"Bah! Just men's tales! Who believes in such bullshit? Perhaps we should put some men there over night and see what happens. Hehehe!"}},
	{{"cemetary"}, {previous}},
	{{"crypt"}, {previous}},

	{{"fuck"}, {"Take this!", burning(10, 1), effectOpp(15), effectMe(5)}},
	{{"idiot"}, {"Take this!", burning(10, 1), effectOpp(15), effectMe(5)}},
	{{"asshole"}, {"Take this!", burning(10, 1), effectOpp(15), effectMe(5)}},
	{{"ass$"}, {"Take this!", burning(10, 1), effectOpp(15), effectMe(5)}},
	{{"fag$"}, {"Take this!", burning(10, 1), effectOpp(15), effectMe(5)}},
	{{"stupid"}, {"Take this!", burning(10, 1), effectOpp(15), effectMe(5)}},
	{{"tyrant"}, {"Take this!", burning(10, 1), effectOpp(15), effectMe(5)}},
	{{"shit"}, {"Take this!", burning(10, 1), effectOpp(15), effectMe(5)}},
	{{"lunatic"}, {"Take this!", burning(10, 1), effectOpp(15), effectMe(5)}},

	{{eq(TOPIC, 2), "yes"}, {"Take this!", burning(10, 1), effectOpp(15), effectMe(5)}},
	{{eq(TOPIC, 2)}, {"You should be careful with your words!"}},

	{{}, {"Your words don't make any sense to me."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
