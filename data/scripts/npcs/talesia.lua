-- npcs/talesia.lua: the behaviour of the NPC Talesia, converted from
-- talesia.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of talesia.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Talesia.npc: Datenbank für die Besitzerin des Grünzeugmarktes Talesia

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Salutation, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Come back later, I am talking."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Fare thee well."}},

	{{"bye"}, {"Fare thee well.", idle}},
	{{"name"}, {"I am Talesia De'Mir, owner of Crunor's Finest Warehouse."}},
	{{"job"}, {"I am a the owner of Crunor's Finest Warehouse of course."}},
	{{"time"}, {"It's %T right now."}},
	{{"king"}, {"We pay this man enough to live here undisturbed of major interventions."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"At least they are useful, but we pay enough taxes to supply the entire Thaian army."}},
	{{"ferumbras"}, {"I hope he is aware that his enemies live elsewhere."}},
	{{"excalibug"}, {"I am contend with my familysword meloncutter."}},
	{{"thais"}, {"I hope they live well from our taxes..."}},
	{{"tibia"}, {"The world is a treasure chest for those of knowledge and skill."}},
	{{"warehouse"}, {"My warehouse is only one of many. We merchants hold this city together and lead it to prosperity."}},
	{{"carlin"}, {"They are a bit problematic as business partners, but their independence from Thais is... interesting."}},
	{{"news"}, {"Even bad news can be good news, if you play your cards well."}},
	{{"tax"}, {"Venore is the major tax payer in the whole realm. So we more than deserve the privileges the king granted us."}},
	{{"crunor"}, {"A god worth to worship. At least he gives something useful back to the faithful."}},
	{{"privilege"}, {"We are alowed to trade with anyone, Thaian subject or not, have no Thaian noble as governor, and own the exclusive gambling license."}},
	{{"gambling"}, {"I don't care much about it, though others profit greatly."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
