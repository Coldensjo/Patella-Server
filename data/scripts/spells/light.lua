-- spells/light.lua: Light ("utevo lux"), what CastSpell's case SPELL_LIGHT did:
-- gives the caster light of radius 6 for 500 (Enlight).
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.enlight(caster, mana, soulPoints, 6, 500)
end
