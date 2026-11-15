-- spells/conjure_bolt.lua: Conjure Bolt ("exevo con mort"), what CastSpell's
-- case SPELL_CONJURE_BOLT did: conjures 5 bolts.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.create_arrows(caster, mana, soul_points, ammo.BOLT, 5)
	end,
}
