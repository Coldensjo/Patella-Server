-- commands/summon_wild_creature.lua: Summon Wild Creature ("alevo res para"), what
-- CharacterRightSpell's case SPELL_SUMMON_WILD_CREATURE did: creates a monster of the race named, for no mana and not as a summon.
return {
	on_say = function(caster, number, words)
		magic.summon_creature(caster, 0, words[3], true)
	end,
}
