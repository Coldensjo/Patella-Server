-- spells/energy_strike.lua: Energy Strike ("exori vis"), what CastSpell's case
-- SPELL_ENERGY_STRIKE did: energy damage of 45 plus or minus 10 on the field in
-- front of the caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		local amount = magic.compute_damage(caster, spell, 45, 10)
		magic.angle_combat(caster, mana, soul_points, amount, effect.TELEPORT, 1, 0, damage.ENERGY)
	end,
}
