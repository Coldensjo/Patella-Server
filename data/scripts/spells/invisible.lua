-- spells/invisible.lua: Invisible ("utana vid"), what CastSpell's case
-- SPELL_INVISIBLE did: makes the caster invisible for 200 (Invisibility).
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.invisibility(caster, mana, soul_points, 200)
	end,
}
