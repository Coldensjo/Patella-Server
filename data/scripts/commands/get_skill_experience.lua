-- commands/get_skill_experience.lua: Get Skill Experience ("alevo cogni para para"), what
-- CharacterRightSpell's case SPELL_GET_SKILL_EXPERIENCE did: gives the caster points of the skill numbered by the first word, as many as the second says.
return {
	on_say = function(caster, number, words)
		command.create_knowledge(caster, words[3], words[4])
	end,
}
