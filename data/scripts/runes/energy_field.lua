-- runes/energy_field.lua: Energy Field ("adevo grav vis"), what UseMagicItem's
-- case SPELL_ENERGY_FIELD did: an energy field on the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.createFieldOn(user, target, 0, 0, FIELD_ENERGY)
end
