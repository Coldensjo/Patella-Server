-- runes/energy_wall.lua: Energy Wall ("adevo mas grav vis"), what
-- UseMagicItem's case SPELL_ENERGY_WALL did: a wall of energy fields through
-- the target, extension 3.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.create_field_wall(user, target, 0, 0, field.ENERGY, 3)
	end,
}
