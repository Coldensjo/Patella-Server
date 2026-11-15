-- commands/create_gold.lua: Create Gold ("alevo vis para"), what
-- CharacterRightSpell's case SPELL_CREATE_GOLD did: creates the amount of money the word says.
return {
	on_say = function(caster, number, words)
		command.create_money(caster, words[3])
	end,
}
