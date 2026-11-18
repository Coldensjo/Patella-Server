-- commands/summon_wild_creature.lua: Summon Wild Creature ("alevo res para"),
-- what SummonCreature(cr, 0, Name, true) (the original magic.cc) did: looks up
-- the race the word names (lib/commands.lua's findRace; none:
-- CREATUREDOESNOTEXIST, before the right is checked), then, with the command's
-- right, creates a monster of it, no one's summon and for no mana, on a summon
-- field within 2 of the caster (Game.searchSummonField, its random draws;
-- none: NOROOM), and shows the blue sparkles on the caster.

local commands = require("lib.commands")

function onSay(caster, command, words)
	local race = commands.findRace(words[3])
	if race == 0 then
		Game.throwResult(RETURNVALUE_CREATUREDOESNOTEXIST)
	end

	if not caster:hasCommandRight(command) then
		return
	end

	local field = Game.searchSummonField(caster:getPosition(), 2)
	if field == nil then
		Game.throwResult(RETURNVALUE_NOTENOUGHROOM)
	end

	Game.createMonsterOfRace(race, field)
	caster:sendMagicEffect(CONST_ME_MAGIC_BLUE)
end
