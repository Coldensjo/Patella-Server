-- commands/change_data.lua: Change Data ("aleta para"), what
-- CharacterRightSpell's case SPELL_CHANGE_DATA did: sets the number (liquid, key, keyhole, amount, charges) of the object in hand or the key door below.
return {
	on_say = function(caster, number, words)
		command.change_data(caster, words[2])
	end,
}
