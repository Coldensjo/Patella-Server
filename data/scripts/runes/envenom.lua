-- runes/envenom.lua: Envenom ("adevo res pox"), what UseMagicItem's case
-- SPELL_ENVENOM did: poisons the target (poison damage over time).
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		local amount = magic.compute_damage(user, spell, 70, 20)
		magic.combat(user, target, 0, 0, amount, effect.POISON_HIT, animation.ENERGY, damage.POISON_CONDITION)
	end,
}
