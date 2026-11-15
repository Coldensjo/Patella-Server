-- spells/intense_healing.lua: Intense Healing ("exura gran"), what CastSpell's
-- case SPELL_INTENSE_HEALING did: heals the caster by 40 plus or minus 20,
-- times the damage factor.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.heal(caster, mana, soul_points, magic.compute_damage(caster, spell, 40, 20))
	end,
}
