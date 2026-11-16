-- npcs/benjamin.lua: the behaviour of the NPC Benjamin, converted from
-- benjamin.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of benjamin.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- benjamin.npc: Datenbank für den Postmann Benjamin

local behaviour = require("lib.npc")
local genPost = require("lib.npcs.gen-post")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local lt = behaviour.lt
local gt = behaviour.gt
local questValue = behaviour.questValue
local add = behaviour.add
local setQuestValue = behaviour.setQuestValue
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hello. How may I help you %N?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"I am already talking to a customer, %N. Please wait a moment.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"It was a pleasure to help you %N."}},

	{{"bye"}, {"It was a pleasure to help you.", idle}},
	{{"farewell"}, {previous}},

	{{"kevin"}, {"That name sounds familiar... who might that be..."}},
	{{"postner"}, {previous}},
	{{"postmasters", "guild"}, {"Hm, I think I heard about that guild... oh wait, I am a member!"}},
	{{"join"}, {"Uh... oh... Uhm... Join what?"}},
	{{"headquarter"}, {"Its just... I mean... there was that road, oh yes, its that house at that road."}},

	{{"measurements", gt(questValue(234), 0), lt(questValue(236), 1)}, {"Oh they dont change that much since in the old days as... <tells a boring and confusing story about a cake, a parcel, himself and two squirrels, at least he tells you his measurements in the end>", setQuestValue(234, add(questValue(234), 1)), setQuestValue(236, 1)}},

	{{"job"}, {"I am working here at the post office. If you have questions about the Royal Tibia Mail System or the depots ask me."}},
	{{"office"}, {"I am always in my office. You are welcome at any time."}},
	{{"name"}, {"My name is Benjamin."}},
	{{"time"}, {"Now it's %T. Maybe you want to buy a watch?"}},
	--"mail"      -> "Our mail system is unique! And everyone can use it. Do you want to know more about it?", Topic=1
	{{"depot"}, {"The depots are very easy to use. Just step in front of them and you will find your items in them. They are free for all tibian citizens. Hail our king!"}},
	{{"king"}, {"Oops, the king? I... can't remember his name..."}},
	{{"tibianus"}, {"Ah, King Tibianus, our wise ruler. He is sick for some time, isn't he?"}},
	{{"quentin"}, {"Ooooh, nice man, visits me often... I think."}},
	{{"lynda"}, {"She is SO pretty!"}},
	{{"harkath"}, {"Oh, young Harkath will be a fine warrior some day."}},
	{{"army"}, {"TO THE ARMS! MAN THE WALLS! FERUMBRAS IS NEAR!", idle}},
	{{"ferumbras"}, {previous}},
	{{"general"}, {previous}},
	{{"sam"}, {"Ham? No thanks, I ate fish already."}},
	{{"frodo"}, {"Frodo... Frodo... ? Uhm... isn't that the man that brings me food at lunchtime?"}},
	{{"gorn"}, {"He sells equipment."}},
	{{"elane"}, {"Oh, she lives next door. I think she's a dentist, I sometimes hear some cries."}},
	{{"muriel"}, {"This Muriel has a lot of correspondence."}},
	{{"gregor"}, {"Never heared of him."}},
	{{"marvik"}, {"He is always talking of healing me but I am fine... I fear he is a little nuts, poor man."}},
	{{"bozo"}, {"He hangs around here quite often. He claimes, I inspire him."}},
	{{"baxter"}, {"This naughty child, always stealing apples!"}},
	{{"sherry"}, {"I don't drink alcohol while on duty."}},
	{{"lugri"}, {"NO! NO! NO! GO AWAY!.", idle}},
	{{"excalibug"}, {"I can't remember that someone named like that lives here."}},
	{{"news"}, {"Sorry, I don't read the letters we transmit."}},
	{{"thais"}, {"This is the town you are currently in."}},
	{{"carlin"}, {"You can sent letters and parcels to Carlin."}},
	{{"xodet"}, {"The young sorcerer is a good businessman."}},
	{{"quero"}, {"I love his music! He is my best friend and I visit him as often as I can."}},

	genPost,

	--"letter" -> Amount=1, Price=5,  "Do you want to buy a letter for %P gold?", Topic=2
	--"parcel" -> Amount=1, Price=10, "Do you want to buy a parcel for %P gold?", Topic=3

	--Topic=1,"yes" -> "The Tibia Mail System enables you to send and receive letters and parcels. You can buy them here if you want."
	--Topic=1       -> "Is there anything else I can do for you?"

	--Topic=2,"yes",CountMoney>=Price -> "Here it is. Don't forget to write the name of the receiver in the first line and the address in the second one before you put the letter in a mailbox.", DeleteMoney, Create(3505)
	--Topic=2,"yes"                   -> "Oh, you have not enough gold to buy a letter."
	--Topic=2                         -> "Ok."

	--Topic=3,"yes",CountMoney>=Price -> "Here you are. Don't forget to write the name and the address of the receiver on the label. The label has to be in the parcel before you put the parcel in a mailbox.", DeleteMoney, Create(3503), Create(3507)
	--Topic=3,"yes"                   -> "Oh, you have not enough gold to buy a parcel."
	--Topic=3                         -> "Ok."
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
