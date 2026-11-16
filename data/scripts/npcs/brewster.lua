-- npcs/brewster.lua: the behaviour of the NPC Brewster, converted from
-- brewster.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of brewster.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- brewster.npc: Datenbank für den priester brewster

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PVPENFORCED = behaviour.PVPENFORCED
local FORCE = behaviour.FORCE
local lt = behaviour.lt
local gt = behaviour.gt
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local questValue = behaviour.questValue
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local effectOpp = behaviour.effectOpp
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"G...greetings <hicks>."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Uh? Gimme a break. As you can see there's another one first.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye ... and now I'll have a quick drink."}},

	{{"bye"}, {"Exactly! <hicks>", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a priest. <hicks> The worldly representative of the gods so to speak. <hicks> Not that I would say such a thing of course. <hicks> This would be vanity after all."}},
	{{"name"}, {"I am ... ah, yes, Brewster. That's me, my name I mean <hicks>."}},
	{{"time"}, {"Uhm ... Uh ... No idea, sorry."}},
	{{"temple"}, {"Hehe! <hicks> Well if you call this hut a temple you are not a devoted churchgoer I guess. But never mind, I won't tell anyone and the gods know it anyway ... if they care."}},
	{{"king"}, {"Ah the king, how lucky he must be - being the ruler of this lovely little piece of dirt here. Hehe."}},
	{{"venore"}, {"Venore, Venore, city of splendour. Hm, the best thing about that city is its brewery."}},
	{{"thais"}, {"Thais!! My beloved hometown! Oh how I miss my good, old Thais."}},
	{{"carlin"}, {"Ha! That's probably even worse than this dump of a jungle here that they call a colony."}},
	{{"edron"}, {"They would never appoint a priest of such a low rank like me to Edron."}},
	{{"jungle"}, {"This jungle must be the way of the gods to give us mortals a taste of hell <hicks>."}},
	{{"gods"}, {"Oh come on, just leave me alone. <hicks> Read a book to find out more."}},

	{{"tibia"}, {"If Tibia is a fallen god, make your guess what bodypart you are on now. <hicks> I have my assumptions ... but I won't tell. Hehe."}},

	{{"kazordoon"}, {"The dwarves I met can't stop to praise the dwarven beer. That wakes the urge in me to ... uhm spread the word of our gods in that city of Kazordoon."}},
	{{"dwarves"}, {previous}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"Was never there <hicks> For all what I have heard it's not that much different from this ugly little settlement."}},
	{{"elves"}, {"After being in that jungle for a while, I can't trust people that love trees anymore."}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"The teachings of our temple counts little on this continent. I think it's a sign from the gods to abandon it. But why should anyone listen to poor old Brewster?"}},
	{{"ankrahmun"}, {"Just to think about this cursed town and its inhabitants makes me shiver. I better take a quick drink to forget about it."}},
	{{"ferumbras"}, {"<hicks> Oh well, he is just that what I'd expect next in all my misery."}},
	{{"excalibug"}, {"Who knows if it is real or just some myth? <hicks> And who cares at all?"}},

	{{"apes"}, {"They don't believe me but I have seen them. There are pink apes! They come when I am sleeping and try to steal my beer and wine <hicks>."}},
	{{"lizard"}, {"They usually stay away from here so who cares?"}},
	{{"dworcs"}, {"Heard enough of them to dislike them."}},

	{{"cough", "syrup"}, {"The only person who might have some cough syrup is this druid Ustan. You find him in the tavern. Hmmm the tavern ... <hicks>"}},

	{{"help", lt(HP, 40), FORCE}, {"You are hurt my child. I will heal your wounds.", setHp(40), effectOpp(13)}},
	{{"help", gt(POISON, 0), FORCE}, {"You are poisoned my child. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"help", gt(BURNING, 0), FORCE}, {"You are burning my child. I will help you.", burning(0, 0), effectOpp(15)}},

	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are looking really bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},

	{{"heal$", PVPENFORCED}, {"You aren't looking that bad."}},
	{{"heal$"}, {"You aren't looking that bad. Sorry, I can't help you. But if you are looking for additional protection you should go on the pilgrimage of ashes."}},

	{{"blessing", PVPENFORCED, FORCE}, {"The vital force of this world is waning. There are no more blessings available on this world."}},
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

	{{"pilgrimage"}, {"Whenever you receive a lethal wound your vital force is damaged. Each one of the five blessings will reduce this damage."}},
	{{"ashes"}, {previous}},
	{{"blessing"}, {"There are five different blessings available in five sacred places. These blessings are: the spiritual shielding, the spark of the phoenix, the embrace of Tibia, the fire of the suns and the wisdom of solitude."}},
	{{"sacred", "places"}, {"Just tell me in which of the five blessings you are interested."}},

	{{"spiritual", gt(questValue(104), 0)}, {"I see you have received the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding", gt(questValue(104), 0)}, {previous}},
	{{"spiritual"}, {"You can receive the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding"}, {previous}},

	{{"spark", gt(questValue(102), 0)}, {"I can sense that the spark of the phoenix has already been given to you by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix", gt(questValue(102), 0)}, {previous}},
	{{"spark"}, {"The spark of the phoenix is given by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix"}, {previous}},

	{{"embrace", gt(questValue(105), 0)}, {"I can sense the druids north of Carlin have provided you with the embrace of Tibia."}},
	{{"embrace"}, {"The druids north of Carlin can provide you with the embrace of Tibia."}},

	{{"fire", "suns", gt(questValue(103), 0)}, {"I can see you have received the blessing of the two suns in the suntower near Ab'Dendriel."}},
	{{"suns", gt(questValue(103), 0)}, {previous}},
	{{"fire", "suns"}, {"Ask for the blessing of the two suns in the suntower near Ab'Dendriel ."}},
	{{"suns"}, {previous}},

	{{"wisdom", gt(questValue(101), 0)}, {"I can sense you have already talked to the hermit Eremo on the isle of Cormaya and received this blessing."}},
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
