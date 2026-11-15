-- commands/get_quest_value.lua: Get Quest Value ("aliva cogni para"), what
-- CharacterRightSpell's case SPELL_GET_QUEST_VALUE did: tells the caster its quest value of the number.
function onSay(caster, command, words)
	Command.getQuestValue(caster, words[3])
end
