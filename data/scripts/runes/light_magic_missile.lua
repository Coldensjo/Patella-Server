-- runes/light_magic_missile.lua: Light Magic Missile ("adori"), what
-- UseMagicItem's case SPELL_LIGHT_MAGIC_MISSILE did: energy damage to the
-- target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		local amount = magic.compute_damage(user, spell, 15, 5)
		magic.combat(user, target, 0, 0, amount, effect.EXPLOSION, animation.FIRE, damage.ENERGY)
	end,
}
