-- npcs/elathriel.lua: the behaviour of the NPC Elathriel, converted from
-- elathriel.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of elathriel.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- erathriel.npc: Datenbank für den Kuridai-Anführer Elathriel (Elfenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PALADIN = behaviour.PALADIN
local DRUID = behaviour.DRUID
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
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
local setData = behaviour.setData
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Be greeted %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Shut up! Can't you see that I am talking?!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi, stranger.", idle}},
	{{"farewell"}, {previous}},
	{{"asha", "thrazi"}, {previous}},
	{{"name"}, {"Not that I like to talk to you, but I am Elathriel Shadowslayer."}},
	{{"job"}, {"I am the leader of the Kuridai and the Az'irel of Ab'dendriel. Humans would call it sheriff, executioner, or avenger."}},
	{{"sheriff"}, {"Sometimes people get imprisoned for some time. True criminals will be cast out and for comitting the worst crimes offenders are thrown into the hellgate."}},
	{{"executioner"}, {previous}},
	{{"avenger"}, {previous}},

	{{"hellgate"}, {"It was here among other structures, like the depot tower, before our people came here. It's secured by a sealed door."}},
	{{"door"}, {"For safety we keep the door to the hellgate locked all times. I have the keys to open it when needed."}},
	{{"sealed"}, {previous}},
	{{"key"}, {setType(2970), setData(3012), setAmount(1), setPrice(5000), "If you are that curious, do you want to buy a key for %P gold? Don't blame me if you get sucked in.", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Believe me, it's better for you that way."}},

	{{"time"}, {"I couldn't care less."}},

	{{"carlin"}, {"We watch this city and the actions of its inhabitants closely."}},
	{{"thais"}, {"The thain kingdom and we share some enemys, so its only logical to cooperate in a few areas."}},
	{{"venore"}, {"The merchants of venore provide us with some usefull goods. Still I an convinced that they get more out of our bargain then we do."}},
	{{"roderick"}, {"He is tolerated here as the spokesman of the thaian king."}},
	{{"olrik"}, {"This human is too unimportant to be even mentioned."}},

	{{"king"}, {"It's hard for some of my people to grasp the true concept of a strong leader."}},
	{{"tibianus"}, {"A human weakling, not much more."}},
	{{"eloise"}, {previous}},
	{{"elves"}, {"My people are divided in castes in these times, until they comprehend that only the way of the Kuridai can save us all."}},
	{{"dwarfs"}, {"We might use the shelter earth and hills provide us, but their obsession for metal is a waste of time."}},
	{{"humans"}, {"They are useful ... and better stay useful."}},
	{{"troll"}, {"Like all inferior races they can be at least used for something good. The other castes are just jealous about our use of them."}},
	{{"army"}, {"It's one of the more useful concepts we can learn from the other races."}},
	{{"cenath"}, {"Arrogant bastards, but they wield quite powerful magics."}},
	{{"kuridai"}, {"We are the heart of the elven society. We forge, we build, and we don't allow our people to be pushed around."}},
	{{"deraisim"}, {"Confused cowards. With all their skill they still tend to hide and run. What a waste."}},
	{{"abdaisim"}, {"Even more undecided then the deraisim."}},
	{{"teshial"}, {"Dreamers are of no practical use. I don't mourn their demise."}},
	{{"ferumbras"}, {"Even if he'd walk through the town above the other castes won't see the necessity to follow OUR way."}},
	{{"crunor"}, {"I have no use for the treething. I worship Mortiur, the ravager, of course."}},
	{{"mortiur"}, {"The celestial paladin of revenge. He was one of the greatest elven wariors of all times."}},
	{{"excalibug"}, {"I still doubt it exists."}},
	{{"news"}, {"News are confidential and not your business."}},

	{{"magic"}, {"I mastered some spells of battle."}},
	{{"druid"}, {"Druids' magic is too peaceful for my taste."}},
	{{"sorcerer"}, {"I have seen human sorcerers doing some impressive things ... before they died."}},
	{{"spellbook"}, {"I don't sell such stuff."}},
	{{"spell"}, {"I teach the spells 'Light Magic Missile', 'Heavy Magic Missile', 'Fireball', 'Great Fireball', 'Fire Bomb', and 'Explosion'."}},

	{{DRUID, "light", "missile"}, {setType(7), setPrice(500), "Do you want to buy the spell 'Light Magic Missile' for %P gold?", setTopic(3)}},
	{{DRUID, "heavy", "missile"}, {setType(8), setPrice(1500), "Do you want to buy the spell 'Heavy Magic Missile' for %P gold?", setTopic(3)}},
	{{DRUID, "fireball"}, {setType(15), setPrice(800), "Do you want to buy the spell 'Fireball' for %P gold?", setTopic(3)}},
	{{DRUID, "great", "fireball"}, {setType(16), setPrice(1200), "Do you want to buy the spell 'Great Fireball' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "bomb"}, {setType(17), setPrice(1500), "Do you want to buy the spell 'Fire Bomb' for %P gold?", setTopic(3)}},
	{{DRUID, "explosion"}, {setType(18), setPrice(1800), "Do you want to buy the spell 'Explosion' for %P gold?", setTopic(3)}},

	{{PALADIN, "light", "missile"}, {setType(7), setPrice(500), "Do you want to buy the spell 'Light Magic Missile' for %P gold?", setTopic(3)}},
	{{PALADIN, "heavy", "missile"}, {setType(8), setPrice(1500), "Do you want to buy the spell 'Heavy Magic Missile' for %P gold?", setTopic(3)}},
	{{PALADIN, "fireball"}, {setType(15), setPrice(800), "Do you want to buy the spell 'Fireball' for %P gold?", setTopic(3)}},

	{{"light", "missile"}, {"I'm sorry, but this spell is only for druids and paladins."}},
	{{"heavy", "missile"}, {previous}},
	{{"fireball"}, {previous}},
	{{"great", "fireball"}, {"I'm sorry, but this spell is only for druids."}},
	{{"fire", "bomb"}, {previous}},
	{{"explosion"}, {previous}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know that spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You need to advance to level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"You do not have enough gold to pay my services."}},
	{{eq(TOPIC, 3), "yes"}, {"From now on you can cast this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"I thought so."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
