-- spells/creature_illusion.lua: Creature Illusion ("utevo res ina para"), what
-- CastSpell's case SPELL_CREATURE_ILLUSION did: gives the caster the look of
-- the creature named in words[4] for 200 (CreatureIllusion).
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.creature_illusion(caster, mana, soul_points, words[4], 200)
	end,
}
