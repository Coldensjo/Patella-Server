-- spells/light.lua: Light ("utevo lux"), what CastSpell's case SPELL_LIGHT did:
-- gives the caster light of radius 6 for 500 (Enlight).
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.enlight(caster, mana, soul_points, 6, 500)
	end,
}
