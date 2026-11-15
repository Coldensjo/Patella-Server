-- runes/fire_field.lua: Fire Field ("adevo grav flam"), what UseMagicItem's
-- case SPELL_FIRE_FIELD did: a fire field on the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.create_field_on(user, target, 0, 0, field.FIRE)
	end,
}
