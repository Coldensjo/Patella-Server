-- runes/animate_dead.lua: Animate Dead ("adana mort"), what UseMagicItem's
-- case SPELL_ANIMATE_DEAD did: raises a skeleton from a corpse on the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.raiseDead(user, target, 0, 0)
end
