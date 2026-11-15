-- spells/fire_wave.lua: Fire Wave ("exevo flam hur"), what CastSpell's case
-- SPELL_FIRE_WAVE did: fire damage of 30 plus or minus 10 in a wave of range 4
-- and angle 45 in front of the caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		local amount = magic.compute_damage(caster, spell, 30, 10)
		magic.angle_combat(caster, mana, soul_points, amount, effect.FIRE_AREA, 4, 45, damage.FIRE)
	end,
}
