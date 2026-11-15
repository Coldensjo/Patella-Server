-- runes/destroy_field.lua: Destroy Field ("adito grav"), what UseMagicItem's
-- case SPELL_DESTROY_FIELD did: removes a field from the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.delete_field(user, target, 0, 0)
	end,
}
