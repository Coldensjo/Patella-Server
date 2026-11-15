-- spells/cancel_invisibility.lua: Cancel Invisibility ("exana ina"), what
-- CastSpell's case SPELL_CANCEL_INVISIBILITY did: makes the invisible creatures
-- in radius 4 around the caster visible.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.cancel_invisibility(caster, creature.object(caster), mana, soul_points, 4)
	end,
}
