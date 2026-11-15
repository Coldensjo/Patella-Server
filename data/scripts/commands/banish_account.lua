-- commands/banish_account.lua: Banish Account ("omana liber para para para"), what
-- AccountRightSpell's case SPELL_BANISH_ACCOUNT did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
return {
	on_say = function(caster, number, words)
		command.banish_account(caster, words[3], game.to_int(words[4]), words[5])
	end,
}
