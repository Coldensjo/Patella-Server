-- runes/poison_wall.lua: Poison Wall ("adevo mas grav pox"), what
-- UseMagicItem's case SPELL_POISON_WALL did: a wall of poison fields through
-- the target, extension 2.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.createFieldWall(user, target, 0, 0, FIELD_POISON, 2)
end
