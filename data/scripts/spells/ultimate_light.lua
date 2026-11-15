-- spells/ultimate_light.lua: Ultimate Light ("utevo vis lux"), what CastSpell's
-- case SPELL_ULTIMATE_LIGHT did: gives the caster light of radius 9 for 2000
-- (Enlight).
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.enlight(caster, mana, soulPoints, 9, 2000)
end
