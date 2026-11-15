-- spells/ultimate_explosion.lua: Ultimate Explosion ("exevo gran mas vis"),
-- what CastSpell's case SPELL_ULTIMATE_EXPLOSION did: physical damage of 250
-- plus or minus 50 in radius 6 around the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 250, 50)
	Magic.massCombat(caster, caster, mana, soulPoints, amount, CONST_ME_EXPLOSIONAREA, 6, COMBAT_PHYSICALDAMAGE, CONST_ANI_FIRE)
end
