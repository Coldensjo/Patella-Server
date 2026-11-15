-- spells/power_bolt.lua: Power Bolt ("exevo con vis"), what CastSpell's case
-- SPELL_POWER_BOLT did: conjures 1 power bolt.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.createArrows(caster, mana, soulPoints, CONJURE_POWER_BOLT, 1)
end
