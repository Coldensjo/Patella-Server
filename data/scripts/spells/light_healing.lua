-- spells/light_healing.lua: Light Healing ("exura"), what CastSpell's case
-- SPELL_LIGHT_HEALING did: heals the caster by 20 plus or minus 10, times the
-- damage factor.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.heal(caster, mana, soulPoints, Magic.computeDamage(caster, spell, 20, 10))
end
