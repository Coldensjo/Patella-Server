-- spells/fire_wave.lua: Fire Wave ("exevo flam hur"), what CastSpell's case
-- SPELL_FIRE_WAVE did: fire damage of 30 plus or minus 10 in a wave of range 4
-- and angle 45 in front of the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 30, 10)
	Magic.angleCombat(caster, mana, soulPoints, amount, CONST_ME_FIREAREA, 4, 45, COMBAT_FIREDAMAGE)
end
