-- runes/sudden_death.lua: Sudden Death ("adori vita vis"), what UseMagicItem's
-- case SPELL_SUDDEN_DEATH did: physical damage to the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	local amount = Magic.computeDamage(user, spell, 150, 20)
	Magic.combat(user, target, 0, 0, amount, CONST_ME_MORTAREA, CONST_ANI_DEATH, COMBAT_PHYSICALDAMAGE)
end
