-- runes/firebomb.lua: Firebomb ("adevo mas flam"), what UseMagicItem's case
-- SPELL_FIREBOMB did: fire fields in radius 3 around the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.massCreateField(user, target, 0, 0, FIELD_FIRE, 3)
end
