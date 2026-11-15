-- spells/conjure_arrow.lua: Conjure Arrow ("exevo con"), what CastSpell's case
-- SPELL_CONJURE_ARROW did: conjures 10 arrows.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.create_arrows(caster, mana, soul_points, ammo.ARROW, 10)
	end,
}
