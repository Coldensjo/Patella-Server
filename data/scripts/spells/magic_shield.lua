-- spells/magic_shield.lua: Magic Shield ("utamo vita"), what CastSpell's case
-- SPELL_MAGIC_SHIELD did: gives the caster a magic shield for 200 (Shielding).
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.shielding(caster, mana, soul_points, 200)
	end,
}
