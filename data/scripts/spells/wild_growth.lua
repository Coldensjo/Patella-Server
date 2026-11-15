-- spells/wild_growth.lua: Wild Growth ("exevo grav vita"), what CastSpell's
-- case SPELL_WILD_GROWTH did: creates rushwood in front of the caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.createField(caster, mana, soulPoints, FIELD_RUSHWOOD)
end
