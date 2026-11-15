-- spells/enchant_staff.lua: Enchant Staff ("exeta vis"), what CastSpell's case
-- SPELL_ENCHANT_STAFF did: turns a staff (90, 25) the caster holds into an
-- enchanted staff (90, 57).
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.enchant_object(caster, mana, soul_points, object.new_type(90, 25), object.new_type(90, 57))
	end,
}
