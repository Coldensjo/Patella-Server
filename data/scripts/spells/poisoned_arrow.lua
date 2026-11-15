-- spells/poisoned_arrow.lua: Poisoned Arrow ("exevo con pox"), what CastSpell's
-- case SPELL_POISONED_ARROW did: conjures 5 poisoned arrows.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.create_arrows(caster, mana, soul_points, ammo.POISONED_ARROW, 5)
	end,
}
