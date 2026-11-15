-- commands/edit_door.lua: Edit Door ("aleta grav"), what
-- CharacterRightSpell's case SPELL_EDIT_DOOR did: opens the list of the name door below or in front of the caster.
return {
	on_say = function(caster, number, words)
		command.edit_name_door(caster)
	end,
}
