-- spells/levitate.lua: Levitate ("exani hur para"), what CastSpell's case
-- SPELL_LEVITATE did: moves the caster a floor up or down, as words[3] says.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.magicClimbing(caster, mana, soulPoints, words[3])
end
