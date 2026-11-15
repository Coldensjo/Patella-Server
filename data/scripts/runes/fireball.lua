-- runes/fireball.lua: Fireball ("adori flam"), what UseMagicItem's case
-- SPELL_FIREBALL did: fire damage in radius 3 around the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		local amount = magic.compute_damage(user, spell, 20, 5)
		magic.mass_combat(user, target, 0, 0, amount, effect.FIRE_AREA, 3, damage.FIRE, animation.FIRE)
	end,
}
