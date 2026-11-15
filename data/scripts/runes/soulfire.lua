-- runes/soulfire.lua: Soulfire ("adevo res flam"), what UseMagicItem's case
-- SPELL_SOULFIRE did: sets the target burning (fire damage over time).
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	local amount = Magic.computeDamage(user, spell, 120, 20)
	Magic.combat(user, target, 0, 0, amount, CONST_ME_HITBYFIRE, CONST_ANI_FIRE, COMBAT_FIRECONDITION)
end
