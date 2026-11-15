-- spells/haste.lua: Haste ("utani hur"), what CastSpell's case SPELL_HASTE did:
-- makes the caster faster (MagicGoStrength 30, 3).
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.magicGoStrength(caster, caster, mana, soulPoints, 30, 3)
end
