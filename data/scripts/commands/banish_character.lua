-- commands/banish_character.lua: Banish Character ("omana res para para para"), what
-- AccountRightSpell's case SPELL_BANISH_CHARACTER did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
function onSay(caster, command, words)
	Command.banishCharacter(caster, words[3], Game.toInt(words[4]), words[5])
end
