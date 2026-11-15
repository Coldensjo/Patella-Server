-- runes/fire_wall.lua: Fire Wall ("adevo mas grav flam"), what UseMagicItem's
-- case SPELL_FIRE_WALL did: a wall of fire fields through the target,
-- extension 2.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.createFieldWall(user, target, 0, 0, FIELD_FIRE, 2)
end
