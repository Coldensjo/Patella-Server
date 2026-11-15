-- spells/mass_healing.lua: Mass Healing ("exura gran mas res"), what
-- CastSpell's case SPELL_MASS_HEALING did: heals by 200 plus or minus 40 in
-- radius 4 around the caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		local amount = magic.compute_damage(caster, spell, 200, 40)
		magic.mass_heal(caster, creature.object(caster), mana, soul_points, amount, 4)
	end,
}
