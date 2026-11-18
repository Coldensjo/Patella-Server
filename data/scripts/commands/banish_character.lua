-- commands/banish_character.lua: Banish Character ("omana res para para
-- para"), what BanishCharacter (the original magic.cc) did: tells the caster
-- the command is gone and to use the dialog (Ctrl+Y). It checks no right and
-- reads no word.

function onSay(caster, command, words)
	caster:sendTextMessage(MESSAGE_STATUS_SMALL, "Spell is not available any more. Press Ctrl+Y for the dialog.")
end
