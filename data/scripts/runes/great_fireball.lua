-- runes/great_fireball.lua: Great Fireball ("adori gran flam"), what
-- UseMagicItem's case SPELL_GREAT_FIREBALL did: fire damage in radius 4 around
-- the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	local amount = Magic.computeDamage(user, spell, 50, 15)
	Magic.massCombat(user, target, 0, 0, amount, CONST_ME_FIREAREA, 4, COMBAT_FIREDAMAGE, CONST_ANI_FIRE)
end
