-- commands/get_position.lua: Get Position ("aliva tera"), what
-- CharacterRightSpell's case SPELL_GET_POSITION did: tells the caster its coordinate.
return {
	on_say = function(caster, number, words)
		command.get_position(caster)
	end,
}
