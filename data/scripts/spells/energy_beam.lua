-- spells/energy_beam.lua: Energy Beam ("exevo vis lux"), what CastSpell's case
-- SPELL_ENERGY_BEAM did: energy damage of 60 plus or minus 20 in a beam of
-- range 5 in front of the caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		local amount = magic.compute_damage(caster, spell, 60, 20)
		magic.angle_combat(caster, mana, soul_points, amount, effect.EXPLOSION, 5, 0, damage.ENERGY)
	end,
}
