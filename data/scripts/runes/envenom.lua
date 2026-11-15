-- runes/envenom.lua: Envenom ("adevo res pox"), what UseMagicItem's case
-- SPELL_ENVENOM did: poisons the target (poison damage over time).
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	local amount = Magic.computeDamage(user, spell, 70, 20)
	Magic.combat(user, target, 0, 0, amount, CONST_ME_HITBYPOISON, CONST_ANI_ENERGY, COMBAT_POISONCONDITION)
end
