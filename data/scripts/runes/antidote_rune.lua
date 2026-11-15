-- runes/antidote_rune.lua: Antidote Rune ("adana pox"), what UseMagicItem's
-- case SPELL_ANTIDOTE_RUNE did: cures the poison of the creature on the target
-- field.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		if target_creature == 0 then
			game.throw(result.NOCREATURE)
		end
		magic.negate_poison(user, 0, 0, target_creature)
	end,
}
