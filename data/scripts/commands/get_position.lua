-- commands/get_position.lua: Get Position ("aliva tera"), what
-- CharacterRightSpell's case SPELL_GET_POSITION did: tells the caster its coordinate.
function onSay(caster, command, words)
	Command.showPosition(caster)
end
