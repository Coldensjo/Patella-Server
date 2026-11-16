-- npcs/hugo.lua: the behaviour of the NPC Hugo, converted from
-- hugo.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of hugo.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- hugo.npc: Datenbank für den Besitzer des Modehauses Hugo Chief

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local questValue = behaviour.questValue
local setQuestValue = behaviour.setQuestValue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Be greeted, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hail$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Perhaps we can chat later, %N."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hail$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye, %N."}},

	{{"bye"}, {"Good bye, %N.", idle}},
	{{"name"}, {"I am known as Hugo Chief."}},
	{{"hugo"}, {"Well it's not my real name. I took it because people think it's scaring and manly. I hate people doubting my manhood for being a tailor, you know."}},
	{{"real", "name"}, {"Uhm, well it's Oscar Savage, but who can become famous, especially as an artist, with a name like that I ask you?"}},
	{{"job"}, {"I am tailor and designer extraordinaire."}},
	{{"warehouse"}, {"I would call it a 'wearhouse'."}},

	{{"uniform", eq(questValue(233), 1)}, {"A new uniform for the post officers? I am sorry but my dog ate the last dress pattern we used. You need to supply us with a new dress pattern."}},
	{{"dress", "pattern", eq(questValue(233), 1)}, {"It was ... wonderous beyond wildest imaginations! I have no clue where Kevin Postner got it from. Better ask him.", setQuestValue(233, 2)}},
	{{"dress", "pattern", gt(questValue(233), 1), lt(questValue(233), 9)}, {"I already told you to ask your boss about that issue."}},
	{{"dress", "pattern", eq(questValue(233), 9)}, {"By the gods of fashion! Didn't it do that I fed the last dress pattern to my poor dog? Will this mocking of all which is taste and fashion never stop?? Ok, ok, you will get those ugly, stinking uniforms and now get lost, fashion terrorist.", setQuestValue(233, 10), idle}},

	{{"uniform"}, {"I don't get it, what uniforms you are talking about."}},

	{{"time"}, {"Sorry, a watch would ruin my stylish outfit."}},
	{{"king"}, {"He does not care much about us, we don't care much about him. I consider that a fair deal."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"I think they should not wear that ugly armor in town. I will see to assure that will be changed soon."}},
	{{"ferumbras"}, {"The ferumbras-bad-ass-fashion is incredibly outdated since years."}},
	{{"excalibug"}, {"I don't care for such fairytales."}},
	{{"thais"}, {"Thais is kind of a fashion hell. If there was an award for the most ugly citizens, it would go to Thais."}},
	{{"tibia"}, {"A world filled with ugly dressed people needs the skills of a fashion-hero."}},
	{{"uglyness"}, {previous}},
	{{"punished"}, {previous}},
	{{"fashion", "hero"}, {"One day Captain Catwalk will punish all crimes to fashion and bring all ugly people to justice."}},
	{{"captain", "catwalk"}, {"He's a supermodel! No one knows his secret identity!"}},
	{{"supermodel"}, {"A model with incredible superpowers. Do you think they call them supermodels for nothing?"}},
	{{"carlin"}, {"Women should know better than to hide in ugly armor. Like all followers of ugliness they will be punished one day."}},
	{{"news"}, {"My newest models are top secret, sorry."}},
	{{"tax"}, {"I don't care about such mundane things like 'taxes'."}},
	{{"privilege"}, {"The city was granted a few privileges by the king. I can't even tell which. They don't affect me that much."}},
	{{"gambling"}, {"I too love to gamble now and then in the Hard Rock tavern."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
