-- spells/magic_rope.lua: Magic Rope ("exani tera"), what CastSpell's case
-- SPELL_MAGIC_ROPE did: pulls the caster up through the rope spot below.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.magic_rope(caster, mana, soul_points)
	end,
}
