-- spells/undead_legion.lua: Undead Legion ("exana mas mort"), what CastSpell's
-- case SPELL_UNDEAD_LEGION did: raises the corpses in radius 4 around the
-- caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.mass_raise_dead(caster, creature.object(caster), mana, soul_points, 4)
	end,
}
