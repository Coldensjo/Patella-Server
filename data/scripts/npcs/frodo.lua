-- npcs/frodo.lua: the behaviour of the NPC Frodo, converted from
-- frodo.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of frodo.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- frodo.npc: Datenbank fuer den Wirt Frodo

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", "frodo", FORCE}, {"Hello, hello, %N. You heard about the news?"}},
	{{ADDRESS, "hi$", "frodo", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Welcome to Frodo's Hut. You heard about the news?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just a minute, %N. I'll be with you in no time.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Please come back from time to time."}},

	{{"bye"}, {"Please come back from time to time.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the owner of this saloon. I call it Frodo's Hut. I am also selling food."}},
	{{"saloon"}, {previous}},
	{{"hut"}, {"I hope you like it."}},
	{{"name"}, {"Just call me Frodo."}},
	{{"time"}, {"It is exactly %T."}},
	{{"king"}, {"Oh, our beloved king! Thanks to him, alcohol is so cheap."}},
	{{"tibianus"}, {previous}},
	{{"quentin"}, {"He hardly visits my humble tavern."}},
	{{"lynda", FEMALE}, {"A very noble lady."}},
	{{"lynda", MALE}, {"Just between you and me: What a babe!"}},
	{{"harkath"}, {"Too disciplined to enjoy life."}},
	{{"army"}, {"Hehe. Great customers."}},
	{{"ferumbras"}, {"Uhm, do not mention him. It may scare customers away."}},
	{{"general"}, {"Harkath Bloodblade is the royal general."}},
	{{"sam"}, {"A loud neighbour, I get a lot of complaints about him."}},
	{{"xodet"}, {"I don't know where he gets these fluids. If I could sell them here, the hut would be crowded."}},
	{{"gorn"}, {"Many of his customers visit my Hut, too."}},
	{{"elane"}, {"Can you believe that she actually told her guildfellows that alcohol is a bad thing?"}},
	{{"muriel"}, {"Muriel has never visited this place."}},
	{{"lungelen"}, {"A sorceress, you can find her in their guild sitting befor a book - always!"}},
	{{"gregor"}, {"The knights have sometimes parties here after some arena fights."}},
	{{"marvik"}, {"Marvik seldom leaves his guildhall at all."}},
	{{"bozo"}, {"I am trying to hire him for an evening or two."}},
	{{"baxter"}, {"He's able to drink a bottle or two."}},
	{{"oswald"}, {"I hate him. Each of his visits here ends with a bar brawl."}},
	{{"sherry"}, {"The McRonalds are a nice couple. Donald is a dear friend of mine."}},
	{{"mcronald"}, {previous}},
	{{"donald"}, {"He is a little shy. In his youth he dreamed to become a druid."}},
	{{"lugri"}, {"I overheared some conversations about his evilness. That's enough to hope, that I never ever meet him."}},
	{{"excalibug"}, {"Nothing more than a tale for warriors."}},
	{{"thais"}, {"Here in Thais is the center of Tibia."}},
	{{"tibia"}, {"Come on! You know that our world is called Tibia."}},
	{{"carlin"}, {"Many travellers tell funny stories about all the emancipated women in this northern town."}},
	{{"rain", "castle"}, {"The king's residence has been renovated lately."}},
	{{"galuna"}, {"She makes excellent arrows and bows."}},
	{{"hugo"}, {"I think some time ago a stranger from Fibula with that name stayed here for some nights."}},
	{{"todd"}, {"That fellow is filthy rich. He rented a room upstairs for months in advance and always orders the best beer and wine i serve."}},

	{{"news"}, {"Some travelers from Edron told about a great treasure guarded by cruel demons in the dungeons there."}},
	{{"rumors"}, {previous}},

	{{"buy"}, {"I can offer you bread, cheese, ham, or meat."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"Are you looking for food? I have bread, cheese, ham, and meat."}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(4), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(6), "Do you want to buy cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A ham for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},

	{{"satanic"}, {"Hmm, I have heard of a 'satanic influence' theory by someone called Newton or something like that... Maybe there's more in the Royal Archives."}},
	{{"cropwell"}, {"No idea who that is, but maybe you'll find something in the Royal Archives..."}},
	{{"alistair"}, {previous}},
	{{"archives"}, {"Oh, the Royal Archives are in Rain Castle!"}},
	{{"dungeon"}, {"Ah yes, the graveyard dungeon. All I know is this riddle: His Grave to the south, the tree above, his soul in the shade. No idea what that means, though!"}},
	{{"graveyard"}, {previous}},
	{{"riddle"}, {"I heard it when I was a child."}},
	{{"sunset", "homes"}, {"The sunset homes are a block of flats south of the harbour."}},
	{{"one", "eyed", "stranger"}, {"Yes, I remember him. His name was Berfasmur."}},
	{{"berfasmur"}, {"Sorry, he spoke only very little. I know nothing more about him."}},

	{{"hengis", "wulfson"}, {"He is a great bard. He often graced my hut with his presence, songs, and rhymes. I wonder what happened to him lately.", setTopic(2)}},
	{{eq(TOPIC, 2), "died"}, {"Oh, by the gods! What do you say happened to him?", setTopic(3)}},
	{{eq(TOPIC, 2), "killed"}, {previous}},
	{{eq(TOPIC, 2), "dead"}, {previous}},
	{{eq(TOPIC, 2), "death"}, {previous}},
	{{eq(TOPIC, 2), "slain"}, {previous}},
	{{eq(TOPIC, 3), "killed", "cyclops"}, {"That's horrible! I am in grief. I will never hear his songs again. I will even miss that strange rhyme he was obsessed with.", setTopic(4)}},
	{{eq(TOPIC, 3), "slain", "cyclops"}, {previous}},
	{{eq(TOPIC, 3), "cyclops", "slay"}, {previous}},
	{{eq(TOPIC, 3), "cyclops", "kill"}, {previous}},
	{{eq(TOPIC, 4), "rhyme"}, {"He recitated it that often that I learned it by heart myself. I would recitate it, but I am not skilled in that kind of things.", setTopic(5)}},
	{{eq(TOPIC, 5), "recitate"}, {"Uhm. If you insist, but I am so awful. I will stop now and then and wait, so you can tell if I should proceed, ok?", setTopic(6)}},
	{{eq(TOPIC, 6), "yes"}, {"Well ok, but don't blame me. Chhrrr... chhrrrr,... it goes like this... chhrrr: and when the dead feast at midnight...", setTopic(7)}},
	{{eq(TOPIC, 7), "proceed"}, {"... the ancient enemy will no longer guard the place of his unlucky heir and the living will walk the paths of the old way...", setTopic(8)}},
	{{eq(TOPIC, 8), "proceed"}, {"... Death awaits the greedy and the brave alike and many will be mourned until the long lost treasure is unearthed.", setTopic(9)}},
	{{eq(TOPIC, 9), "proceed"}, {"That's all. He recitated it when he was in one of his melancholy moods."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
