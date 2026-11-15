-- runes/poisonbomb.lua: Poisonbomb ("adevo mas pox"), what UseMagicItem's case
-- SPELL_POISONBOMB did: poison fields in radius 3 around the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.massCreateField(user, target, 0, 0, FIELD_POISON, 3)
end
