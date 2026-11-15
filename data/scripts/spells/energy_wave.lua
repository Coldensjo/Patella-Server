-- spells/energy_wave.lua: Energy Wave ("exevo mort hur"), what CastSpell's case
-- SPELL_ENERGY_WAVE did: energy damage of 150 plus or minus 50 in a wave of
-- range 5 and angle 30 in front of the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 150, 50)
	Magic.angleCombat(caster, mana, soulPoints, amount, CONST_ME_TELEPORT, 5, 30, COMBAT_ENERGYDAMAGE)
end
