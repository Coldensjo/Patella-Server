-- spells/summon_creature.lua: Summon Creature ("utevo res para"), what
-- CastSpell's case SPELL_SUMMON_CREATURE did: summons the creature the caster
-- names in words[3].
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.summon_creature(caster, mana, words[3], false)
	end,
}
