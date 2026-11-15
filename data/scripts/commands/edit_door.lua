-- commands/edit_door.lua: Edit Door ("aleta grav"), what
-- CharacterRightSpell's case SPELL_EDIT_DOOR did: opens the list of the name door below or in front of the caster.
function onSay(caster, command, words)
	Command.editNameDoor(caster)
end
