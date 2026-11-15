-- spells/food.lua: Food ("exevo pan"), what CastSpell's case SPELL_FOOD did:
-- creates food for the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.createFood(caster, mana, soulPoints)
end
