-- npcs/baxter.lua: the behaviour of the NPC Baxter, converted from
-- baxter.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of baxter.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- baxter.npc: Datenbank für den Burgwächter Baxter

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
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"LONG LIVE KING TIBIANUS!"}},
	{{ADDRESS, "hi$", FORCE}, {"LONG LIVE KING TIBIANUS!"}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$"}, {"Cant you see I am busy, eh?!."}},
	{{BUSY, "hi$"}, {"Cant you see I am busy, eh?!."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"What a lack of manners!"}},

	{{"bye"}, {"LONG LIVE THE KING!", idle}},
	{{"farewell"}, {previous}},
	{{"news"}, {"It is rumoured that Ferumbras is planning a new attack on town."}},
	{{"how", "are", "you"}, {"I am healthy and vigilant."}},
	{{"sell"}, {"Visit Tibia's shopkeepers to buy their fine wares."}},
	{{"king"}, {"King Tibianus III is our wise and just leader!"}},
	{{"leader"}, {previous}},
	{{"job"}, {"I am a proud member of the king's army. It is my duty to guard the castle."}},
	{{"army"}, {"Our brave army, which protects our city, consists of three battlegroups."}},
	{{"guard"}, {previous}},
	{{"battlegroup"}, {"There are the dogs of war, the red guards, and the silver guards."}},
	{{"castle"}, {"His Royal Highness ordered the castle to be open for all his subjects."}},
	{{"subject"}, {"We all live under the benevolent guidance of our king."}},
	{{"dogs", "of", "war"}, {"They are our main army."}},
	{{"red", "guard"}, {"They are our special forces. Some serve as cityguards, others as secret police."}},
	{{"secret", "police"}, {"Ask a higher offical about that."}},
	{{"silver", "guard"}, {"The best sorcerers, paladins, knights, or druids of our forces are choosen to serve as silver guards. They are the bodyguards of the king."}},
	{{"city"}, {"Now that the king returned, we will clean the city from all scum."}},
	{{"scum"}, {"To much scum roams our streets in our days, the red guards will take care of them."}},
	{{"stutch"}, {"He is soldier in the silver guard."}},
	{{"harsky"}, {"He is soldier in the silver guard."}},
	{{"bozo"}, {"The royal jester. I dont think he is funny."}},
	{{"sam"}, {"He is a fine blacksmith. Almost all our weapons are made by him."}},
	{{"gorn"}, {"An old friend of mine. He was once a great warrior and adventurer, now he is running a shop."}},
	{{"benjamin"}, {"He was one of the king's best generals, now he is a bit ...uhm... forgetful."}},
	{{"excalibug"}, {"Gorn and I searched for this weapon in the darkest corners of each dungeon, but found nothing."}},
	{{"partos"}, {"He was wanted for a long time and got caught stealing some time ago.", setTopic(1)}},
	{{eq(TOPIC, 1), "fruit"}, {"I understand he was stealing some fruit, he is obsessed with, and got incautious."}},
	{{eq(TOPIC, 1)}, {"What has this to do with this Partos guy?"}},
	{{"chester"}, {"This man is paranoid, but I guess that is useful in his job."}},
	{{"tbi"}, {"There is almost nothing known about that organization."}},
	{{"work"}, {"We have a rat problem in the sewers. In the name of our glorious king I am paying 1 blinking piece of gold for every freshly killed rat you bring to me."}},
	{{"mission"}, {previous}},
	{{"quest"}, {previous}},
	{{"rat"}, {setType(3994), setAmount(1), setPrice(1), "Do you bring a freshly killed rat for a bounty of %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "rat"}, {setType(3994), setAmount(P1), setPrice(mul(1, P1)), "Do you want to deliver me %A rats for a bounty of %P gold?", setTopic(2)}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Here is your reward. You will become a great warrior some day.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Look like it wasn't as dead as you thought ... it's gone."}},
	{{eq(TOPIC, 2)}, {"Come on. Don't waste my time with your jests."}},

	{{"fuck"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"idiot"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"asshole"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"ass$"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"fag$"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"stupid"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"tyrant"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"shit"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"lunatic"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
