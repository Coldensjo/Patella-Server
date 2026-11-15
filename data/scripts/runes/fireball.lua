-- runes/fireball.lua: Fireball ("adori flam"), what UseMagicItem's case
-- SPELL_FIREBALL did: fire damage in radius 3 around the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	local amount = Magic.computeDamage(user, spell, 20, 5)
	Magic.massCombat(user, target, 0, 0, amount, CONST_ME_FIREAREA, 3, COMBAT_FIREDAMAGE, CONST_ANI_FIRE)
end
