-- spells/berserk.lua: Berserk ("exori"), what CastSpell's case SPELL_BERSERK
-- did: physical damage of level * (80 plus or minus 20, times the caster's
-- damage factor of level and magic level) / 25 in radius 2 around the caster,
-- for 4 mana per level and no soul points.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		local amount = magic.compute_damage(caster, spell, 80, 20)
		amount = game.idiv(creature.skill(caster, skill.LEVEL) * amount, 25)
		magic.mass_combat(caster, creature.object(caster), creature.skill(caster, skill.LEVEL) * 4, 0, amount, effect.HIT_AREA, 2, damage.PHYSICAL, animation.NONE)
	end,
}
