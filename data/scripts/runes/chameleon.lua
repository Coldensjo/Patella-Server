-- runes/chameleon.lua: Chameleon ("adevo ina"), what UseMagicItem's case
-- SPELL_CHAMELEON did: gives the user the look of the target object for a
-- while.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.objectIllusion(user, 0, 0, target, 200)
end
