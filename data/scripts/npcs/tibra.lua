-- npcs/tibra.lua: the behaviour of the NPC Tibra, converted from
-- tibra.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of tibra.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- tibra.npc: Datenbank für die Priesterin Tibra

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
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
	{{ADDRESS, "hello$", "tibra", FORCE}, {"Welcome in the name of the gods, pilgrim %N!"}},
	{{ADDRESS, "hi$", "tibra", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{ADDRESS, MALE, "my$", "heart$", "belongs", "to", FORCE}, {"I ask thee, %N, will you honor your bride and stay at her side even in the darkest hours live could bring upon you?", setTopic(9)}},
	{{ADDRESS, FEMALE, "my$", "heart$", "belongs", "to", FORCE}, {"I ask thee, %N, will you honor your groom and stay at his side even in the darkest hours live could bring upon you?", setTopic(9)}},

	{{BUSY, "hello$", FORCE}, {"%N, please be patient and wait a moment, my child.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May the gods guard you!"}},

	{{"bye"}, {"Good bye, %N. May the gods be with you to guard and guide you, my child!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a priest of the great pantheon."}},
	{{"news"}, {"Sorry, worldly matters are of no concern to me."}},
	{{"name"}, {"My name is Tibra. Your soul tells me that you are %N"}},
	{{"tibia"}, {"The world of Tibia is the creation of the gods."}},
	{{"how", "are", "you"}, {"Thank you, I'm fine, the gods give me hope and comfort."}},
	{{"sell"}, {"The grace of the gods must be earned, it cannot be bought!"}},
	{{"sin$"}, {"Do you wish to confess your sins, my chilid?", setTopic(3)}},
	{{"sins$"}, {previous}},

	{{"marriage", "ceremony"}, {"So you want me to initate a marriage ceremony?", setTopic(5)}},
	{{eq(TOPIC, 5), "yes"}, {"In the name of the Gods of good, I ask thee, if both of you are prepared and ready!", setTopic(6)}},
	{{eq(TOPIC, 5), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 5)}, {"Perhaps another time, marriage isn't a step one should consider without love in the heart."}},
	{{eq(TOPIC, 6), "yes"}, {"Silence, please! I hereby invoke the attention of the eternal Powers looking over our souls and lives. May the gods bless us!", effectMe(2), setTopic(7)}},
	{{eq(TOPIC, 6), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 7), MALE, "may", "gods", "bless", "us"}, {"I ask thee %N, will you honor your bride and stay at her side even in the darkest hours live could bring upon you?", setTopic(8)}},
	{{eq(TOPIC, 7), FEMALE, "may", "gods", "bless", "us"}, {"I ask thee %N, will you honor your groom and stay at his side even in the darkest hours live could bring upon you?", setTopic(8)}},
	{{eq(TOPIC, 8), MALE, "yes"}, {"So by the powers of the gods your soul is now bound to your bride. Bride, step forward and tell me to whom your heart belongs!", effectOpp(1), idle}},
	{{eq(TOPIC, 8), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 8), FEMALE, "yes"}, {"So by the powers of the gods your soul is now bound to your groom. Groom, step forward and tell me to whom your heart belongs!", effectOpp(1), idle}},
	{{eq(TOPIC, 8), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 9), "yes"}, {"So by the powers of the gods your souls are now bound together for eternity. May the gods watch with grace over your further life as married couple! Go now and celebrate your marriage!", effectOpp(1), effectMe(7), idle}},
	{{eq(TOPIC, 9), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 9), "no"}, {"Your neglection of love hurts my heart. Leave now!", idle}},

	{{"god$"}, {"The gods of good guard us and guide us, the gods of evil want to destroy us and steal our souls!", setTopic(2)}},
	{{"gods$"}, {previous}},
	{{"life"}, {"The teachings of Crunor tell us to honor life and not to harm it."}},
	{{"citizen"}, {"The things we priests know about the citizens are confidential."}},
	{{"lugri"}, {"Only a man can fall as low as he did. His soul rotted away already."}},
	{{"queen"}, {"Queen Eloise is wise to listen to the proposals of the druidic followers of Crunor."}},
	{{"monster"}, {"Remind: Not everything you call monster is evil to the core!"}},
	{{"quest"}, {"It is my mission to bring the teachings of the gods to everyone."}},
	{{"mission"}, {previous}},
	{{"gold"}, {setPrice(15), "Do you want to make a donation?", setTopic(1)}},
	{{"money"}, {previous}},
	{{"donation"}, {previous}},
	{{"fight"}, {"It is MY mission to teach, it is YOUR mission to fight!"}},
	{{"slay"}, {previous}},
	{{"mission"}, {previous}},
	{{"heal$", lt(HP, 40)}, {"You are hurt, my child. I will heal your wounds.", setHp(40), effectOpp(13)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned, my child. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", gt(BURNING, 0)}, {"You are burning, my child. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$"}, {"You aren't looking that bad. Sorry, I need my powers for cases more severe than yours."}},
	{{"help", lt(HP, 40)}, {"You are hurt, my child. I will heal your wounds.", setHp(40), effectOpp(13)}},
	{{"help", gt(POISON, 0)}, {"You are poisoned, my child. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"help", gt(BURNING, 0)}, {"You are burning, my child. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"help"}, {"You aren't looking that bad. Sorry, I need my powers for cases more severe than yours."}},
	{{"ferumbras"}, {"The fallen one should be mourned, not feared."}},
	{{"time"}, {"Now, it is %T."}},
	{{"excalibug"}, {"The mythical blade was hidden in ancient times. Its said that powerful wards protect it."}},
	{{"graveyard"}, {"There's something strange in its neighbourhood. But whom we gonna call for help if not the gods?"}},
	{{"crypt"}, {previous}},
	{{"mausoleum"}, {previous}},

	{{eq(TOPIC, 2), "good"}, {"The gods we call the good ones are Fardos, Uman, the Elements, Suon, Crunor, Nornur, Bastesh, Kirok, Toth, and Banor."}},
	{{"fardos"}, {"Fardos is the creator. The great obsever. He is our caretaker."}},
	{{"uman"}, {"Uman is the positive aspect of magic. He brings us the secrets of the arcane arts."}},
	{{"suon"}, {"Suon is the lifebringing sun. He observes the creation with love."}},
	{{"crunor"}, {"Crunor, the great tree, is the father of all plantlife. He is a prominent god for many druids."}},
	{{"nornur"}, {"Nornur is the mysterious god of fate. Who knows if he is its creator or just a chronist?"}},
	{{"bastesh"}, {"Bastesh, the deep one, is the goddess of the sea and its creatures."}},
	{{"kirok"}, {"Kirok, the mad one, is the god of scientists and jesters."}},
	{{"toth"}, {"Toth, Lord of Death, is the keeper of the souls, the guardian of the afterlife."}},
	{{"banor"}, {"Banor, the heavenly warrior, is the patron of all fighters against evil. He is the gift of the gods to inspire humanity."}},
	{{"tibiasula"}, {"Tibiasula lost her life, but out of her essence the world was created."}},
	{{eq(TOPIC, 2), "tibia"}, {"Tibia is the essence of the elemental power of earth."}},
	{{"sula"}, {"Sula is the essence of the elemental power of water."}},
	{{"air"}, {"Air is one of the primal elemental forces, sometimes worshipped by tribal shamans."}},
	{{"fire"}, {"Fire is one of the primal elemental forces, sometimes worshipped by tribal shamans."}},

	{{eq(TOPIC, 2), "evil"}, {"The gods we call the evil ones are Zathroth, Fafnar, Brog, Urgith, and the Archdemons!"}},
	{{"zathroth"}, {"Zathroth is the destructive aspect of magic. He is the deciver and the thief of souls."}},
	{{"fafnar"}, {"Fafnar is the scorching sun. She observes the creation with hate and jealousy."}},
	{{"brog"}, {"Brog, the raging one, is the great destroyer. The berserk of darkness."}},
	{{"urgith"}, {"The bonemaster Urgith is the lord of the undead and keeper of the damned souls."}},
	{{"archdemons"}, {"The demons are followers of Zathroth. The cruelest are known as the ruthless seven."}},
	{{"ruthless", "seven"}, {"I dont want to talk about that subject!"}},

	{{eq(TOPIC, 1), "no"}, {"As you wish."}},
	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"May the gods bless you!", deleteMoney, effectOpp(15)}},
	{{eq(TOPIC, 1), "yes", lt(COUNTMONEY, PRICE)}, {"Don't be ashamed, but you lack the gold."}},

	{{eq(TOPIC, 3), "yes"}, {"So tell me, what shadows your soul, my child.", setTopic(4)}},
	{{eq(TOPIC, 3)}, {"As you wish."}},
	{{eq(TOPIC, 4)}, {"Meditate on that and pray for your soul."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
