-- commands/start_monsterraid.lua: Start Monsterraid ("alevo mas res para"),
-- what StartMonsterraid (the original magic.cc) did: with the command's right,
-- starts the raid the word names (in lower case as strLower makes it:
-- lib/commands.lua's lower) now (Game.startRaid), with the blue sparkles on
-- the caster, or the puff when there is no such raid.

local commands = require("lib.commands")

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	if not Game.startRaid(commands.lower(words[4])) then
		caster:sendMagicEffect(CONST_ME_POFF)
		return
	end

	caster:sendMagicEffect(CONST_ME_MAGIC_BLUE)
end
