-- spells/magic_shield.lua: Magic Shield ("utamo vita"), what CastSpell's case
-- SPELL_MAGIC_SHIELD did: gives the caster a magic shield for 200 (Shielding).
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.shielding(caster, mana, soulPoints, 200)
end
