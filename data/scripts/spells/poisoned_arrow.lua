-- spells/poisoned_arrow.lua: Poisoned Arrow ("exevo con pox"), what CastSpell's
-- case SPELL_POISONED_ARROW did: conjures 5 poisoned arrows.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.createArrows(caster, mana, soulPoints, CONJURE_POISONED_ARROW, 5)
end
