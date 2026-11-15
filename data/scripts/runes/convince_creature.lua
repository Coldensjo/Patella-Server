-- runes/convince_creature.lua: Convince Creature ("adeta sio"), what
-- UseMagicItem's case SPELL_CONVINCE_CREATURE did: makes the creature on the
-- target field the user's summon.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		if target_creature == 0 then
			game.throw(result.NOCREATURE)
		end
		magic.convince(user, target_creature)
	end,
}
