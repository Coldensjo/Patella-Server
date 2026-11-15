-- spells/ultimate_light.lua: Ultimate Light ("utevo vis lux"), what CastSpell's
-- case SPELL_ULTIMATE_LIGHT did: gives the caster light of radius 9 for 2000
-- (Enlight).
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.enlight(caster, mana, soul_points, 9, 2000)
	end,
}
