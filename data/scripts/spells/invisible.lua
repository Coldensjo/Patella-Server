-- spells/invisible.lua: Invisible ("utana vid"), what CastSpell's case
-- SPELL_INVISIBLE did: makes the caster invisible for 200 (Invisibility).
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.invisibility(caster, mana, soulPoints, 200)
end
