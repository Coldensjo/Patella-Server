-- spells/ultimate_healing.lua: Ultimate Healing ("exura vita"), what
-- CastSpell's case SPELL_ULTIMATE_HEALING did: heals the caster by 250 plus or
-- minus 50, times the damage factor.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.heal(caster, mana, soul_points, magic.compute_damage(caster, spell, 250, 50))
	end,
}
