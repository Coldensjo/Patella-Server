-- spells/antidote.lua: Antidote ("exana pox"), what CastSpell's case
-- SPELL_ANTIDOTE did: cures the caster of poison.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.negatePoison(caster, mana, soulPoints, caster)
end
