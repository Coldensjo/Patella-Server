-- npcs/tothdral.lua: the behaviour of the NPC Tothdral, converted from
-- tothdral.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of tothdral.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- tothdral.npc: Datenbank für den pyramidenmagier tothdral

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local SORCERER = behaviour.SORCERER
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
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Be mourned, pilgrim in flesh."}},
	{{ADDRESS, "hi$", FORCE}, {"Be mourned, pilgrim in flesh."}},
	{{ADDRESS, "be", "mourned", "pilgrim", "flesh", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Mourned %N, know that I am busy.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "be", "mourned", "pilgrim", "flesh", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Your incivility will not be tolerated much longer."}},

	{{"bye"}, {"Strive for enlightenment, mourned mortal.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the foremost astrologer and supreme magus of this city."}},
	{{"name"}, {"My name is Tothdral. They call me 'The Seeker Beyond the Grave'."}},
	{{"time"}, {"Time is your problem. It is no longer mine."}},
	{{"temple"}, {"Our temple is a centre of spirituality and learning. The temple and the serpentine tower work hand in hand for the good of all people, whether they are alive or undead."}},
	{{"pharaoh"}, {"The immortal pharaoh is our god and our example. He alone holds the secrets that will save us all from the greedy grasp of the false gods."}},
	{{"oldpharaoh"}, {"He has been given a chance to ascend. I am sure he will feel nothing but thankfulness for this divine son, our revered pharaoh."}},
	{{"scarab"}, {"If you know how to listen to them they will reveal ancient secrets to you."}},
	{{"tibia"}, {"This world is only a shadow of the worlds that have been. That was long ago, before the true gods fought each other in the godwars and the false gods rose to claim their heritage."}},
	{{"carlin"}, {"Those cities that bow to the false gods will fall prey to their treacherous greed sooner or later."}},
	{{"thais"}, {previous}},
	{{"edron"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {"The dwarves should have learned their lessons, but these boneheaded fools still don't see there is only one way to escape the false gods' grasp."}},
	{{"dwarves"}, {previous}},
	{{"dwarfes"}, {previous}},
	{{"ab'dendriel"}, {"Elves are nothing but idle riff-raff. They embrace the vain amusements of physical pleasure. Eternal damnation will be their lot."}},
	{{"elves"}, {previous}},
	{{"elfes"}, {previous}},
	{{"darama"}, {"This continent is mostly free from the servants of the false gods. Those who live here may hope to become worthy one day of the first steps towards ascension."}},
	{{"darashia"}, {"Their foolishness is great, but perhaps they still can be saved. If only they listened and accepted the next step to ascension."}},
	{{"next", "step"}, {"Undeath alone will rid you of the temptations that blur your vision of the true path."}},
	{{"daraman"}, {"He was so close. ... and still he failed to draw the right conclusions."}},
	{{"ankrahmun"}, {"This city is as old as the sands that surround it, and it is built on previous settlements that date back even further in time. Perhaps only the wise scarabs know the full story of this place."}},
	{{"pharaoh"}, {"The pharaoh was the first to take the ultimate step. He braved death and claimed the godhood that was rightfully his."}},
	{{"mortality"}, {"Mortality is your curse. When you are worthy the burden of mortality will be taken from your shoulders."}},
	{{"false", "gods"}, {"The self-styled gods were nothing but minor servants of the true gods. They steal the soul of any mortal foolish enough to believe in them. They plan to use the stolen souls to ascend to true godhood."}},
	{{"godswar"}, {"This war brought about the end of the true universe. That which is left now is but a shadow of former glories, a bleak remainder of what once was. The true gods perished and their essence was dispersed throughout the remaining universe."}},
	{{"great", "suffering"}, {"The great suffering is a phase of steady decline that will end eventually in void and emptiness unless some divine power such as our pharaoh will reverse it. Mend your ways and follow him! Perhaps you will be chosen to join him in his noble struggle."}},
	{{"ascension"}, {"The essence of the true gods is omnipresent in the universe. We all share this divine heritage, for every single one of us carries the divine spark inside him. This is the reason we all have a chance to ascend to godhood, too."}},
	{{"Akh'rah", "Uthun"}, {"The Akh'rah Uthun is the trinity of existence, the three that are one. The Akh, the shell, the Rah, the source of power, and the Uthun, our consciousness, form this union."}},
	{{"steal", "souls"}, {"The false gods harvest the souls of the dead to secure their stolen powers and status."}},
	{{"Akh"}, {"The Akh is a tool. As long as it is alive it is a burden and source of weakness, but if you ascend to undeath it becomes a useful tool that can be used to work towards greater ends."}},
	{{"undead"}, {"Undeath is an improvement. It is the gateway to goals that are nobler than eating, drinking or other fulfilments of trivial physical needs.."}},
	{{"undeath"}, {previous}},
	{{"Rah"}, {"The Rah is what the ignorant might call the soul. But it's more than that. It is the divine spark in all of us, the source of energy that keeps us alive."}},
	{{"uthun"}, {"The Uthun is the part of the trinity that is easiest to form. It consists of our recollections of the past and of our thoughts. It is that which determines who we are in this world and it gives us guidance throughout our existence."}},
	{{"mourn"}, {"The dead mourn the living because they are weak and excluded from ascension."}},
	{{"arena"}, {"The arena is a suitable distraction for the Uthun of the mortals. It might even serve as a place for them to prove their worth. If they pass the test they may be freed of their mortal shells."}},
	{{"palace"}, {"The residence of the pharaoh should be worshipped just as the pharaoh is worshipped. Don't enter until you have business there."}},

	{{SORCERER, "wand", lt(questValue(333), 1)}, {"Oh, you did not purchase your first magical wand yet? Please take this little present from me as your magic teacher!", setQuestValue(333, 1), setType(3074), setAmount(1), create(TYPE)}},

	{{"spell", SORCERER}, {"I can teach you rune spells and instant spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to Sorcerers."}},

	{{eq(TOPIC, 2), "rune", "spell"}, {"I sell attack rune spells and support rune spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "instant", "spell"}, {"I sell attack spells, healing spells, support spells and summon spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{eq(TOPIC, 2), "bye"}, {"Strive for enlightenment, mourned mortal.", idle}},

	{{SORCERER, "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{SORCERER, "rune", "spell"}, {"I sell attack rune spells and support rune spells. Which of these interests you most?"}},
	{{SORCERER, "instant", "spell"}, {"I sell attack spells, healing spells, support spells and summon spells. Which of these interests you most?"}},

	{{SORCERER, "attack", "rune", "spell"}, {"I sell missile rune spells, explosive rune spells, field rune spells, wall rune spells and bomb rune spells."}},
	{{SORCERER, "support", "rune", "spell"}, {"In this category I have 'Destroy Field'."}},

	{{SORCERER, "missile", "rune", "spell"}, {"In this category I have 'Light Magic Missile', 'Heavy Magic Missile' and 'Sudden Death'."}},
	{{SORCERER, "explosive", "rune", "spell"}, {"In this category I have 'Fireball', 'Great Fireball' and 'Explosion'."}},
	{{SORCERER, "field", "rune", "spell"}, {"In this category I have 'Poison Field', 'Fire Field' and 'Energy Field'."}},
	{{SORCERER, "wall", "rune", "spell"}, {"In this category I have 'Fire Wall', 'Poison Wall' and 'Energy Wall'."}},
	{{SORCERER, "bomb", "rune", "spell"}, {"In this category I have 'Firebomb'."}},

	{{SORCERER, "attack", "spell"}, {"In this category I have 'Fire Wave', 'Energy Wave', 'Energy Beam' and 'Great Energy Beam'."}},
	{{SORCERER, "healing", "spell"}, {"In this category I have 'Light Healing', 'Intense Healing', 'Ultimate Healing' and 'Antidote'."}},
	{{SORCERER, "support", "spell"}, {"In this category I have 'Find Person', 'Light', 'Great Light', 'Magic Shield', 'Creature Illusion' and 'Invisible'."}},
	{{SORCERER, "summon", "spell"}, {"In this category I have 'Summon Creature'."}},

	{{SORCERER, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(3)}},
	{{SORCERER, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(3)}},
	{{SORCERER, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(3)}},
	{{SORCERER, "light", "missile"}, {setType(7), setPrice(500), "Do you want to buy the spell 'Light Magic Missile' for %P gold?", setTopic(3)}},
	{{SORCERER, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(3)}},
	{{SORCERER, "intense", "healing"}, {setType(2), setPrice(350), "Do you want to buy the spell 'Intense Healing' for %P gold?", setTopic(3)}},
	{{SORCERER, "poison", "field"}, {setType(26), setPrice(300), "Do you want to buy the spell 'Poison Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "field"}, {setType(25), setPrice(500), "Do you want to buy the spell 'Fire Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "heavy", "missile"}, {setType(8), setPrice(1500), "Do you want to buy the spell 'Heavy Magic Missile' for %P gold?", setTopic(3)}},
	{{SORCERER, "magic", "shield"}, {setType(44), setPrice(450), "Do you want to buy the spell 'Magic Shield' for %P gold?", setTopic(3)}},
	{{SORCERER, "fireball"}, {setType(15), setPrice(800), "Do you want to buy the spell 'Fireball' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "field"}, {setType(27), setPrice(700), "Do you want to buy the spell 'Energy Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "destroy", "field"}, {setType(30), setPrice(700), "Do you want to buy the spell 'Destroy Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "wave"}, {setType(19), setPrice(850), "Do you want to buy the spell 'Fire Wave' for %P gold?", setTopic(3)}},
	{{SORCERER, "ultimate", "healing"}, {setType(3), setPrice(1000), "Do you want to buy the spell 'Ultimate Healing' for %P gold?", setTopic(3)}},
	{{SORCERER, "great", "fireball"}, {setType(16), setPrice(1200), "Do you want to buy the spell 'Great Fireball' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "bomb"}, {setType(17), setPrice(1500), "Do you want to buy the spell 'Fire Bomb' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "beam"}, {setType(22), setPrice(1000), "Do you want to buy the spell 'Energy Beam' for %P gold?", setTopic(3)}},
	{{SORCERER, "creature", "illusion"}, {setType(38), setPrice(1000), "Do you want to buy the spell 'Creature Illusion' for %P gold?", setTopic(3)}},
	{{SORCERER, "poison", "wall"}, {setType(32), setPrice(1600), "Do you want to buy the spell 'Poison Wall' for %P gold?", setTopic(3)}},
	{{SORCERER, "explosion"}, {setType(18), setPrice(1800), "Do you want to buy the spell 'Explosion' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "wall"}, {setType(28), setPrice(2000), "Do you want to buy the spell 'Fire Wall' for %P gold?", setTopic(3)}},
	{{SORCERER, "great", "energy", "beam"}, {setType(23), setPrice(1800), "Do you want to buy the spell 'Great Energy Beam' for %P gold?", setTopic(3)}},
	{{SORCERER, "invisible"}, {setType(45), setPrice(2000), "Do you want to buy the spell 'Invisible' for %P gold?", setTopic(3)}},
	{{SORCERER, "summon", "creature"}, {setType(9), setPrice(2000), "Do you want to buy the spell 'Summon Creature' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "wall"}, {setType(33), setPrice(2500), "Do you want to buy the spell 'Energy Wall' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "wave"}, {setType(13), setPrice(2500), "Do you want to buy the spell 'Energy Wave' for %P gold?", setTopic(3)}},
	{{SORCERER, "sudden", "death"}, {setType(21), setPrice(3000), "Do you want to buy the spell 'Sudden Death' for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 2), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "11$"}, {"For level 11 I have 'Intense Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"For level 13 I have 'Great Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "14$"}, {"For level 14 I have 'Poison Field' and 'Magic Shield'.", setTopic(2)}},
	{{eq(TOPIC, 2), "15$"}, {"For level 15 I have 'Fire Field' and 'Light Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "17$"}, {"For level 17 I have 'Fireball' and 'Destroy Field'.", setTopic(2)}},
	{{eq(TOPIC, 2), "18$"}, {"For level 18 I have 'Energy Field' and 'Fire Wave'.", setTopic(2)}},
	{{eq(TOPIC, 2), "20$"}, {"For level 20 I have 'Ultimate Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "23$"}, {"For level 23 I have 'Great Fireball', 'Energy Beam' and 'Creature Illusion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "25$"}, {"For level 25 I have 'Summon Creature' and 'Heavy Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "27$"}, {"For level 27 I have 'Firebomb'.", setTopic(2)}},
	{{eq(TOPIC, 2), "29$"}, {"For level 29 I have 'Poison Wall' and 'Great Energy Beam'.", setTopic(2)}},
	{{eq(TOPIC, 2), "31$"}, {"For level 31 I have 'Explosion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "33$"}, {"For level 33 I have 'Fire Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "35$"}, {"For level 35 I have 'Invisible'.", setTopic(2)}},
	{{eq(TOPIC, 2), "38$"}, {"For level 38 I have 'Energy Wave'.", setTopic(2)}},
	{{eq(TOPIC, 2), "41$"}, {"For level 41 I have 'Energy Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "45$"}, {"For level 45 I have 'Sudden Death'.", setTopic(2)}},

	{{eq(TOPIC, 2)}, {"Hmm, I have no spells for this level, but for many levels from 8 to 45.", setTopic(2)}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know how to cast this spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You have to be level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"You need more money."}},
	{{eq(TOPIC, 3), "yes"}, {"Here you are. Look in your spellbook for the pronunciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"Then not."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
