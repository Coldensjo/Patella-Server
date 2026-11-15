-- runes/explosion.lua: Explosion ("adevo mas hur"), what UseMagicItem's case
-- SPELL_EXPLOSION did: physical damage in radius 1 around the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		local amount = magic.compute_damage(user, spell, 60, 40)
		magic.mass_combat(user, target, 0, 0, amount, effect.EXPLOSION_AREA, 1, damage.PHYSICAL, animation.FIRE)
	end,
}
