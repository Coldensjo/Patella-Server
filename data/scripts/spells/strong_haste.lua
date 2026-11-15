-- spells/strong_haste.lua: Strong Haste ("utani gran hur"), what CastSpell's
-- case SPELL_STRONG_HASTE did: makes the caster much faster (MagicGoStrength
-- 70, 2).
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.magicGoStrength(caster, caster, mana, soulPoints, 70, 2)
end
