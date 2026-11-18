-- commands/set_namerule.lua: Set Namerule ("omamo para"), what SetNameRule
-- (the original magic.cc) did: tells the caster the command is gone. It checks
-- no right and reads no word.

function onSay(caster, command, words)
	caster:sendTextMessage(MESSAGE_STATUS_SMALL, "Spell is not available any more.")
end
