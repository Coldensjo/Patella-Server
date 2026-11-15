-- commands/clear_quest_values.lua: Clear Quest Values ("alana cogni"), what
-- CharacterRightSpell's case SPELL_CLEAR_QUEST_VALUES did: sets all of the caster's quest values to 0.
function onSay(caster, command, words)
	Command.clearQuestValues(caster)
end
