-- commands/set_quest_value.lua: Set Quest Value ("aleta cogni para para"), what
-- CharacterRightSpell's case SPELL_SET_QUEST_VALUE did: sets the caster's quest value of the first number to the second.
function onSay(caster, command, words)
	Command.setQuestValue(caster, words[3], words[4])
end
