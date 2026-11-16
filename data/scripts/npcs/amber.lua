-- npcs/amber.lua: the behaviour of the NPC Amber, converted from
-- amber.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of amber.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- amber.npc: Datenbank für die Abenteurerin Amber

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local count = behaviour.count
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local delete = behaviour.delete
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Oh hello, nice to see you %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry, I'm already talking to someone."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"See you later."}},

	{{"bye"}, {"See you later.", idle}},
	{{"farewell"}, {previous}},
	{{"how", "are", "you"}, {"I am recovering from a sea journey."}},
	{{"job"}, {"I explore and seek adventure."}},
	{{"explore"}, {"I have been almost everywhere in Tibia."}},
	{{"adventure"}, {"I fought fierce monsters, climbed the highest mountains, and crossed the sea on a raft."}},
	{{"sea"}, {"My trip over the sea was horrible. The weather was bad, the waves high and my raft quite simple."}},
	{{"time"}, {"Sorry, I lost my watch in a storm."}},
	{{"help"}, {"I can't help you much beyond information."}},
	{{"information"}, {"Just ask and I'll try to answer."}},
	{{"dungeon"}, {"I have not had the time to explore the dungeons of this isle, but I have seen two big caves in the east, and there is a ruined tower in the northwest."}},
	{{"sewer"}, {"I like sewers. I made my very first battle experience in the Thais sewers. The small sewersystem of Rookgaard has some nasty rats to fight."}},
	{{"assistant"}, {"I have a job of great responsibility. Mostly I keep annoying persons away from my boss."}},
	{{"monster"}, {"Oh, I fought orcs, cyclopses, minotaurs, and even green dragons."}},
	{{"cyclops"}, {"Horrible monsters they are."}},
	{{"minotaur"}, {previous}},
	{{"dragon"}, {previous}},
	{{"raft"}, {"I left my raft at the south eastern shore. I forgot my private notebook on it. If you could return it to me I would be very grateful."}},
	{{"quest"}, {previous}},
	{{"mission"}, {previous}},
	{{"seymour"}, {"I think this poor guy was a bad choice as the head of the academy."}},
	{{"academy"}, {"A fine institution, but it needs definitely more funds from the king."}},
	{{"king"}, {"King Tibianus is the ruler of Thais."}},
	{{"thais"}, {"A fine city, but the king has some problems enforcing the law."}},
	{{"cipfried"}, {"A gentle person. You should visit him, if you have problems."}},
	{{"dallheim"}, {"An extraordinary warrior. He's the first and last line of defense of Rookgaard."}},
	{{"hyacinth"}, {"Hyacinth is a great healer. He lives somewhere hidden on this isle."}},
	{{"willie"}, {"He's funny in his own, gruffy way."}},
	{{"obi"}, {"He's a funny little man."}},
	{{"weapon"}, {"The best weapons on this isle are just toothpicks, compared with the weapons warriors of the mainland wield."}},
	{{"magic"}, {"You can learn spells only in the guildhalls of the mainland."}},
	{{"tibia"}, {"I try to explore each spot of Tibia, and one day I will succeed."}},
	{{"castle"}, {"If you travel to Thais, you really should visit the marvelous castle."}},

	{{"book"}, {setType(2821), setAmount(1), "Do you bring me my notebook?", setTopic(1)}},
	{{"notebook"}, {previous}},
	{{eq(TOPIC, 1), "yes", ge(count(TYPE), AMOUNT)}, {"Excellent. Here, take this short sword, that might serve you well.", delete(TYPE), create(3294)}},
	{{eq(TOPIC, 1), "yes"}, {"Hm, you don't have it."}},
	{{eq(TOPIC, 1)}, {"Too bad."}},

	{{"orcish"}, {"I speak some orcish words, not much though, just 'yes' and 'no' and such basic.", setTopic(2)}},
	{{"language"}, {previous}},
	{{"prisoner"}, {previous}},
	{{"orc"}, {"Not the nicest guys you can encounter. I had some clashes with them and was prisoner of the orcs for some months."}},
	{{eq(TOPIC, 2), "yes"}, {"It's 'mok' in orcish. I help you more about that if you have some food."}},
	{{eq(TOPIC, 2), "no"}, {"In orcish that's 'burp'. I help you more about that if you have some food."}},

	{{"food"}, {"My favorite dish is salmon. Oh please, bring me some of it."}},
	{{"salmon"}, {setType(3579), setAmount(1), "Yeah! If you give me some salmon I will tell you more about the orcish language.", setTopic(3)}},
	{{eq(TOPIC, 3), "yes", ge(count(TYPE), AMOUNT)}, {"Thank you. Orcs call arrows 'pixo'.", delete(TYPE)}},
	{{eq(TOPIC, 3), "yes"}, {"You don't have one!"}},
	{{eq(TOPIC, 3)}, {"Ok, then I don't tell you more about the orcish language."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
