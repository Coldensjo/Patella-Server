-- runes/chameleon.lua: Chameleon ("adevo ina"), what UseMagicItem's case
-- SPELL_CHAMELEON did: gives the user the look of the target object for a
-- while.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.object_illusion(user, 0, 0, target, 200)
	end,
}
