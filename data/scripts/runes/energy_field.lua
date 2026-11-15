-- runes/energy_field.lua: Energy Field ("adevo grav vis"), what UseMagicItem's
-- case SPELL_ENERGY_FIELD did: an energy field on the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.create_field_on(user, target, 0, 0, field.ENERGY)
	end,
}
