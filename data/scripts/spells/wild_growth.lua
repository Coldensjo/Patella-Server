-- spells/wild_growth.lua: Wild Growth ("exevo grav vita"), what CastSpell's
-- case SPELL_WILD_GROWTH did: creates rushwood in front of the caster.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.create_field(caster, mana, soul_points, field.RUSHWOOD)
	end,
}
