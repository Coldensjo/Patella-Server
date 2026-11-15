-- commands/get_quest_value.lua: Get Quest Value ("aliva cogni para"), what
-- CharacterRightSpell's case SPELL_GET_QUEST_VALUE did: tells the caster its quest value of the number.
return {
	on_say = function(caster, number, words)
		command.get_quest_value(caster, words[3])
	end,
}
