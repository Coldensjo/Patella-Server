-- spells/haste.lua: Haste ("utani hur"), what CastSpell's case SPELL_HASTE did:
-- makes the caster faster (MagicGoStrength 30, 3).
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.magic_go_strength(caster, caster, mana, soul_points, 30, 3)
	end,
}
