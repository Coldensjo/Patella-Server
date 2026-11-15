-- runes/explosion.lua: Explosion ("adevo mas hur"), what UseMagicItem's case
-- SPELL_EXPLOSION did: physical damage in radius 1 around the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	local amount = Magic.computeDamage(user, spell, 60, 40)
	Magic.massCombat(user, target, 0, 0, amount, CONST_ME_EXPLOSIONAREA, 1, COMBAT_PHYSICALDAMAGE, CONST_ANI_FIRE)
end
