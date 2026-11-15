-- commands/move.lua: Move ("alani para"), what
-- CharacterRightSpell's case SPELL_MOVE did: moves the caster up or down, changes its speed, or sends it to a coordinate or mark.
function onSay(caster, command, words)
	Command.teleport(caster, words[2])
end
