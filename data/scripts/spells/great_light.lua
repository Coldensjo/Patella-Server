-- spells/great_light.lua: Great Light ("utevo gran lux"), what CastSpell's case
-- SPELL_GREAT_LIGHT did: gives the caster light of radius 8 for 1000 (Enlight).
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.enlight(caster, mana, soul_points, 8, 1000)
	end,
}
