-- npcs/lynda.lua: the behaviour of the NPC Lynda, converted from
-- lynda.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of lynda.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- lynda.npc: Datenbank für die Priesterin Lynda

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local PVPENFORCED = behaviour.PVPENFORCED
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, MALE, "my", "heart", "belongs", "to", FORCE}, {"I ask thee, %N, will you honor your bride and stay at her side even in the darkest hours life could bring upon you?", setTopic(9)}},
	{{ADDRESS, FEMALE, "my", "heart", "belongs", "to", FORCE}, {"I ask thee, %N, will you honor your groom and stay at his side even in the darkest hours life could bring upon you?", setTopic(9)}},
	{{ADDRESS, "hello$", "lynda", FORCE}, {"Welcome in the name of the gods, pilgrim %N!"}},
	{{ADDRESS, "hi$", "lynda", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"%N! Please be patient, my child.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May the gods be with you!"}},

	{{"bye"}, {"Good bye, %N. May the gods guard you, my child!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a priest of the great pantheon."}},
	{{"news"}, {"Sorry, I had no enlightening visions lately."}},
	{{"name"}, {"My name is Lynda. And the spirits tell me that you are %N."}},
	{{"tibia"}, {"The world of Tibia is the creation of the gods."}},
	{{"how", "are", "you"}, {"Thank you, I'm fine, the gods are with me."}},
	{{"sell"}, {"The grace of the gods must be earned, not bought!"}},
	{{"sin$"}, {"Do you whish to confess your sins?", setTopic(3)}},
	{{"sins$"}, {"Do you whish to confess your sins?", setTopic(3)}},
	{{"god$"}, {"The gods of good guard us and guide us, the gods of evil want to destroy us and steal our souls!", setTopic(2)}},
	{{"gods$"}, {previous}},
	{{"life"}, {"Life is a gift of the gods, honor life and don't destroy it."}},
	{{"citizen"}, {"The things I know about our citizens are confidential."}},
	{{"lugri"}, {"He is a follower of evil. May the gods punish him."}},
	{{"king"}, {"King Tibianus is our benevolent sovereign."}},
	{{"monster"}, {"They are creatures of the gods of evil!"}},
	{{"quest"}, {"It is my mission to spread knowledge about the gods."}},
	{{"mission"}, {previous}},
	{{"gold"}, {setPrice(15), "Do you want to make a donation?", setTopic(1)}},
	{{"money"}, {previous}},
	{{"donation"}, {previous}},
	{{"fight"}, {"It is MY mission to teach, it is YOUR mission to fight!"}},
	{{"slay"}, {previous}},

	{{"help", lt(HP, 40), FORCE}, {"You are hurt, my child. I will heal your wounds.", setHp(40), effectOpp(13)}},
	{{"help", gt(POISON, 0), FORCE}, {"You are poisoned, my child. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"help", gt(BURNING, 0), FORCE}, {"You are burning, my child. I will help you.", burning(0, 0), effectOpp(15)}},

	{{"heal$", gt(BURNING, 0)}, {"You are burning, my child. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned, my child. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are hurt, my child. I will heal your wounds.", setHp(40), effectOpp(13)}},

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

	{{"ferumbras"}, {"He is a favourite of the gods of evil and one of the champions of evil."}},
	{{"time"}, {"Now, it is %T."}},
	{{"excalibug"}, {"This fabled weapon was lost in ancient times. If someone found it, this person would be nearly invincible."}},

	{{eq(TOPIC, 2), "good"}, {"The gods we call the good ones are Fardos, Uman, the Elements, Suon, Crunor, Nornur, Bastesh, Kirok, Toth, and Banor."}},
	{{"fardos"}, {"Fardos is the creator, the great obsever. He is our caretaker."}},
	{{"uman"}, {"Uman is the positive aspect of magic. He brings us the secrets of the arcane arts."}},
	{{"suon"}, {"Suon is the lifebringing sun. He observes the creation with love."}},
	{{"crunor"}, {"Crunor, the great tree, is the father of all plantlife. He is a prominent god for many druids."}},
	{{"nornur"}, {"Nornur is the mysterious god of fate. Who knows if he is its creator or just a chronist?"}},
	{{"bastesh"}, {"Bastesh, the deep one, is the goddess of the sea and its creatures."}},
	{{"kirok"}, {"Kirok, the mad one, is the god of scientists and jesters."}},
	{{"toth"}, {"Toth, lord of death, is the keeper of the souls, the guardian of the afterlife."}},
	{{"banor"}, {"Banor, the heavenly warrior, is the patron of all fighters against evil. He is the gift of the gods to inspire humanity."}},
	{{"tibiasula"}, {"Tibiasula lost her life, but out of her essence the world was created."}},

	{{eq(TOPIC, 2), "tibia"}, {"Tibia is the essence of the elemental power of earth."}},
	{{"sula"}, {"Sula is the essence of the elemental power of water."}},
	{{"air"}, {"Air is one of the primal elemental forces, sometimes worshipped by tribal shamans."}},
	{{"fire"}, {"Fire is one of the primal elemental forces, sometimes worshipped by tribal shamans."}},

	{{eq(TOPIC, 2), "evil"}, {"The gods we call the evil ones are Zathroth, Fafnar, Brog, Urgith, and the Archdemons!"}},
	{{"zathroth"}, {"Zathroth is the destructive aspect of magic. He is the deceiver and the thief of souls."}},
	{{"fafnar"}, {"Fafnar is the scorching sun. She observes the creation with hate and jealousy."}},
	{{"brog"}, {"Brog, the raging one, is the great destroyer. The berserk of darkness."}},
	{{"urgith"}, {"The bonemaster Urgith is the lord of the undead and keeper of the damned souls."}},
	{{"archdemons"}, {"The demons are followers of Zathroth. The cruelest are known as the ruthless seven."}},
	{{"ruthless", "seven"}, {"I dont want to talk about that subject!"}},

	{{eq(TOPIC, 1), "no"}, {"As you wish."}},
	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"May the gods bless you!", deleteMoney, effectOpp(15)}},
	{{eq(TOPIC, 1), "yes", lt(COUNTMONEY, PRICE)}, {"Dont be ashamed, but you lack the gold."}},

	{{eq(TOPIC, 3), "yes"}, {"So tell me what shadows your soul, my child.", setTopic(4)}},
	{{eq(TOPIC, 3)}, {"As you wish."}},
	{{eq(TOPIC, 4)}, {"Meditate on that and pray for your soul."}},

	{{"marriage"}, {"You want me to initiate a marriage ceremony?", setTopic(5)}},
	{{"ceremony"}, {previous}},
	{{eq(TOPIC, 5), "yes"}, {"In the Name of the Gods of good, I ask thee, if both of you are prepared and ready!", setTopic(6)}},
	{{eq(TOPIC, 5), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 5)}, {"Perhaps another time. Marriage isn't a step one should consider without love in the heart."}},
	{{eq(TOPIC, 6), "yes"}, {"Silence please! I hereby invoke the attention of the eternal powers looking over our souls and lives. May the gods bless us!", effectMe(13), setTopic(7)}},
	{{eq(TOPIC, 6), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 7), MALE, "may", "gods", "bless", "us"}, {"I ask thee, %N, will you honor your bride and stay at her side even in the darkest hours life could bring upon you?", setTopic(8)}},
	{{eq(TOPIC, 7), FEMALE, "may", "gods", "bless", "us"}, {"I ask thee, %N, will you honor your groom and stay at his side even in the darkest hours life could bring upon you?", setTopic(8)}},
	{{eq(TOPIC, 8), MALE, "yes"}, {"So by the powers of the gods your soul is now bound to your bride. Bride, step forward and tell me to whom your heart belongs!", effectOpp(14), idle}},
	{{eq(TOPIC, 8), MALE, "i$", "will$"}, {previous}},
	{{eq(TOPIC, 8), FEMALE, "yes"}, {"So by the powers of the gods your soul is now bound to your groom. Groom, step forward and tell me to whom your heart belongs!", effectOpp(14), idle}},
	{{eq(TOPIC, 8), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 9), "yes"}, {"So by the powers of the gods your souls are now bound together for eternity. May the gods watch with grace over your further life as a married couple. Go now and celebrate your marriage!", effectOpp(14), effectMe(13), idle}},
	{{eq(TOPIC, 9), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 9), "no"}, {"Your neglection of love hurts my heart. Leave now!", idle}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
