-- runes/firebomb.lua: Firebomb ("adevo mas flam"), what UseMagicItem's case
-- SPELL_FIREBOMB did: fire fields in radius 3 around the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.mass_create_field(user, target, 0, 0, field.FIRE, 3)
	end,
}
