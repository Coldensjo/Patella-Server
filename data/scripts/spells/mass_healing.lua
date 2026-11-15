-- spells/mass_healing.lua: Mass Healing ("exura gran mas res"), what
-- CastSpell's case SPELL_MASS_HEALING did: heals by 200 plus or minus 40 in
-- radius 4 around the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 200, 40)
	Magic.massHeal(caster, caster, mana, soulPoints, amount, 4)
end
