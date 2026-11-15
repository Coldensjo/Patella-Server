-- commands/banish_character.lua: Banish Character ("omana res para para para"), what
-- AccountRightSpell's case SPELL_BANISH_CHARACTER did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
return {
	on_say = function(caster, number, words)
		command.banish_character(caster, words[3], game.to_int(words[4]), words[5])
	end,
}
