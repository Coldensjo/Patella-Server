-- runes/poisonbomb.lua: Poisonbomb ("adevo mas pox"), what UseMagicItem's case
-- SPELL_POISONBOMB did: poison fields in radius 3 around the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		magic.mass_create_field(user, target, 0, 0, field.POISON, 3)
	end,
}
