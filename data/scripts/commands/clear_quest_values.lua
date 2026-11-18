-- commands/clear_quest_values.lua: Clear Quest Values ("alana cogni"), what
-- ClearQuestValues (the original magic.cc) did: with the command's right, sets
-- all of the caster's quest values, 0 to MAX_QUESTS - 1 in order, to 0 (each
-- printed at level 3 by player:setStorageValue) and tells the caster.

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	for number = 0, MAX_QUESTS - 1 do
		caster:setStorageValue(number, 0)
	end

	caster:sendTextMessage(MESSAGE_INFO_DESCR, "All quest values deleted.")
end
