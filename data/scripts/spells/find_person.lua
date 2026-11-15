-- spells/find_person.lua: Find Person ("exiva para"), what CastSpell's case
-- SPELL_FIND_PERSON did: tells the caster where the player named in words[2]
-- is.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.find_person(caster, mana, soul_points, words[2])
	end,
}
