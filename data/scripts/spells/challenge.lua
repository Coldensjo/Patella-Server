-- spells/challenge.lua: Challenge ("exeta res"), what CastSpell's case
-- SPELL_CHALLENGE did: makes the monsters in radius 2 around the caster attack
-- it.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.challenge(caster, mana, soulPoints, 2)
end
