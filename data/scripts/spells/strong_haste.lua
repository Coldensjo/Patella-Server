-- spells/strong_haste.lua: Strong Haste ("utani gran hur"), what CastSpell's
-- case SPELL_STRONG_HASTE did: makes the caster much faster (MagicGoStrength
-- 70, 2).
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.magic_go_strength(caster, caster, mana, soul_points, 70, 2)
	end,
}
