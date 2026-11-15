-- runes/heavy_magic_missile.lua: Heavy Magic Missile ("adori gran"), what
-- UseMagicItem's case SPELL_HEAVY_MAGIC_MISSILE did: energy damage to the
-- target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	local amount = Magic.computeDamage(user, spell, 30, 10)
	Magic.combat(user, target, 0, 0, amount, CONST_ME_EXPLOSIONHIT, CONST_ANI_FIRE, COMBAT_ENERGYDAMAGE)
end
