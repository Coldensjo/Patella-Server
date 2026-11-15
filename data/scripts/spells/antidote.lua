-- spells/antidote.lua: Antidote ("exana pox"), what CastSpell's case
-- SPELL_ANTIDOTE did: cures the caster of poison.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.negate_poison(caster, mana, soul_points, caster)
	end,
}
