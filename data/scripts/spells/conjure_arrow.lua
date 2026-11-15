-- spells/conjure_arrow.lua: Conjure Arrow ("exevo con"), what CastSpell's case
-- SPELL_CONJURE_ARROW did: conjures 10 arrows.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.createArrows(caster, mana, soulPoints, CONJURE_ARROW, 10)
end
