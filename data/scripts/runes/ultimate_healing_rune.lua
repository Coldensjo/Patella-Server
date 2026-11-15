-- runes/ultimate_healing_rune.lua: Ultimate Healing Rune ("adura vita"), what
-- UseMagicItem's case SPELL_ULTIMATE_HEALING_RUNE did: heals the creature on
-- the target field.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		if target_creature == 0 then
			game.throw(result.NOCREATURE)
		end
		local amount = magic.compute_damage(user, spell, 250, 0)
		magic.heal(target_creature, -1, 0, amount)
	end,
}
