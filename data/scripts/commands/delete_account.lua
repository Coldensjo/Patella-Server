-- commands/delete_account.lua: Delete Account ("omana gran liber para para"), what
-- AccountRightSpell's case SPELL_DELETE_ACCOUNT did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
return {
	on_say = function(caster, number, words)
		command.delete_account(caster, words[4], words[5])
	end,
}
