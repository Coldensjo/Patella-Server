-- commands/levitate_gamemaster.lua: Levitate Gamemaster ("alani hur para"), what
-- CharacterRightSpell's case SPELL_LEVITATE_GAMEMASTER did: moves the caster up or down in the direction of the word, for no mana.
function onSay(caster, command, words)
	Magic.magicClimbingFree(caster, words[3])
end
