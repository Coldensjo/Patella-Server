-- spells/energy_beam.lua: Energy Beam ("exevo vis lux"), what CastSpell's case
-- SPELL_ENERGY_BEAM did: energy damage of 60 plus or minus 20 in a beam of
-- range 5 in front of the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 60, 20)
	Magic.angleCombat(caster, mana, soulPoints, amount, CONST_ME_EXPLOSIONHIT, 5, 0, COMBAT_ENERGYDAMAGE)
end
