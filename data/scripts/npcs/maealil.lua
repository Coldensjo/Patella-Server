-- npcs/maealil.lua: the behaviour of the NPC Maealil, converted from
-- maealil.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of maealil.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- maealil.npc: Datenbank für die Mystikerin Maealil (Elfenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PALADIN = behaviour.PALADIN
local DRUID = behaviour.DRUID
local PVPENFORCED = behaviour.PVPENFORCED
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local LEVEL = behaviour.LEVEL
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local COUNTMONEY = behaviour.COUNTMONEY
local spellKnown = behaviour.spellKnown
local spellLevel = behaviour.spellLevel
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Ashari, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry, please wait a moment %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi, traveller."}},

	{{"bye"}, {"Asha Thrazi, traveller.", idle}},
	{{"asha", "thrazi"}, {previous}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a mystic."}},
	{{"name"}, {"I am known as Maealil."}},
	{{"time"}, {"I don't own one of those little machines."}},
	{{"mystic"}, {"I am a philosopher and healer."}},

	{{"elves"}, {"We are an ancient race, abandoned by the gods and doomed to find our way alone."}},
	{{"dwarfs"}, {"They cultivate earth but don't understand it."}},
	{{"humans"}, {"They are somewhat orcish in their nature."}},
	{{"troll"}, {"I don't think it's a good idea to keep servants."}},

	{{"cenath"}, {"My parents were Deraisim but joined the Cenath caste before my birth."}},
	{{"kuridai"}, {"I hope they don't do something foolish one day."}},
	{{"deraisim"}, {"Unfortunately they are to busy to care for the finer things in life."}},
	{{"abdaisim"}, {"They should join our town for their and our own safety."}},
	{{"teshial"}, {"I would love to learn more about the Teshial."}},
	{{"dream"}, {previous}},
	{{"ferumbras"}, {"Only another servant of evil."}},
	{{"crunor"}, {"The great tree is the beginning for all things living and Priyla helps us to understand that."}},
	{{"priyla"}, {"The daughter of the stars gives us knowledge and teaches us magic."}},

	{{"excalibug"}, {"Is that a new kind of bug the Deraisim found?"}},
	{{"news"}, {"I don't know anything of importance."}},

	{{"magic"}, {"I can heal you or even teach you some spells of healing."}},
	{{"druid"}, {"Druids are great healers."}},
	{{"sorcerer"}, {"They understand so few..."}},
	{{"spellbook"}, {"I have none here."}},
	{{"spell"}, {"I teach the spells 'Light Healing', 'Antidote', 'Antidote Rune', 'Intense Healing', 'Intense Healing Rune, 'Ultimate Healing', and 'Ultimate Healing Rune'."}},

	{{DRUID, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(3)}},
	{{DRUID, "intense", "healing"}, {setType(2), setPrice(350), "Do you want to buy the spell 'Intense Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "intense", "healing", "rune"}, {setType(4), setPrice(600), "Do you want to buy the spell 'Intense Healing Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "antidote", "rune"}, {setType(31), setPrice(600), "Do you want to buy the spell 'Antidote Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "ultimate", "healing"}, {setType(3), setPrice(1000), "Do you want to buy the spell 'Ultimate Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "ultimate", "healing", "rune"}, {setType(5), setPrice(1500), "Do you want to buy the spell 'Ultimate Healing Rune' for %P gold?", setTopic(3)}},

	{{PALADIN, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(3)}},
	{{PALADIN, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(3)}},
	{{PALADIN, "intense", "healing"}, {setType(2), setPrice(350), "Do you want to buy the spell 'Intense Healing' for %P gold?", setTopic(3)}},
	{{PALADIN, "ultimate", "healing"}, {setType(3), setPrice(1000), "Do you want to buy the spell 'Ultimate Healing' for %P gold?", setTopic(3)}},

	{{"light", "healing"}, {"I'm sorry, but this spell is only for druids and paladins."}},
	{{"antidote"}, {previous}},
	{{"intense", "healing"}, {previous}},
	{{"ultimate", "healing"}, {previous}},
	{{"intense", "healing", "rune"}, {"I'm sorry, but this spell is only for druids."}},
	{{"antidote", "rune"}, {previous}},
	{{"ultimate", "healing", "rune"}, {previous}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know that spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You need to advance to level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"You do not have enough gold to pay my services."}},
	{{eq(TOPIC, 3), "yes"}, {"From now on you can cast this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"As you wish."}},

	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are looking really bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},

	{{"blessing", PVPENFORCED, FORCE}, {"The lifeforce of this world is waning. There are no more blessings avaliable on this world."}},
	{{"pilgrimage", PVPENFORCED, FORCE}, {previous}},
	{{"ashes", PVPENFORCED, FORCE}, {previous}},

	{{"spiritual", PVPENFORCED, FORCE}, {previous}},
	{{"shielding", PVPENFORCED, FORCE}, {previous}},
	{{"sacred", "places", PVPENFORCED, FORCE}, {previous}},
	{{"spark", PVPENFORCED, FORCE}, {previous}},
	{{"phoenix", PVPENFORCED, FORCE}, {previous}},
	{{"embrace", PVPENFORCED, FORCE}, {previous}},
	{{"fire", PVPENFORCED, FORCE}, {previous}},
	{{"suns", PVPENFORCED, FORCE}, {previous}},
	{{"wisdom", PVPENFORCED, FORCE}, {previous}},
	{{"solitude", PVPENFORCED, FORCE}, {previous}},

	{{"pilgrimage"}, {"Whenever you receive a lethal wound your lifeforce is damaged. With every single of the five blessings you have this damage will be reduced."}},
	{{"ashes"}, {previous}},

	{{"blessing", PVPENFORCED}, {"The lifforce of this world is wannig. There are no more blessings avaliable on this world."}},

	{{"blessing"}, {"There are five different blessings available in five sacred places. These blessings are: the spiritual shielding, the spark of the phoenix, the embrace of tibia, the fire of the suns and the wisdom of solitude."}},
	{{"sacred", "places"}, {"Just ask in which of the five blessings you are interested in."}},

	{{"spiritual", gt(questValue(104), 0)}, {"I see you received the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding", gt(questValue(104), 0)}, {previous}},
	{{"spiritual"}, {"You can receive the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding"}, {previous}},

	{{"spark", gt(questValue(102), 0)}, {"I an sense that the spark of the phoenix already was given to you by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix", gt(questValue(102), 0)}, {previous}},
	{{"spark"}, {"The spark of the phoenix is given by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix"}, {previous}},

	{{"embrace", gt(questValue(105), 0)}, {"I can sense the druids north of Carlin provided you with the embrace of tibia."}},
	{{"embrace"}, {"The druids north of Carlin can provide you with the embrace of tibia."}},

	{{"fire", "suns", gt(questValue(103), 0)}, {"I can see you recieved the blessing of the two suns in the suntower near Ab'Dendriel ."}},
	{{"suns", gt(questValue(103), 0)}, {previous}},
	{{"fire", "suns"}, {"Ask for the blessing of the two suns in the suntower near Ab'Dendriel ."}},
	{{"suns"}, {previous}},

	{{"wisdom", gt(questValue(101), 0)}, {"I can sense you already talked to the hermit Eremo on the isle of Cormaya and recieved this blessing."}},
	{{"solitude", gt(questValue(101), 0)}, {previous}},
	{{"wisdom"}, {"Talk to the hermit Eremo on the isle of Cormaya about this blessing."}},
	{{"solitude"}, {previous}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
