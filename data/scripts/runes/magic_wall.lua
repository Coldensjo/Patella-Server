-- runes/magic_wall.lua: Magic Wall ("adevo grav tera"), what UseMagicItem's
-- case SPELL_MAGIC_WALL did: a magic wall on the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.create_field_on(user, target, 0, 0, field.MAGICWALL)
	end,
}
