-- runes/light_magic_missile.lua: Light Magic Missile ("adori"), what
-- UseMagicItem's case SPELL_LIGHT_MAGIC_MISSILE did: energy damage to the
-- target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	local amount = Magic.computeDamage(user, spell, 15, 5)
	Magic.combat(user, target, 0, 0, amount, CONST_ME_EXPLOSIONHIT, CONST_ANI_FIRE, COMBAT_ENERGYDAMAGE)
end
