-- npcs/daniel.lua: the behaviour of the NPC Daniel Steelsoul, converted from
-- daniel.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of daniel.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- daniel.npc: Datenbank für Gouverneur Daniel Steelsoul

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local TOPIC = behaviour.TOPIC
local setTopic = behaviour.setTopic
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings and Banor with you, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hail$", FORCE}, {previous}},
	{{ADDRESS, "salutations$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Shut up! I am talking already!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hail$", FORCE}, {previous}},
	{{BUSY, "salutations$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"PRAISE TO BANOR!"}},

	{{"bye"}, {"PRAISE TO BANOR!", idle}},
	{{"news"}, {"Nothing new from the north."}},
	{{"king"}, {"LONG LIVE THE KING!"}},
	{{"leader"}, {"King Tibianus III is our wise and just leader."}},
	{{"name"}, {"I am Sir Daniel Steelsoul of the Sacred Order of Banor's Blood."}},
	{{"job"}, {"I am the governor of this isle, Edron, and grandmaster of the Knights of Banor's Blood."}},
	{{"how", "are", "you"}, {"I did not have much sleep lately, but I am fine."}},
	{{"sell"}, {"Are you suggesting I am corruptible?", setTopic(2)}},
	{{"army"}, {"The army protects the Thaian realm. The order of the Knights of Banor's Blood supports them with all our skills."}},
	{{"guard"}, {previous}},
	{{"general"}, {"Harkath Bloodblade declined the governorship because he's needed more in Thais."}},
	{{"enemies"}, {"Evil challenges the forces of good in any shape. Be it the claws of vicious monsters or the seductive dark secrets of rotten power."}},
	{{"enemy"}, {previous}},
	{{"banors", "blood"}, {"We believe that the blood of Banor runs through the veins of all humans. Therefore, we are responsible to live up to Banors standards and not to stain his legacy with sinful acts."}},
	{{"castle"}, {"The castle was built on elder foundations we found on this isle."}},

	{{"edron"}, {"This isle is rumoured to have been the home of a powerful ancient race which became extinct before the corpsewars. It was up to King Tibianus III to reclaim it for humanity and to found this colony."}},
	{{"colony"}, {"With the Thaian army bound to other duties, our order was entrusted to secure the area. We defeated the evil minotaurs living right here and cleansed the isle of their unholy presence."}},
	{{"minotaur"}, {"The minotaurs, though evil, were worthy opponents. After the treason of the man who is now known as Kaine Kinslayer, we lack the manpower to crush their cyclopean allies, too."}},
	{{"cyclop"}, {"They live in an underground city, known as cyclopolis in the north of the isle. Constantly forging weapons for the servants of darkness."}},
	{{"allies"}, {previous}},
	{{"kaine"}, {"He was my second in command. After learning about the forbidden ruins, he, the priestess Agaltha, and their followers freed the criminals we brought here as workers and headed to the north."}},
	{{"ruins"}, {"An ancient taboo forbids to enter the northern ruins."}},
	{{"taboo"}, {"We thought it was only superstition and no one bothered that Kaine and his friends went there to hunt servants of evil that might have hidden there. So we did not notice the dark cult they started."}},
	{{"cult"}, {"We know only little about them. Kaine and his fallen knights were joined by criminal scum and remaining forces of darkness that escaped us. They were joined by some ominous dark monks."}},
	{{"monk"}, {"We don't know if they came here or already hid in the ruins as we arrived. Maybe they seduced Kaine or Agaltha, maybe they were theirs for years."}},
	{{"agaltha"}, {"She was beautiful but seemed cold hearted. She spoke little to me, prefered the company of Kaine."}},
	{{"eremo"}, {"Eremo is a very wise man. I visit him sometimes on his little island near Edron. Just ask a fisherman for a passage."}},
	{{"fisherman"}, {"Pemaret is a fisherman on Cormaya."}},
	{{"cormaya"}, {"It is a peaceful isle next to Edron with a nice village. There, you should visit the wonderful garden."}},
	{{"falk"}, {"A promising young fellow."}},
	{{"horn", "plenty"}, {"I hardly find the time to visit the tavern."}},
	{{"mirabell"}, {previous}},
	{{"willard"}, {"When he was young, Willard served in the royal army."}},
	{{"weapon"}, {"Look for Willard, our local blacksmith."}},
	{{"armor"}, {previous}},
	{{"academy"}, {"After the treason of Kaine, we observe these mages closely. If even a knight falls prey to the seduction of the forbidden ruins, no one can tell how easy some of these mystics might betray his people."}},
	{{"amanda"}, {"I think this nun might be a bit young for this position, but I won't question the decisions of the church of Banor's Blood."}},
	{{"benjamin"}, {"He and his men fought against Ferumbras somewhere in the north of this isle, long before there were even plans of a colony. Only old Ben returned alive from the battle, but his mind was broken."}},
	{{"ferumbras"}, {"He searched something in the north of the isle years ago. Probably he needed something from the forbidden ruins. He was chased and fought by the troops of General Benjamin."}},
	{{"join"}, {"You may join the order of Banor's blood if you prove your honor."}},
	{{"honor"}, {"Only those who live a life of bravery, honor, and piety may join our sacred order."}},
	{{"piety"}, {previous}},
	{{"bravery"}, {previous}},
	{{"quest"}, {"A life in bravery, honor, and piety should be every man's most important quest."}},
	{{"mission"}, {previous}},
	{{"god"}, {"I worship Banor, the first champion of good!"}},
	{{"banor"}, {"His spirit and blood are within us. Honor this fact or be cast into hell."}},
	{{"zathroth"}, {"Do not mention the name of the cursed one!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"brog"}, {"The rotten cyclopses whorship the raging giant of hell."}},
	{{"monster"}, {"We cleansed the south of any major enemy, but watch out while travelling the north."}},
	{{"excalibug"}, {"With this weapon in my hand, I would teach the servants of darkness the true meaning of the word fear."}},
	{{"kazordoon"}, {"Now and then a dwarf comes to this isle. Most behave secretive about their reason to come here. As far as I can tell they are looking for some dwarfish artifact which was lost in ancient times."}},
	{{"dwarf"}, {previous}},
	{{"carlin"}, {"I belong to a sacred order and don't bother about mundane politics."}},

	{{"fuck"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"idiot"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"asshole"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"ass$"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"fag$"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"stupid"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"tyrant"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"shit"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"lunatic"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},

	{{eq(TOPIC, 2), "yes"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{eq(TOPIC, 2)}, {"Then be more careful with your words!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
