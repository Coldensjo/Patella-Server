-- commands/entry_in_criminal_record.lua: Entry in Criminal Record ("omisa para para"), what
-- AccountRightSpell's case SPELL_ENTRY_IN_CRIMINAL_RECORD did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
return {
	on_say = function(caster, number, words)
		command.notation(caster, words[2], words[3])
	end,
}
