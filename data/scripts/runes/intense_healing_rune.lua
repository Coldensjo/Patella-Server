-- runes/intense_healing_rune.lua: Intense Healing Rune ("adura gran"), what
-- UseMagicItem's case SPELL_INTENSE_HEALING_RUNE did: heals the creature on
-- the target field.
return {
	on_use = function(user, spell, mana, soul_points, target, target_creature)
		if target_creature == 0 then
			game.throw(result.NOCREATURE)
		end
		local amount = magic.compute_damage(user, spell, 70, 30)
		magic.heal(target_creature, -1, 0, amount)
	end,
}
