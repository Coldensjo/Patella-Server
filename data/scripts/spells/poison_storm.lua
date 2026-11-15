-- spells/poison_storm.lua: Poison Storm ("exevo gran mas pox"), what
-- CastSpell's case SPELL_POISON_STORM did: poisons with 200 plus or minus 50 in
-- radius 8 around the caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		local amount = magic.compute_damage(caster, spell, 200, 50)
		magic.mass_combat(caster, creature.object(caster), mana, soul_points, amount, effect.POISON, 8, damage.POISON_CONDITION, animation.NONE)
	end,
}
