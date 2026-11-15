-- spells/challenge.lua: Challenge ("exeta res"), what CastSpell's case
-- SPELL_CHALLENGE did: makes the monsters in radius 2 around the caster attack
-- it.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.challenge(caster, mana, soul_points, 2)
	end,
}
