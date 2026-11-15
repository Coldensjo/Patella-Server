-- spells/find_person.lua: Find Person ("exiva para"), what CastSpell's case
-- SPELL_FIND_PERSON did: tells the caster where the player named in words[2]
-- is.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.findPerson(caster, mana, soulPoints, words[2])
end
