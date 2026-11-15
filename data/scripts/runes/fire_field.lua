-- runes/fire_field.lua: Fire Field ("adevo grav flam"), what UseMagicItem's
-- case SPELL_FIRE_FIELD did: a fire field on the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.createFieldOn(user, target, 0, 0, FIELD_FIRE)
end
