-- runes/paralyze.lua: Paralyze ("adana ani"), what UseMagicItem's case
-- SPELL_PARALYZE did: slows the creature on the target field (druids only).
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		if target_creature == 0 then
			game.throw(result.NOCREATURE)
		end
		if creature.effective_profession(user) ~= profession.DRUID then
			game.throw(result.NOTUSABLE)
		end
		magic.magic_go_strength(user, target_creature, mana, soul_points, -101, 1)
	end,
}
