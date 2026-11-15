-- runes/animate_dead.lua: Animate Dead ("adana mort"), what UseMagicItem's
-- case SPELL_ANIMATE_DEAD did: raises a skeleton from a corpse on the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.raise_dead(user, target, 0, 0)
	end,
}
