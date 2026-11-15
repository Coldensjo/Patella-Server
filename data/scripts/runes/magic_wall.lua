-- runes/magic_wall.lua: Magic Wall ("adevo grav tera"), what UseMagicItem's
-- case SPELL_MAGIC_WALL did: a magic wall on the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.createFieldOn(user, target, 0, 0, FIELD_MAGICWALL)
end
