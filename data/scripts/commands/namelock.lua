-- commands/namelock.lua: Namelock ("omana hora para"), what
-- AccountRightSpell's case SPELL_NAMELOCK did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
return {
	on_say = function(caster, number, words)
		command.name_lock(caster, words[3])
	end,
}
