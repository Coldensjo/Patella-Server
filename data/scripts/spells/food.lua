-- spells/food.lua: Food ("exevo pan"), what CastSpell's case SPELL_FOOD did:
-- creates food for the caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.create_food(caster, mana, soul_points)
	end,
}
