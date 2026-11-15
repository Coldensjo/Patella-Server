-- runes/fire_wall.lua: Fire Wall ("adevo mas grav flam"), what UseMagicItem's
-- case SPELL_FIRE_WALL did: a wall of fire fields through the target,
-- extension 2.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.create_field_wall(user, target, 0, 0, field.FIRE, 2)
	end,
}
