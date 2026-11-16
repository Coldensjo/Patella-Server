-- npcs/boques.lua: the behaviour of the NPC Bo'ques, converted from
-- boques.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of boques.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- boques.npc: Datenbank für den Djinnkoch Bo'ques

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local TOPIC = behaviour.TOPIC
local count = behaviour.count
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local delete = behaviour.delete
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", eq(questValue(278), 2), FORCE}, {"Hey! A human! What are you doing in my kitchen, %N?"}},
	{{ADDRESS, "hi$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, "greetings$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, "djanni'hah$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", eq(questValue(278), 2), FORCE}, {"Whoa. Do I look as if I had two heads? Only one at a time, %N!", queue}},
	{{BUSY, "hi$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, "greetings$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, "djanni'hah$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Now, where was I?"}},

	{{"bye"}, {"Goodbye. I am sure you will come back for more. They all do.", idle}},
	{{"farewell"}, {previous}},

	{{"name"}, {"My name is Bo'ques. Perhaps you know my name from a restaurant guide."}},
	{{"bo'ques"}, {"You want Bo'ques? Well, you have found him, I'd say."}},
	{{"job"}, {"I'm preparing the food for all djinn in Ashta'daramai. ...", "Therefore I'm what is commonly called a cook, although I do not like that word too much. It is vulgar. I prefer to call myself 'chef'."}},
	{{"cook"}, {previous}},
	{{"chef"}, {"Chef sounds nice, doesn't it? Well... I must admit I do not really know what it means, but it certainly sounds classy."}},
	{{"food"}, {"I know many recipes for preparing the finest food on Darama and maybe even whole Tibia!"}},

	{{"king"}, {"Gabel used to be king, you know. I must confess I miss those days a bit because I was allowed to carry the title of his royal majesty's personal cook. Ah, those were the days."}},
	{{"gabel"}, {"He is my boss. A most loyal customer and a real con... conni... well, a man of taste, at any rate. His favourite dish is Scarab\233e au Vin served with onions and rice."}},
	{{"connoisseur"}, {"Yes! That's it! I have always trouble with pronouncing that damn word. A conno... conni... ah, hang it all!"}},
	{{"djinn"}, {"That is our race. It has seen better days, you know. ...", "It would have been better for us all if more djinn would share my interest in cooking. But no! Bashing each others' heads in is the only thing they are good at! Vandals and trogo ...trogli ... and cavemen, that's what they are!"}},
	{{"efreet"}, {"A bunch of ignorants and primitives, that's what they are. You should see the things they eat! ...", "You know they serve ketchup with just about every kind of meal! Ketchup! Oh, those barbarians."}},
	{{"marid"}, {"That is us - the loyalists who have have remained faithful to Gabel and to good cooking."}},
	{{"malor"}, {"That accursed traitor! I think there will never be peace until he is completely vanquished. If only he would allow me to cook for him. I would fix him a dinner he would never forget."}},
	{{"mal'ouquah"}, {"Ah yes. The efreets' notorious fortress. I have never been there. That is no place for an artist such as myself."}},
	{{"ashta'daramai"}, {"That is our little fortress - our home. Nice, isn't it? I find it inspirational, although I find the culinary facilities could do with some improvements."}},
	{{"human"}, {"I totally agree with Gabel that djinn and humans can learn from each other. ...", "Take cooking, for example. It has such a long tradition among humans - even I could still learn a thing or two from the famous cooks at king Tibianus' court!"}},
	{{"zathroth"}, {"That is a sad story, and like most djinn I dislike talking about it. Let's put it this way. Once there was a great cook who worked hard to prepare the finest meal of his life. ...", "But when he found that the product of his efforts did not meet his expectations he just ditched it even though it was wonderfully unique in its own special way. ...", "You know what I think? I think Zathroth was a bad cook."}},
	{{"tibia"}, {"It may be that this world is wide and full of adventure, but to be honest I am not at all keen to see it myself. A comfortable lamp to sleep in and a well equipped kitchen is all I need."}},
	{{"daraman"}, {"Ah yes. That human WAS special, believe me. Did you know I talked to him myself, back in those days? In fact, I even had an argument with him because he dared to insult my work! He drove me mad when he called me a self-indulgent glutton. ...", "But you know, eventually we came to respect each other! He taught me to stress quality rather than quantity, and he came to appreciate my 'Chili con Cobra'. Today I know that having met him was a major step forward in my development as a culinary artist."}},
	{{"darashia"}, {"I have heard good things about this place. I understand the Caliph is a true gourmet. People who eat good food can't be bad, that's what I say."}},
	{{"scarab"}, {"Ah yes. I like them well. Especially with a good sauce or in a stew. But they have to be young! Have you tried ancient scarab? Their meat is impossible to chew unless you have teeth made of titanium."}},
	{{"edron"}, {"Ah, the northern cities. One day I will start an extensive culinary expedition there. I have this dream of writing some sort of culinary guide, you know. Isn't that a great idea?"}},
	{{"thais"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {previous}},
	{{"carlin"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"ankrahmun"}, {"No djinn who is in his right state of mind would want to go there? What for? The land is ruled by an undead nut case, and from what I have heard his subjects are no better."}},
	{{"pharaoh"}, {"Apparently he is an undead! And what's worse, he actually chose that fate for himself! Undead! Imagine that! Never sleep, never laugh, and worst of all: Never eat! What a crackpot!"}},
	{{"palace"}, {"Who would like to live in a palace if there is never the delicious smell of freshly prepared food! I would not want to live there. Not for love nor for money."}},
	{{"ascension"}, {"As far as I know that is one of the pharaoh's crazy ideas. Just a load of baloney."}},
	{{"rah"}, {"Hm. Is that some exotic spice? Hang on, I know! It is a kind of lizard stew - right?"}},
	{{"uthun"}, {previous}},
	{{"akh"}, {previous}},

	{{"kha'zeel"}, {"These mountains are a nice place to live in, but food-wise they are pretty lousy. We basically import everything we eat from the lowlands, trading them for magic trinkets and for gold. ...", "The only plants that grow well in these mountains are potatoes, and they are not really my idea of Haute Cuisine."}},
	{{"kha'labal"}, {"Such a shame about that land. It wasn't always a desert, you know. That land was garden, a veritable paradise. Just the thought of the fruit that used to grow there makes my mouth water <sighs>. Well, guess who messed it up."}},
	{{"war"}, {"I have never been much of a warrior, but I will storm into battle swinging my meat cleaver if necessary. We simply must win this war!"}},
	{{"melchior"}, {"Ah yes, the trader - right? I remember him. He used to travel the mountains with his mule. A tough haggler and a real skinflint, he was. I thought he had fallen down a cliff with all his money."}},
	{{"alesar"}, {"Ah - that guy. You probably don't know it, but nobody around here likes to hear that name. It brings back painful memories, you know. His betrayal was such a heavy blow to us. I think I will never understand what made him do it? It is a mystery."}},
	{{"lamp"}, {"You would not believe it, but those lamps are actually quite comfy. And on top of that they are immensely practical! Did you ever try to stash one of your beds into your pocket?"}},
	{{"fa'hradin"}, {"That djinn is so engrossed in his work! I constantly have to remind him to eat because if I didn't he would simply forget. Forgetting to eat! Can you imagine that?"}},
	{{"djema"}, {"Djema is a nice girl, but she eats so little. It's frustrating, really. Humans and their little stomachs!"}},

	{{"recipe", eq(questValue(280), 0)}, {"My collection of recipes is almost complete. There are only but a few that are missing. ...", "Hmmm... now that we talk about it. There is something you could help me with. Are you interested?", setTopic(2)}},
	{{"mission", eq(questValue(280), 0)}, {previous}},
	{{eq(TOPIC, 2), "yes"}, {"Fine! Even though I know so many recipes, I'm looking for the description of some dwarven meals. ...", "So, if you could bring me a cookbook of the dwarven kitchen I will reward you well.", setQuestValue(280, 1)}},
	{{eq(TOPIC, 2)}, {"Well, too bad."}},

	{{"book", eq(questValue(280), 1)}, {"Do you have the cookbook of the dwarven kitchen with you? Can I have it?", setTopic(1)}},
	{{"cookbook", eq(questValue(280), 1)}, {previous}},
	{{"book", eq(questValue(280), 2)}, {"Thanks again, for bringing me that book!"}},
	{{"cookbook", eq(questValue(280), 2)}, {previous}},

	{{eq(TOPIC, 1), "yes", eq(count(3234), 1), FORCE}, {"The book! You have it! Let me see! <browses the book> ...", "Dragon Egg Omelette, Dwarven beer sauce... it's all there. This is great! Here is your well-deserved reward. ...", "Incidentally, I have talked to Fa'hradin about you during dinner. I think he might have some work for you. Why don't you talk to him about it?", setAmount(1), delete(3234), setAmount(3), create(3029), setQuestValue(280, 2)}},
	{{eq(TOPIC, 1)}, {"Too bad. I must have this book."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
