-- spells/cancel_invisibility.lua: Cancel Invisibility ("exana ina"), what
-- CastSpell's case SPELL_CANCEL_INVISIBILITY did: makes the invisible creatures
-- in radius 4 around the caster visible.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.cancelInvisibility(caster, caster, mana, soulPoints, 4)
end
