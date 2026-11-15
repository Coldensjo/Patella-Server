-- commands/delete_character.lua: Delete Character ("omana gran res para para"), what
-- AccountRightSpell's case SPELL_DELETE_CHARACTER did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
function onSay(caster, command, words)
	Command.deleteCharacter(caster, words[4], words[5])
end
