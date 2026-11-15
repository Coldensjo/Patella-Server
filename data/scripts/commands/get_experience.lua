-- commands/get_experience.lua: Get Experience ("alevo cogni para"), what
-- CharacterRightSpell's case SPELL_GET_EXPERIENCE did: gives the caster the number of experience points.
return {
	on_say = function(caster, number, words)
		command.create_knowledge(caster, words[3], nil)
	end,
}
