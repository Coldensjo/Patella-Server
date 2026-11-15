-- runes/desintegrate.lua: Desintegrate ("adito tera"), what UseMagicItem's
-- case SPELL_DESINTEGRATE did: removes the movable objects from the target
-- field.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.cleanupFieldOn(user, target, 0, 0)
end
