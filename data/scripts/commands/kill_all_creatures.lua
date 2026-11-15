-- commands/kill_all_creatures.lua: Kill All Creatures ("alito mas res"), what
-- CharacterRightSpell's case SPELL_KILL_ALL_CREATURES did: kills the monsters within 2 fields of the caster, with the death effect.
return {
	on_say = function(caster, number, words)
		magic.kill_all_monsters(caster, effect.DEATH, 2)
	end,
}
