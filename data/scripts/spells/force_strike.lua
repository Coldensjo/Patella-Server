-- spells/force_strike.lua: Force Strike ("exori mort"), what CastSpell's case
-- SPELL_FORCE_STRIKE did: physical damage of 45 plus or minus 10 on the field
-- in front of the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 45, 10)
	Magic.angleCombat(caster, mana, soulPoints, amount, CONST_ME_MORTAREA, 1, 0, COMBAT_PHYSICALDAMAGE)
end
