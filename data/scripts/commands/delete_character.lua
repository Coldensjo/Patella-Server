-- commands/delete_character.lua: Delete Character ("omana gran res para para"), what
-- AccountRightSpell's case SPELL_DELETE_CHARACTER did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
return {
	on_say = function(caster, number, words)
		command.delete_character(caster, words[4], words[5])
	end,
}
