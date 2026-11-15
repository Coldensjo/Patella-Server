-- commands/banish_ip_address.lua: Banish IP Address ("omana vis para para"), what
-- AccountRightSpell's case SPELL_BANISH_IP_ADDRESS did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
return {
	on_say = function(caster, number, words)
		command.ip_banishment(caster, words[3], words[4])
	end,
}
