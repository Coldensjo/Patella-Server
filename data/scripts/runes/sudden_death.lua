-- runes/sudden_death.lua: Sudden Death ("adori vita vis"), what UseMagicItem's
-- case SPELL_SUDDEN_DEATH did: physical damage to the target.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		local amount = magic.compute_damage(user, spell, 150, 20)
		magic.combat(user, target, 0, 0, amount, effect.DEATH, animation.DEATH, damage.PHYSICAL)
	end,
}
