-- runes/energybomb.lua: Energybomb ("adevo mas vis"), what UseMagicItem's case
-- SPELL_ENERGYBOMB did: energy fields in radius 3 around the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.mass_create_field(user, target, 0, 0, field.ENERGY, 3)
	end,
}
