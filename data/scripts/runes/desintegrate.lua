-- runes/desintegrate.lua: Desintegrate ("adito tera"), what UseMagicItem's
-- case SPELL_DESINTEGRATE did: removes the movable objects from the target
-- field.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.cleanup_field_on(user, target, 0, 0)
	end,
}
