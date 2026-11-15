-- spells/explosive_arrow.lua: Explosive Arrow ("exevo con flam"), what
-- CastSpell's case SPELL_EXPLOSIVE_ARROW did: conjures 3 explosive arrows.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.createArrows(caster, mana, soulPoints, CONJURE_EXPLOSIVE_ARROW, 3)
end
