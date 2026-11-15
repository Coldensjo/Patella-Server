-- runes/poison_field.lua: Poison Field ("adevo grav pox"), what UseMagicItem's
-- case SPELL_POISON_FIELD did: a poison field on the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.createFieldOn(user, target, 0, 0, FIELD_POISON)
end
