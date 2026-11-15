-- runes/heavy_magic_missile.lua: Heavy Magic Missile ("adori gran"), what
-- UseMagicItem's case SPELL_HEAVY_MAGIC_MISSILE did: energy damage to the
-- target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		local amount = magic.compute_damage(user, spell, 30, 10)
		magic.combat(user, target, 0, 0, amount, effect.EXPLOSION, animation.FIRE, damage.ENERGY)
	end,
}
