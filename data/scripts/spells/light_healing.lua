-- spells/light_healing.lua: Light Healing ("exura"), what CastSpell's case
-- SPELL_LIGHT_HEALING did: heals the caster by 20 plus or minus 10, times the
-- damage factor.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.heal(caster, mana, soul_points, magic.compute_damage(caster, spell, 20, 10))
	end,
}
