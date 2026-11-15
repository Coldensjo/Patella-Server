-- spells/poison_storm.lua: Poison Storm ("exevo gran mas pox"), what
-- CastSpell's case SPELL_POISON_STORM did: poisons with 200 plus or minus 50 in
-- radius 8 around the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 200, 50)
	Magic.massCombat(caster, caster, mana, soulPoints, amount, CONST_ME_GREEN_RINGS, 8, COMBAT_POISONCONDITION, CONST_ANI_NONE)
end
