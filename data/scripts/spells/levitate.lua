-- spells/levitate.lua: Levitate ("exani hur para"), what CastSpell's case
-- SPELL_LEVITATE did: moves the caster a floor up or down, as words[3] says.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.magic_climbing(caster, mana, soul_points, words[3])
	end,
}
