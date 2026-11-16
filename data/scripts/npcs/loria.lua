-- npcs/loria.lua: the behaviour of the NPC Loria, converted from
-- loria.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of loria.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- loria.npc: Datenbank für die Zauberkundige Loria

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local KNIGHT = behaviour.KNIGHT
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local TOPIC = behaviour.TOPIC
local setTopic = behaviour.setTopic
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello", "loria", FORCE}, {"Welcome %N, my friend."}},
	{{ADDRESS, "hi", "loria", FORCE}, {previous}},
	{{ADDRESS, "hello", FORCE}, {"Welcome %N."}},
	{{ADDRESS, "hi", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Hope to see you again."}},

	{{"bye"}, {"May the magic be with you, %N.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am studying the power of magic all the time."}},
	{{"lake"}, {"I hope you like it. It is named like my master, Alatar, the Sage"}},
	{{"name"}, {"I am Loria, a former apprentice of Alatar, the Sage."}},
	{{"alatar"}, {"Well, he was my great master. He taught me all these fantastic things about magic. I really miss him."}},
	{{"time"}, {"Time means nothing to me."}},
	{{"buy"}, {"I don't care for money, I care for magic."}},
	{{"power"}, {"Although your attack spells get stronger with your usage of magic, real power is gained by finding strategies to properly use your magic abilities."}},
	{{"mana"}, {"Mana is the source of all magic. If you use spells, it will drain mana from your energy pool. This mana regenerates slowly, if you eat, or if you drink those mana fluids you can buy at Xodet's."}},
	{{"gorn"}, {"He runs an equipment shop close to the north gate of the city."}},
	{{"xodet"}, {"He runs a magic shop in the main road."}},
	{{"praise", "alatar"}, {"I praise my master Alatar."}},

	{{"kill"}, {"Killing and destruction are just foolish steaps to entrophy."}},

	{{"quest"}, {"I heard from a mystic bone of the lich lord below the House of Necromant. Bring it to me, and you will receive a reward."}},
	{{"crystal"}, {"The mystic crystal should be able to resurrect fresh corpses."}},
	{{"necromant"}, {"He lived in a lonely house in the south eastern part of Tibia beyond the mountains."}},
	{{"reward"}, {"I'll teach you a very seldom spell.", setTopic(4)}},
	{{eq(TOPIC, 4), "spell"}, {"I'll teach you 'exevo gran mas vis', but bring me this bone first!"}},

	{{"magic", KNIGHT}, {"I could tell you much about all sorcerer spells, but you won't understand it. Anyway, feel free to ask me."}},
	{{"magic"}, {"Oh, I can tell you a lot about all sorcerer spells. Feel free to ask me."}},
	{{"spell"}, {"Oh, I can tell you a lot about all sorcerer spells. Feel free to ask me."}},
	{{"rune"}, {"All spells starting with the syllable 'ad' must be burned into a rune. For this buy a rune from Xodet and put it in one of your hands. Now cast the formula of the spell."}},
	{{"Muriel"}, {"He runs his magic shop in the southwest of the city. He sells runes and spells and helps you, if you want to become a sorcerer."}},
	{{"find", "person"}, {"If you search someone, this spell will give you an idea of the direction you must head. You will be able to see, whether he is below or above you."}},
	{{"light"}, {"A ray of light will emerge from your flat hand to illuminate your environment."}},
	{{"light", "healing"}, {"The paths to the next temple are long. Even in Tibia. So learn this spell, and be able to heal yourself during your travels. This spell will only cure small wounds, but it is pretty helpful."}},
	{{"light", "magic", "missile"}, {"You can activate this spell by pointing your index finger in the direction of your enemy, conjure the power of your rune and shoot the magic missiles in your enemy's body."}},
	{{"antidote"}, {"This spell sucks the venom out of your veins, that some enemy might have injected."}},
	{{"intense", "healing"}, {"This spell will cure more wounds or greater ones at once. This is of course more 'mana intensive', but everybody will sooner or later get in a situation where mana is nothing - compared to life."}},
	{{"poison", "field"}, {"This spell will create a single field of poisonous gas. Cast it on a creature you were not able to arrange a peace treaty with. If it has no antidote, watch what could happen if you forget yours."}},
	{{"great", "light"}, {"This spell will illuminate your whole screen and last longer than 'light'. Use it in deep dungeons, 'cause behind every corner there could be your last enemy ... and you might just walk into him."}},
	{{"fire", "field"}, {"This spell acts similar to the 'poison field' spell, except that you create fire instead of poisonous gas. Don't enter it yourself or you will realize why it is said that 'fire eats everything'."}},
	{{"heavy", "magic", "missile"}, {"Remember the spell where you only got to wave your hand? Well, wave it twice and shoot a heavy magic missile at your enemy. This spell will create a rune with five charges."}},
	{{"magic", "shield"}, {"Well, mages are more bookworms than sportsmen, and as such often neglect their physical fitness. In ancient tomes lies the power to use mana as an equivalent to life. So use this spell to survive."}},
	{{"fireball"}, {"A perfect symbiosis of fire and wind. More is not to be said about this spell. Use this fireball as a warning or as your defence, but don't burn your fingers."}},
	{{"energy", "field"}, {"This one will create a field of energy. Everyone stepping in will be struck from lightning. This field will not last as long as poison or fire fields, but it is more deadly."}},
	{{"destroy", "field"}, {"Trapped again between fire, poison and energy fields? This spell will give you the ability to destruct the fields, so you can pass safely."}},
	{{"fire", "wave"}, {"Turn to you opponent and release the forces of nature with a whisper of your voice. A triangle of fire will burn all persons in your view, so take care, in which direction you look!"}},
	{{"ultimate", "healing"}, {"This spell is able to cure almost every injury at a higher cost than the other healing spells."}},
	{{"great", "fireball"}, {"Imagine scaling the normal fireball by two and raising the fire temperature."}},
	{{"firebomb"}, {"With a snip of your finger you can cover the floor with a burning carpet that keeps on burning for a while."}},
	{{"fire", "bomb"}, {"With a snip of your finger you can cover the floor with a burning carpet that keeps on burning for a while."}},
	{{"energybeam"}, {"A ray of energy will strike everyone in your current direction"}},
	{{"creature", "illusion"}, {"A good one to scare childs. You can change your appearance to any monster. You can be as handsome as a ghoul!"}},
	{{"poison", "wall"}, {"With this one you can create a huge wall of poisonous gas. Many monsters will be too scared to pass the wall and if they do, they will choke from nausea."}},
	{{"explosion"}, {"A strong blast of fire wounds the opponent you point at, and the adjacent squares."}},
	{{"fire", "wall"}, {"As the poison wall, this spell creates an even larger wall of fire, burning everyone who passes."}},
	{{"great", "energy", "beam"}, {"A lightning bolt strikes the point you look at."}},
	{{"invisible"}, {"This spell drains the colors out of you body, making yourself invisible for an hour or two."}},
	{{"summon", "creature"}, {"This one gives you the ability to summon monsters that aid you in your battles."}},
	{{"energy", "wall"}, {"Attracts lightning bolts from the sky, to form a giant wall, seriously damaging everyone who passes."}},
	{{"energy", "wave"}, {"Shoots a triangular bundle of lightning bolts in the direction you look."}},
	{{"sudden", "death"}, {"The best spell for deciding a battle within seconds. The spell tries to interrupt the opponents heart beat, leading to his instant death in most cases."}},

	{{"formula"}, {"Which is the spell, you need the formula to?", setTopic(1)}},
	{{eq(TOPIC, 1), "find", "person"}, {"Say the words: exiva 'name'"}},
	{{eq(TOPIC, 1), "light"}, {"Say the words: utevo lux"}},
	{{eq(TOPIC, 1), "light", "healing"}, {"Say the word: exura"}},
	{{eq(TOPIC, 1), "light", "magic", "missile"}, {"Say the word: adori"}},
	{{eq(TOPIC, 1), "antidote"}, {"Say the words: exana pox"}},
	{{eq(TOPIC, 1), "intense", "healing"}, {"Say the words: exura gran"}},
	{{eq(TOPIC, 1), "poison", "field"}, {"Say the words: adevo grav pox"}},
	{{eq(TOPIC, 1), "great", "light"}, {"Say the words: utevo gran lux"}},
	{{eq(TOPIC, 1), "fire", "field"}, {"Say the words: adevo grav flam"}},
	{{eq(TOPIC, 1), "heavy", "magic", "missile"}, {"Say the words: adori gran"}},
	{{eq(TOPIC, 1), "magic", "shield"}, {"Say the words: utamo vita"}},
	{{eq(TOPIC, 1), "fireball"}, {"Say the words: adori flam"}},
	{{eq(TOPIC, 1), "energy", "field"}, {"Say the words: adevo grav vis"}},
	{{eq(TOPIC, 1), "destroy", "field"}, {"Say the words: adito grav"}},
	{{eq(TOPIC, 1), "fire", "wave"}, {"Say the words: exevo flam hur"}},
	{{eq(TOPIC, 1), "ultimate", "healing"}, {"Say the words: exura vita"}},
	{{eq(TOPIC, 1), "great", "fireball"}, {"Say the words: adori gran flam"}},
	{{eq(TOPIC, 1), "fire", "bomb"}, {"Say the words: adevo mas flam"}},
	{{eq(TOPIC, 1), "firebomb"}, {"Say the words: adevo mas flam"}},
	{{eq(TOPIC, 1), "energy", "beam"}, {"Say the words: exevo vis lux"}},
	{{eq(TOPIC, 1), "creature", "illusion"}, {"Say the words: utevo res ina 'creature'"}},
	{{eq(TOPIC, 1), "poison", "wall"}, {"Say the words: adevo mas grav pox"}},
	{{eq(TOPIC, 1), "explosion"}, {"Say the words: adevo mas hur"}},
	{{eq(TOPIC, 1), "fire", "wall"}, {"Say the words: adevo mas grav flam"}},
	{{eq(TOPIC, 1), "great", "energy", "beam"}, {"Say the words: exevo gran vis lux"}},
	{{eq(TOPIC, 1), "invisible"}, {"Say the words: utana vid"}},
	{{eq(TOPIC, 1), "summon", "creature"}, {"Say the words: utevo res 'creature'"}},
	{{eq(TOPIC, 1), "energy", "wall"}, {"Say the words: adevo mas grav vis"}},
	{{eq(TOPIC, 1), "energy", "wave"}, {"Say the words: exevo mort hur"}},
	{{eq(TOPIC, 1), "sudden", "death"}, {"Say the words: adori vita vis"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
