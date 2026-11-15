-- runes/convince_creature.lua: Convince Creature ("adeta sio"), what
-- UseMagicItem's case SPELL_CONVINCE_CREATURE did: makes the creature on the
-- target field the user's summon.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	if targetCreature == nil then
		Game.throwResult(RETURNVALUE_CANONLYUSETHISRUNEONCREATURES)
	end
	Magic.convince(user, targetCreature)
end
