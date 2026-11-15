-- spells/ultimate_explosion.lua: Ultimate Explosion ("exevo gran mas vis"),
-- what CastSpell's case SPELL_ULTIMATE_EXPLOSION did: physical damage of 250
-- plus or minus 50 in radius 6 around the caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		local amount = magic.compute_damage(caster, spell, 250, 50)
		magic.mass_combat(caster, creature.object(caster), mana, soul_points, amount, effect.EXPLOSION_AREA, 6, damage.PHYSICAL, animation.FIRE)
	end,
}
