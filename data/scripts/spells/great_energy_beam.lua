-- spells/great_energy_beam.lua: Great Energy Beam ("exevo gran vis lux"), what
-- CastSpell's case SPELL_GREAT_ENERGY_BEAM did: energy damage of 120 plus or
-- minus 80 in a beam of range 8 in front of the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 120, 80)
	Magic.angleCombat(caster, mana, soulPoints, amount, CONST_ME_EXPLOSIONHIT, 8, 0, COMBAT_ENERGYDAMAGE)
end
