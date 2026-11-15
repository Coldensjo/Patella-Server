-- spells/great_light.lua: Great Light ("utevo gran lux"), what CastSpell's case
-- SPELL_GREAT_LIGHT did: gives the caster light of radius 8 for 1000 (Enlight).
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.enlight(caster, mana, soulPoints, 8, 1000)
end
