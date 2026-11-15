-- commands/set_namerule.lua: Set Namerule ("omamo para"), what
-- AccountRightSpell's case SPELL_SET_NAMERULE did: tells the caster the command is gone.
return {
	on_say = function(caster, number, words)
		command.set_name_rule(caster, words[2])
	end,
}
