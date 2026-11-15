-- commands/levitate_gamemaster.lua: Levitate Gamemaster ("alani hur para"), what
-- CharacterRightSpell's case SPELL_LEVITATE_GAMEMASTER did: moves the caster up or down in the direction of the word, for no mana.
return {
	on_say = function(caster, number, words)
		magic.magic_climbing_free(caster, words[3])
	end,
}
