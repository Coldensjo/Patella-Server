-- spells/power_bolt.lua: Power Bolt ("exevo con vis"), what CastSpell's case
-- SPELL_POWER_BOLT did: conjures 1 power bolt.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.create_arrows(caster, mana, soul_points, ammo.POWER_BOLT, 1)
	end,
}
