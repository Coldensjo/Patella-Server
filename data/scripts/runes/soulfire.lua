-- runes/soulfire.lua: Soulfire ("adevo res flam"), what UseMagicItem's case
-- SPELL_SOULFIRE did: sets the target burning (fire damage over time).
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		local amount = magic.compute_damage(user, spell, 120, 20)
		magic.combat(user, target, 0, 0, amount, effect.FIRE_HIT, animation.FIRE, damage.FIRE_CONDITION)
	end,
}
