-- spells/great_energy_beam.lua: Great Energy Beam ("exevo gran vis lux"), what
-- CastSpell's case SPELL_GREAT_ENERGY_BEAM did: energy damage of 120 plus or
-- minus 80 in a beam of range 8 in front of the caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		local amount = magic.compute_damage(caster, spell, 120, 80)
		magic.angle_combat(caster, mana, soul_points, amount, effect.EXPLOSION, 8, 0, damage.ENERGY)
	end,
}
