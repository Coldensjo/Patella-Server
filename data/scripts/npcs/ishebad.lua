-- npcs/ishebad.lua: the behaviour of the NPC Ishebad, converted from
-- ishebad.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ishebad.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ishebad.npc: Datenbank für den großwesir ankrahmuns

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PREMIUM = behaviour.PREMIUM
local PROMOTED = behaviour.PROMOTED
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local profession = behaviour.profession
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello", FORCE}, {"Be mourned, pilgrim in flesh."}},
	{{ADDRESS, "hi", FORCE}, {previous}},
	{{ADDRESS, "salutations", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"This person will never achieve ascension that way!"}},

	{{"bye"}, {"Good bye, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am Ishebad the chosen."}},
	{{"time"}, {"Time does not matter to the undead."}},

	{{"temple"}, {"The temple will take care of your spiritual matters."}},
	{{"pharaoh"}, {"Our immortal ruler, may he be blessed, is the keeper of our enlightenment and our saviour."}},
	{{"ashmunrah"}, {"The fallen pharaoh did not see it was time to step back and let his son rule. So he met the fate that he deserved."}},
	{{"scarab"}, {"The scarabs are keepers of secrets. Some secrets are not ment for your mortals. Ever keep that in mind."}},

	{{"tibia"}, {"This world just awaits the wisdom of our pharaoh. It needs that wisdom and will soon learn to appreciate it."}},
	{{"carlin"}, {"Other cities are of no importance. Ankrahmun will become the center of the known world anyways."}},
	{{"thais"}, {previous}},
	{{"edron"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"darama"}, {"The rule of our beloved pharaoh will soon spread this continent and one day the whole known world."}},
	{{"darashia"}, {"This village is so insignificant that our wise pharaoh has choosen to ignore it."}},
	{{"daraman"}, {"Some lunatic who was driven mad by the heat of the desert and dehydration."}},
	{{"Ankrahmun"}, {"Our city will become the capital of a worldwide empire."}},
	{{"Arkhothep"}, {"The pharaoh wants not to be disturbed. I am his grand vizier and responsible for the daily affairs of the city and promotions of heroes."}},
	{{"pharaoh"}, {previous}},
	{{"job"}, {previous}},
	{{"mortality"}, {"If you please our pharaoh, he will reward you and free you from your mortality."}},
	{{"ascension"}, {"Consult a priest to learn how you could achieve ascension."}},
	{{"Akh'rah", "Uthun"}, {previous}},
	{{"Akh"}, {previous}},
	{{"Rah"}, {previous}},
	{{"uthun"}, {previous}},
	{{"undead"}, {"Undeath is only for the choosen."}},
	{{"undeath"}, {previous}},
	{{"mourn"}, {"You mortals are all to be mourned for your miserable existance."}},

	{{"promot"}, {setPrice(20000), "Do you want to be promoted in your vocation for %P gold?", setTopic(4)}},
	{{eq(TOPIC, 4), "yes", PROMOTED, FORCE}, {"You are already promoted."}},
	{{eq(TOPIC, 4), "yes", lt(LEVEL, 20), FORCE}, {"You need to be at least level 20 in order to be promoted."}},
	{{eq(TOPIC, 4), "yes", lt(COUNTMONEY, PRICE), FORCE}, {"You do not have enough money."}},
	{{eq(TOPIC, 4), "yes", PREMIUM}, {"Congratulations! You are now promoted. Visit the sage Eremo for new spells.", profession(10), deleteMoney}},
	{{eq(TOPIC, 4), "yes"}, {"You need a premium account in order to promote."}},
	{{eq(TOPIC, 4)}, {"Ok, then not."}},
	{{"eremo"}, {"It is said that he lives on a small island near Edron. Maybe the people there know more about him."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
