-- runes/poison_wall.lua: Poison Wall ("adevo mas grav pox"), what
-- UseMagicItem's case SPELL_POISON_WALL did: a wall of poison fields through
-- the target, extension 2.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.create_field_wall(user, target, 0, 0, field.POISON, 2)
	end,
}
