-- spells/energy_wave.lua: Energy Wave ("exevo mort hur"), what CastSpell's case
-- SPELL_ENERGY_WAVE did: energy damage of 150 plus or minus 50 in a wave of
-- range 5 and angle 30 in front of the caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		local amount = magic.compute_damage(caster, spell, 150, 50)
		magic.angle_combat(caster, mana, soul_points, amount, effect.TELEPORT, 5, 30, damage.ENERGY)
	end,
}
