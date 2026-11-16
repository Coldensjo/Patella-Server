-- npcs/rahkem.lua: the behaviour of the NPC Rahkem, converted from
-- rahkem.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of rahkem.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- rahkem.npc: Datenbank für den pyramidenpriester rahkem

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
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
	{{ADDRESS, "hello$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "hi$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "be", "mourned", "pilgrim", "flesh", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Mourned %N, know that I am busy.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "be", "mourned", "pilgrim", "flesh", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Accept my thanks for your gift of silence."}},

	{{"bye"}, {"May enlightenment be your path.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a humble preacher of the true revelation in the temple of the mourned flesh. I heal and teach magic spells to those who are spiritual enough."}},
	{{"name"}, {"I am the mourned Rahkem."}},
	{{"time"}, {"Time is a tool in the hands of the false gods, but it also serves to free us from our mortal prisons."}},

	{{"temple"}, {"Here we mourn our mortal existence. Our flesh is our weakness and our curse, the bait for all the trials and tribulations the false gods let loose on the world."}},
	{{"pharaoh"}, {"Our immortal ruler, may he be blessed, is the keeper of our enlightenment and our saviour."}},
	{{"ashmunrah"}, {"The foolish old pharaoh withheld knowledge and power from his son, knowing that he would surpass him in every aspect. But in his infinite mercy his son granted him the chance to ascend."}},
	{{"scarab"}, {"The eternal burrowers are the keepers of all the secrets their kind has unearthed in countless aeons."}},

	{{"uman"}, {"The beings Uman and Zathroth merged forever in the blaze that followed when the last of the true gods perished."}},
	{{"zathroth"}, {previous}},
	{{"banor"}, {"Banor was the most devout minion of the false gods. Their lickspittle lapdog. Seeing they needed additional strength they granted him some of their powers, and he became a lesser false god himself."}},
	{{"tibia"}, {"It is likely that our world is a part of one of the dead true gods or one of his manifestations that somehow escaped destruction. We must assume this is all that is left of the original universe."}},
	{{"carlin"}, {"The cities that bow to the false gods will be afflicted with plague and fear until they embrace the wisdom of the pharaoh."}},
	{{"thais"}, {previous}},
	{{"edron"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {"The dwarves suffered, but they have drawn wrong conclusions. If they do not listen to the revelations of our immortal pharaoh, pain and grief will prove to be better teachers this time."}},
	{{"dwarves"}, {previous}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"The foolish elves hold on to life too hard to see the way to salvation. However, if we teach them to remove the shackles of flesh through pain and suffering, they might begin to see their mistake."}},
	{{"elves"}, {previous}},
	{{"elfes"}, {previous}},
	{{"darama"}, {"The continent was named after Daraman, the prophet during the reign of Ashmunrah. The new pharaoh acknowledged the power that is in names and did not change the name when he acceded to the throne."}},
	{{"darashia"}, {"The followers of Daraman suffer the curse of the flesh. They can't reach ascension because they never really take the all-important initial step - they only pretend to do so."}},
	{{"initial", "step"}, {"In his preachings Daraman taught that you can conquer the tempations of the flesh through denial. However, the truth is that this constant struggle between temptation and the will blurs your vision, so no follower of Daraman can focus on ascension."}},
	{{"daraman"}, {"We call Daraman the one-eyed prophet, for he clearly saw that ascension is possible, but he was blind to the fact that mortality itself and not mere temptation is the first obstacle that must be overcome."}},
	{{"Ankrahmun"}, {"This city is a marvel of old. Our forefathers built it here on the ruins of an even older civilisation."}},
	{{"pharaoh"}, {"The pharaoh, praised may he be, was the first to learn the truth about mortality, ascension and the false goods."}},
	{{"mortality"}, {"Mortality is our prison. It makes us vulnerable for the temptations of the false gods."}},
	{{"false", "gods"}, {"The so-called gods are just the weakest of their kind. They are pitiful remnants from the terrible godswar between the elder gods which tore the universe appart."}},
	{{"godswar"}, {"In ancient times the elder gods waged war upon each other. Those that call themselves gods today were the lowest of their minions. When the last of the true gods died the great suffering begun."}},
	{{"great", "suffering"}, {"The universe is dying. Death placed his mark on everything. Only the pharaoh can grant us freedom from mortality and open up the path of true ascension to us."}},
	{{"ascension"}, {"The sentient beings are all that is left of the essence of the elder gods. We can awake the dormant powers that slumber in us all. But ascension is a thorny path to follow."}},
	{{"thorny", "path"}, {"Our mortal shells make us vulnerable to the temptations of the false gods. Only by leaving our mortality behind, we can study the true path of ascension. The balance of Akh'rah Uthun has to be changed to our favour."}},
	{{"Akh'rah", "Uthun"}, {"The Akh'rah Uthun is the unity of the Akh, our body, the Rah, our soul and the Uthun, our memories and experiences."}},
	{{"balance"}, {"As long as it is mortal the body breeds temptations and distractions. Its needs make it easy for the false gods to lead us from the path of enlightenment and to ultimately steal our souls."}},
	{{"steal", "souls"}, {"When a mortal is bound to one of the false gods by his faith this god will harvest his Rah on his death and strip away his Uthun, casting it into the void."}},
	{{"Akh"}, {"Your flesh is traitorous and weak. The pharaoh grants the power to conquer death to those who serve him well. Once they have entered this state of being neither dead nor alive they are ready to enter the path of ascension."}},
	{{"undead"}, {"Undeath is freedom from mortal needs. It is the first obvious step to divinity."}},
	{{"undeath"}, {previous}},
	{{"Rah"}, {"The Rah is the ultimate treasure. The false gods need the stolen Rah to sustain their usurped powers."}},
	{{"uthun"}, {"The memory is what makes our personality. It is what defines us ... and its utterly worthless to the gods. For this reason destroy it to harvest our Rah."}},
	{{"mourn"}, {"We mortals are all to be mourned for our prison of flesh. Only through loyal servitude to the pharaoh, praised be his existence, may we escape this prison and find our true destiny."}},

	{{"arena"}, {"The arena is a fitting place to test your mortal shell and to feed the power of the Rah and the Uthun."}},
	{{"palace"}, {"The residence of our immortal king is a temple in its own right because it is the home of a true god."}},
	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are looking really bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},

	{{"heal$", PVPENFORCED}, {"You aren't looking that bad."}},
	{{"heal$"}, {"You aren't looking that bad. Sorry, I can't help you. But if you are looking for additional protection you should go on the pilgrimage of ashes."}},

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

	{{"blessing"}, {"There are five different blessings available in five sacred places. These blessings are: the spiritual shielding, the spark of the phoenix, the embrace of tibia, the fire of the suns and the wisdom of solitude."}},
	{{"sacred", "places"}, {"Just ask in which of the five blessings you are interested in."}},

	{{"spiritual", gt(questValue(104), 0)}, {"I see you received the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding", gt(questValue(104), 0)}, {previous}},
	{{"spiritual"}, {"You can receive the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding"}, {previous}},

	{{"spark", gt(questValue(102), 0)}, {"I can sense that the spark of the phoenix already was given to you by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix", gt(questValue(102), 0)}, {previous}},
	{{"spark"}, {"The spark of the phoenix is given by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix"}, {previous}},

	{{"embrace", gt(questValue(105), 0)}, {"I can sense the druids north of Carlin provided you with the embrace of tibia."}},
	{{"embrace"}, {"The druids north of Carlin can provide you with the embrace of tibia."}},

	{{"fire", "suns", gt(questValue(103), 0)}, {"I can see you received the blessing of the two suns in the suntower near Ab'Dendriel ."}},
	{{"suns", gt(questValue(103), 0)}, {previous}},
	{{"fire", "suns"}, {"Ask for the blessing of the two suns in the suntower near Ab'Dendriel ."}},
	{{"suns"}, {previous}},

	{{"wisdom", gt(questValue(101), 0)}, {"I can sense you already talked to the hermit Eremo on the isle of Cormaya and received this blessing."}},
	{{"solitude", gt(questValue(101), 0)}, {previous}},
	{{"wisdom"}, {"Talk to the hermit Eremo on the isle of Cormaya about this blessing."}},
	{{"solitude"}, {previous}},

	{{"spell", DRUID}, {"I can teach you rune spells and instant spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to druids."}},

	{{DRUID, "rod", lt(questValue(333), 1)}, {"Oh, you did not purchase your first magical rod yet? Please take this little present from me as your magic teacher!", setQuestValue(333, 1), setType(3066), setAmount(1), create(TYPE)}},

	{{eq(TOPIC, 2), "rune", "spell"}, {"I sell attack rune spells, healing rune spells, support rune spells and summon rune spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "instant", "spell"}, {"I sell healing spells, supply spells, support spells and summon spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{eq(TOPIC, 2), "bye"}, {"May enlightenment be your path.", idle}},

	{{DRUID, "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{DRUID, "rune", "spell"}, {"I sell attack rune spells, healing rune spells, support rune spells and summon rune spells. Which of these interests you most?"}},
	{{DRUID, "instant", "spell"}, {"I sell healing spells, supply spells, support spells and summon spells. Which of these interests you most?"}},

	{{DRUID, "attack", "rune", "spell"}, {"I sell missile rune spells, explosive rune spells, field rune spells, wall rune spells and bomb rune spells."}},
	{{DRUID, "healing", "rune", "spell"}, {"In this category I have 'Intense Healing Rune', 'Antidote Rune' and 'Ultimate Healing Rune'."}},
	{{DRUID, "support", "rune", "spell"}, {"In this category I have 'Destroy Field' and 'Chameleon'."}},
	{{DRUID, "summon", "rune", "spell"}, {"In this category I have 'Convince Creature'."}},

	{{DRUID, "missile", "rune", "spell"}, {"In this category I have 'Light Magic Missile' and 'Heavy Magic Missile'."}},
	{{DRUID, "explosive", "rune", "spell"}, {"In this category I have 'Fireball', 'Great Fireball' and 'Explosion'."}},
	{{DRUID, "field", "rune", "spell"}, {"In this category I have 'Poison Field', 'Fire Field' and 'Energy Field'."}},
	{{DRUID, "wall", "rune", "spell"}, {"In this category I have 'Fire Wall', 'Poison Wall' and 'Energy Wall'."}},
	{{DRUID, "bomb", "rune", "spell"}, {"In this category I have 'Firebomb'."}},

	{{DRUID, "healing", "spell"}, {"In this category I have 'Light Healing', 'Intense Healing', 'Ultimate Healing' and 'Antidote'."}},
	{{DRUID, "supply", "spell"}, {"In this category I have 'Food'."}},
	{{DRUID, "support", "spell"}, {"In this category I have 'Find Person', 'Light', 'Great Light', 'Magic Shield', 'Creature Illusion' and 'Invisible'."}},
	{{DRUID, "summon", "spell"}, {"In this category I have 'Summon Creature'."}},

	{{DRUID, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(3)}},
	{{DRUID, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(3)}},
	{{DRUID, "food"}, {setType(42), setPrice(300), "Do you want to buy the spell 'Food' for %P gold?", setTopic(3)}},
	{{DRUID, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "light", "missile"}, {setType(7), setPrice(500), "Do you want to buy the spell 'Light Magic Missile' for %P gold?", setTopic(3)}},
	{{DRUID, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(3)}},
	{{DRUID, "intense", "healing"}, {setType(2), setPrice(350), "Do you want to buy the spell 'Intense Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "poison", "field"}, {setType(26), setPrice(300), "Do you want to buy the spell 'Poison Field' for %P gold?", setTopic(3)}},
	{{DRUID, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "field"}, {setType(25), setPrice(500), "Do you want to buy the spell 'Fire Field' for %P gold?", setTopic(3)}},
	{{DRUID, "heavy", "missile"}, {setType(8), setPrice(1500), "Do you want to buy the spell 'Heavy Magic Missile' for %P gold?", setTopic(3)}},
	{{DRUID, "magic", "shield"}, {setType(44), setPrice(450), "Do you want to buy the spell 'Magic Shield' for %P gold?", setTopic(3)}},
	{{DRUID, "intense", "healing", "rune"}, {setType(4), setPrice(600), "Do you want to buy the spell 'Intense Healing Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "antidote", "rune"}, {setType(31), setPrice(600), "Do you want to buy the spell 'Antidote Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "fireball"}, {setType(15), setPrice(800), "Do you want to buy the spell 'Fireball' for %P gold?", setTopic(3)}},
	{{DRUID, "energy", "field"}, {setType(27), setPrice(700), "Do you want to buy the spell 'Energy Field' for %P gold?", setTopic(3)}},
	{{DRUID, "destroy", "field"}, {setType(30), setPrice(700), "Do you want to buy the spell 'Destroy Field' for %P gold?", setTopic(3)}},
	{{DRUID, "ultimate", "healing"}, {setType(3), setPrice(1000), "Do you want to buy the spell 'Ultimate Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "great", "fireball"}, {setType(16), setPrice(1200), "Do you want to buy the spell 'Great Fireball' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "bomb"}, {setType(17), setPrice(1500), "Do you want to buy the spell 'Fire Bomb' for %P gold?", setTopic(3)}},
	{{DRUID, "creature", "illusion"}, {setType(38), setPrice(1000), "Do you want to buy the spell 'Creature Illusion' for %P gold?", setTopic(3)}},
	{{DRUID, "convince", "creature"}, {setType(12), setPrice(800), "Do you want to buy the spell 'Convince Creature' for %P gold?", setTopic(3)}},
	{{DRUID, "ultimate", "healing", "rune"}, {setType(5), setPrice(1500), "Do you want to buy the spell 'Ultimate Healing Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "chameleon"}, {setType(14), setPrice(1300), "Do you want to buy the spell 'Chameleon' for %P gold?", setTopic(3)}},
	{{DRUID, "poison", "wall"}, {setType(32), setPrice(1600), "Do you want to buy the spell 'Poison Wall' for %P gold?", setTopic(3)}},
	{{DRUID, "explosion"}, {setType(18), setPrice(1800), "Do you want to buy the spell 'Explosion' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "wall"}, {setType(28), setPrice(2000), "Do you want to buy the spell 'Fire Wall' for %P gold?", setTopic(3)}},
	{{DRUID, "Invisible"}, {setType(45), setPrice(2000), "Do you want to buy the spell 'Invisible' for %P gold?", setTopic(3)}},
	{{DRUID, "summon", "creature"}, {setType(9), setPrice(2000), "Do you want to buy the spell 'Summon Creature' for %P gold?", setTopic(3)}},
	{{DRUID, "energy", "wall"}, {setType(33), setPrice(2500), "Do you want to buy the spell 'Energy Wall' for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 2), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "11$"}, {"For level 11 I have 'Intense Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"For level 13 I have 'Great Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "14$"}, {"For level 14 I have 'Food', 'Poison Field' and 'Magic Shield'.", setTopic(2)}},
	{{eq(TOPIC, 2), "15$"}, {"For level 15 I have 'Fire Field', 'Intense Healing Rune', 'Antidote Rune' and 'Light Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "16$"}, {"For level 16 I have 'Convince Creature'.", setTopic(2)}},
	{{eq(TOPIC, 2), "17$"}, {"For level 17 I have 'Fireball' and 'Destroy Field'.", setTopic(2)}},
	{{eq(TOPIC, 2), "18$"}, {"For level 18 I have 'Energy Field'.", setTopic(2)}},
	{{eq(TOPIC, 2), "20$"}, {"For level 20 I have 'Ultimate Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "23$"}, {"For level 23 I have 'Great Fireball' and 'Creature Illusion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "24$"}, {"For level 24 I have 'Ultimate Healing Rune'.", setTopic(2)}},
	{{eq(TOPIC, 2), "25$"}, {"For level 25 I have 'Summon Creature' and 'Heavy Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "27$"}, {"For level 27 I have 'Firebomb' and 'Chameleon'.", setTopic(2)}},
	{{eq(TOPIC, 2), "29$"}, {"For level 29 I have 'Poison Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "31$"}, {"For level 31 I have 'Explosion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "33$"}, {"For level 33 I have 'Fire Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "35$"}, {"For level 35 I have 'Invisible'.", setTopic(2)}},
	{{eq(TOPIC, 2), "41$"}, {"For level 41 I have 'Energy Wall'.", setTopic(2)}},

	{{eq(TOPIC, 2)}, {"Sorry, I have only spells for level 8 to 11, 13 to 18, 20, 23 to 25 as well as for the levels 27, 29, 31, 33, 35 and 41.", setTopic(2)}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know how to cast this spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You have to be level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 3), "yes"}, {"Here you are. Look in your spellbook for the pronunciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
