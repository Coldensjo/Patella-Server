-- spells/flame_strike.lua: Flame Strike ("exori flam"), what CastSpell's case
-- SPELL_FLAME_STRIKE did: fire damage of 45 plus or minus 10 on the field in
-- front of the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 45, 10)
	Magic.angleCombat(caster, mana, soulPoints, amount, CONST_ME_FIREAREA, 1, 0, COMBAT_FIREDAMAGE)
end
