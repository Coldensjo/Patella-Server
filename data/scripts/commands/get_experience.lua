-- commands/get_experience.lua: Get Experience ("alevo cogni para"), what
-- CharacterRightSpell's case SPELL_GET_EXPERIENCE did: gives the caster the number of experience points.
function onSay(caster, command, words)
	Command.createKnowledge(caster, words[3], nil)
end
