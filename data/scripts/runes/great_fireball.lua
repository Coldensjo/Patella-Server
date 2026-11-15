-- runes/great_fireball.lua: Great Fireball ("adori gran flam"), what
-- UseMagicItem's case SPELL_GREAT_FIREBALL did: fire damage in radius 4 around
-- the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		local amount = magic.compute_damage(user, spell, 50, 15)
		magic.mass_combat(user, target, 0, 0, amount, effect.FIRE_AREA, 4, damage.FIRE, animation.FIRE)
	end,
}
