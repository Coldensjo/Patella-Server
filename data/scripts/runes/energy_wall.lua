-- runes/energy_wall.lua: Energy Wall ("adevo mas grav vis"), what
-- UseMagicItem's case SPELL_ENERGY_WALL did: a wall of energy fields through
-- the target, extension 3.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.createFieldWall(user, target, 0, 0, FIELD_ENERGY, 3)
end
