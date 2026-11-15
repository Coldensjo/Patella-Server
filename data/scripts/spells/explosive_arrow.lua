-- spells/explosive_arrow.lua: Explosive Arrow ("exevo con flam"), what
-- CastSpell's case SPELL_EXPLOSIVE_ARROW did: conjures 3 explosive arrows.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.create_arrows(caster, mana, soul_points, ammo.EXPLOSIVE_ARROW, 3)
	end,
}
