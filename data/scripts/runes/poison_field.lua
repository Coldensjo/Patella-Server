-- runes/poison_field.lua: Poison Field ("adevo grav pox"), what UseMagicItem's
-- case SPELL_POISON_FIELD did: a poison field on the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.create_field_on(user, target, 0, 0, field.POISON)
	end,
}
