-- commands/move.lua: Move ("alani para"), what
-- CharacterRightSpell's case SPELL_MOVE did: moves the caster up or down, changes its speed, or sends it to a coordinate or mark.
return {
	on_say = function(caster, number, words)
		command.teleport(caster, words[2])
	end,
}
