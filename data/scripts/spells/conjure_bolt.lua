-- spells/conjure_bolt.lua: Conjure Bolt ("exevo con mort"), what CastSpell's
-- case SPELL_CONJURE_BOLT did: conjures 5 bolts.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.createArrows(caster, mana, soulPoints, CONJURE_BOLT, 5)
end
