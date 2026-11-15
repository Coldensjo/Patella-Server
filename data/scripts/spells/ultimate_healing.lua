-- spells/ultimate_healing.lua: Ultimate Healing ("exura vita"), what
-- CastSpell's case SPELL_ULTIMATE_HEALING did: heals the caster by 250 plus or
-- minus 50, times the damage factor.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.heal(caster, mana, soulPoints, Magic.computeDamage(caster, spell, 250, 50))
end
