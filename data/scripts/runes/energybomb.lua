-- runes/energybomb.lua: Energybomb ("adevo mas vis"), what UseMagicItem's case
-- SPELL_ENERGYBOMB did: energy fields in radius 3 around the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.massCreateField(user, target, 0, 0, FIELD_ENERGY, 3)
end
