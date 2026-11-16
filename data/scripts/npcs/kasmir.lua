-- npcs/kasmir.lua: the behaviour of the NPC Kasmir, converted from
-- kasmir.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of kasmir.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Kasmir.npc: Datenbank für den Priester Kasmir

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
	{{ADDRESS, MALE, "my", "heart", "belongs", "to", FORCE}, {"I ask thee, %N, will you honor your bride and stay at her side even in the darkest hours live could bring upon you?", setTopic(9)}},
	{{ADDRESS, FEMALE, "my", "heart", "belongs", "to", FORCE}, {"I ask thee, %N, will you honor your groom and stay at his side even in the darkest hours live could bring upon you?", setTopic(9)}},
	{{ADDRESS, "hello$", FORCE}, {"May Daraman enlighten you %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"May Daraman fulfill you with patience, %N. Wait please.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Gone like a Djinn..."}},

	{{"bye"}, {"Good bye, %N. May Daraman's all-seeing eye watch your travels!", idle}},
	{{"job"}, {"I am a chosen of Daraman."}},
	{{"news"}, {"Don't look for news when you don't know the past."}},
	{{"name"}, {"Kasmir Ibn Darasir."}},
	{{"tibia"}, {"The world is only a portal the gods created to allow our ascension to heaven."}},
	{{"ascension"}, {"Daraman had a vision that all mortals are able to ascend to heaven, becoming celestial beings."}},
	{{"heaven"}, {previous}},
	{{"celestial"}, {"By enhancing one's soul a mortal can ascend to heaven. If you are not prepared to ascend, you are bound to this world by reincarnation."}},
	{{"reincarnation"}, {"If your soul is not strong and purified, you will not ascend but return to life on death, even losing strength in the process."}},
	{{"necromancer"}, {"Undeath is even worse than reincarnation. Those souls are nothing but a rotting mockery of a soul on the path of ascension."}},
	{{"daraman"}, {"Daraman travelled the world and learned the secrets of the ancients. At last he learned the secret of ascension and founded his philosophy."}},
	{{"soul"}, {"The soul was made by the gods and therefore is divine. So by enhancing its divinity it can become more like the image of its creators."}},
	{{"philosophy"}, {"Daraman led his followers to this promised land to follow his teachings. It was named Darama after him later."}},
	{{"darama"}, {"This land is harsh and challenging. It's far away from temptations and delusions. Here Daraman's people can concentrate on themselves."}},
	{{"how", "are", "you"}, {"Thank you, I'm fine and my soul is strong."}},
	{{"sell"}, {"Go to the bazaar if you are interested in worldly wealth."}},
	{{"sin$"}, {"Do you whish to confess your sins?", setTopic(3)}},
	{{"sins$"}, {previous}},
	{{"god$"}, {"The gods are in heaven and far away. You are here. So concentrate on your soul and take care for it on your own."}},
	{{"gods$"}, {previous}},
	{{"life"}, {"Life is divine though not without flaws."}},
	{{"citizen"}, {"The people are too concerned about the illusions of the moment and care less and less about Daraman's philosophy."}},
	{{"caliph"}, {"The caliph is heavily involved in the affairs in the world, but one has to make this sacrifice for the welfare of all."}},
	{{"monster"}, {"They misuse their god given souls for evil and must be destroyed."}},
	{{"quest"}, {"Your quest should be to prepare your soul for ascension."}},
	{{"mission"}, {previous}},
	{{"gold"}, {setPrice(15), "Do you want to make a donation?", setTopic(1)}},
	{{"money"}, {previous}},
	{{"donation"}, {previous}},
	{{"fight"}, {"Fight is unavoidable sometimes. Make sure only your body is hurt but not your soul."}},
	{{"slay"}, {previous}},

	{{"heal$", lt(HP, 40)}, {"You are hurt, pilgrim. I will heal your wounds.", setHp(40), effectOpp(13)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned, pilgrim. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", gt(BURNING, 0)}, {"You are burning, pilgrim. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$"}, {"You aren't looking that bad. Sorry, I can't help you. But if you are looking for additional protection you should go on the pilgrimage of ashes."}},
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

	{{"help$", lt(HP, 40)}, {"You are hurt, pilgrim. I will heal your wounds.", setHp(40), effectOpp(13)}},
	{{"help$", gt(POISON, 0)}, {"You are poisoned, pilgrim. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"help$", gt(BURNING, 0)}, {"You are burning, pilgrim. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"help$"}, {"You aren't looking so bad. Sorry, I need my powers for cases more severe than yours."}},

	{{"ferumbras"}, {"His soul is corrupted beyond any hope for ascension."}},
	{{"time"}, {"Now, it is %T."}},
	{{"excalibug"}, {"Your greed for such items can easily corrupt your soul."}},
	{{"fardos"}, {"Fardos is the creator. It was his work that we possess divine souls."}},
	{{"uman"}, {"Uman is the positive aspect of magic. His powers flow through each Tibian, making us his children."}},
	{{"suon"}, {"Suon is the sun. He watches our ascension."}},
	{{"crunor"}, {"Crunor is the force of life and part of our all being."}},
	{{"nornur"}, {"Nornur is the mysterious god of fate. Daraman taught us that he is the judge who allows ascension."}},
	{{"bastesh"}, {"Bastesh is the goddess of the seas, and deep as the see is our soul, indeed."}},
	{{"kirok"}, {"Kirok is called the mad one. He gifted us with the creativity to achive our ascension."}},
	{{"toth"}, {"Toth is the final judge. The unworthy are condemened to reincarnation."}},
	{{"banor"}, {"Banor is the very proof that ascension is possible."}},
	{{"tibiasula"}, {"Though this entity is dead, Tibiasula's energy is present in all of us."}},
	{{"tibia"}, {"Tibia is the raw force of earth."}},
	{{"sula"}, {"Sula is the raw force of water."}},
	{{"air"}, {"Air is without true mind and meaning."}},
	{{"fire"}, {"Fire is without true mind and meaning."}},
	{{"zathroth"}, {"Zathroth is the corruptor of souls who does not want mortals to ascend and become more like him."}},
	{{"fafnar"}, {"Fafnar is a test for our endurance and dilligence."}},
	{{"brog"}, {"Brog's hot blood is in our veins, tempting us and distracting us from improvement of our souls."}},
	{{"urgith"}, {"The bonemaster is strong in the ruins of Drefia. There you can test the braveness of your soul ... or lose it to his minions."}},
	{{"archdemons"}, {"They are soulless and therefore without true power in the end."}},
	{{"ruthless", "seven"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"May Daraman guide your quest for ascension.", deleteMoney, effectOpp(15)}},
	{{eq(TOPIC, 1), "yes"}, {"Don't be ashamed, but you lack the gold."}},
	{{eq(TOPIC, 1), "no"}, {"As you wish."}},

	{{eq(TOPIC, 3), "yes"}, {"So what does trouble your soul, pilgrim?", setTopic(4)}},
	{{eq(TOPIC, 3)}, {"As you wish."}},
	{{eq(TOPIC, 4)}, {"Meditate on that and try harder to improve soul."}},

	{{"marriage"}, {"You want me to initiate a marriage ceremony?", setTopic(5)}},
	{{"ceremony"}, {previous}},
	{{eq(TOPIC, 5), "yes"}, {"In the Name of the Gods of good, I ask thee, if both of you are prepared and ready!", setTopic(6)}},
	{{eq(TOPIC, 5), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 5)}, {"Perhaps another time. Marriage isn't a step one should consider without love in the heart."}},
	{{eq(TOPIC, 6), "yes"}, {"Silence please! I hereby invoke the attention of the eternal powers looking over our souls and lives. May the gods bless us!", effectMe(13), setTopic(7)}},
	{{eq(TOPIC, 6), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 7), MALE, "may", "gods", "bless", "us"}, {"I ask thee, %N, will you honor your bride and stay at her side even in the darkest hours live could bring upon you?", setTopic(8)}},
	{{eq(TOPIC, 7), FEMALE, "may", "gods", "bless", "us"}, {"I ask thee, %N, will you honor your groom and stay at his side even in the darkest hours live could bring upon you?", setTopic(8)}},
	{{eq(TOPIC, 8), MALE, "yes"}, {"So by the powers of the gods your soul is now bound to your bride. Bride, step forward and tell me to whom your heart belongs!", effectOpp(14), idle}},
	{{eq(TOPIC, 8), MALE, "i$", "will$"}, {previous}},
	{{eq(TOPIC, 8), FEMALE, "yes"}, {"So by the powers of the gods your soul is now bound to your groom. Groom, step forward and tell me to whom your heart belongs!", effectOpp(14), idle}},
	{{eq(TOPIC, 8), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 9), "yes"}, {"So by the powers of the gods your souls are now bound together for eternity. May the gods watch with grace over your further live as a married couple. Go now and celebrate your marriage!", effectOpp(14), effectMe(13), idle}},
	{{eq(TOPIC, 9), "i$", "will$"}, {previous}},
	{{eq(TOPIC, 9), "no"}, {"Your neglection of love hurts my heart. Leave now!", idle}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
