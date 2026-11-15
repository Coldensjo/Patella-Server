-- spells/intense_healing.lua: Intense Healing ("exura gran"), what CastSpell's
-- case SPELL_INTENSE_HEALING did: heals the caster by 40 plus or minus 20,
-- times the damage factor.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.heal(caster, mana, soulPoints, Magic.computeDamage(caster, spell, 40, 20))
end
