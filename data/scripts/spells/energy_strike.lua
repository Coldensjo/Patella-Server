-- spells/energy_strike.lua: Energy Strike ("exori vis"), what CastSpell's case
-- SPELL_ENERGY_STRIKE did: energy damage of 45 plus or minus 10 on the field in
-- front of the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 45, 10)
	Magic.angleCombat(caster, mana, soulPoints, amount, CONST_ME_TELEPORT, 1, 0, COMBAT_ENERGYDAMAGE)
end
