-- npcs/ormuhn.lua: the behaviour of the NPC Ormuhn, converted from
-- ormuhn.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ormuhn.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ormuhn.npc: Datenbank für die arenaleiter ormuhn

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local KNIGHT = behaviour.KNIGHT
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
local spellKnown = behaviour.spellKnown
local spellLevel = behaviour.spellLevel
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "hi$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "be", "mourned", "pilgrim", "flesh", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Mourned %N, know that I am busy.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "be", "mourned", "pilgrim", "flesh", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May enlightenment be your path."}},

	{{"bye"}, {"May enlightenment be your path.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the arena master. I supervise all challenges that take place in this arena and train true fighters."}},
	{{"name"}, {"I am called Ormuhn."}},
	{{"time"}, {"Time only matters to you while you are mortal. Another instrument in the hands of the false gods to fool us all."}},
	{{"temple"}, {"The temple takes care of your Uthun. In this arena we challenge your Akh."}},
	{{"pharaoh"}, {"The pharaoh, our mighty leader, is an unliving god."}},
	{{"oldpharaoh"}, {"The pharaoh will know why he granted him a chance to ascend."}},
	{{"scarab"}, {"Scarabs might be sacred, but they are also a challenge. If you are able to overcome one of them, its spirit will forgive you. The everlasting sand will grant him rebirth anyway."}},
	{{"chosen"}, {"I am one of the chosen. To become like me you have to serve the pharaoh and his temple faithfully."}},

	{{"tibia"}, {"Tibia is a place kept in the thrall of the greedy false gods. One day though our pharaoh will free Tibia and guide all of its people to ascension."}},

	{{"carlin"}, {"These cities are nests of corruption and lies. For those who know the treason of the false gods is almost tangible there."}},
	{{"thais"}, {previous}},
	{{"edron"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {"Dwarves are worthy warriors. Still their mortal Akh makes them prey to true death and so their lives are wasted."}},
	{{"dwarves"}, {previous}},
	{{"dwarfes"}, {previous}},
	{{"ab'dendriel"}, {"Elves are nothing but feeble treehuggers."}},
	{{"elves"}, {previous}},
	{{"elfes"}, {previous}},
	{{"darama"}, {"This continent will be the first to prosper under the guidance of our pharaoh."}},
	{{"darashia"}, {"The people there are not totally lost to the false gods yet. Who knows? They may be saved yet."}},
	{{"daraman"}, {"A mere mortal prophet. As was to be expected, his mortality blurred his visions of ascension."}},
	{{"ankrahmun"}, {"This is the shelter of the mortal flock who listens to the teachings of our pharaoh."}},

	{{"mortality"}, {"Your curse of mortality can be lifted if you only prove youself worthy in the eyes of our pharaoh."}},
	{{"false", "gods"}, {"These greedy reapers of souls are the true scourge of poor mortals like you. Your living Akh makes you vulnerable for their attacks. Withstand them and you will get a chance to be raised to the exalted state of undeath."}},

	{{"ascension"}, {"Ascension is achieved in many steps. The first and most important step is unquestioning loyal service to our pharaoh."}},
	{{"Akh'rah", "Uthun"}, {"You should discuss such topics with our priests. I don't care too much for these matters."}},
	{{"Akh"}, {"As far as I know this is what you would call your body. Ask a priest for further information."}},
	{{"undead"}, {"We are the chosen ones."}},
	{{"undeath"}, {previous}},
	{{"Rah"}, {"That is your so-called soul. Ask a priest for further information."}},
	{{"uthun"}, {"All the things you remember form your uthun. Ask a priest for further information."}},
	{{"mourn"}, {"Living flesh is so ... pathetic."}},

	{{"arena"}, {"If you wish to test your mortal Akh you are at the right place."}},
	{{"palace"}, {"The palace is guarded by the elite forces of the chosen."}},
	{{KNIGHT, "spell"}, {"I can teach you healing spells and support spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to knights."}},

	{{KNIGHT, "instant", "spell"}, {"I can teach you healing spells and support spells. What kind of spell do you wish to learn?"}},
	{{KNIGHT, "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{KNIGHT, "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},

	{{eq(TOPIC, 2), "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{eq(TOPIC, 2), "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},
	{{eq(TOPIC, 2), "bye"}, {"May enlightenment be your path.", idle}},

	{{eq(TOPIC, 2), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"For level 13 I have 'Great Light'.", setTopic(2)}},
	{{eq(TOPIC, 2)}, {"Sorry, I have only spells for level 8, 9, 10 and 13.", setTopic(2)}},

	{{KNIGHT, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(3)}},
	{{KNIGHT, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(3)}},
	{{KNIGHT, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(3)}},
	{{KNIGHT, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(3)}},
	{{KNIGHT, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know how to cast this spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You have to be level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"Return when you have enough gold."}},
	{{eq(TOPIC, 3), "yes"}, {"Here you are. Look in your spellbook for the pronunciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
