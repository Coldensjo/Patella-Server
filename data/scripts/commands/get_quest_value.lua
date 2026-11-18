-- commands/get_quest_value.lua: Get Quest Value ("aliva cogni para"), what
-- GetQuestValue (the original magic.cc) did: with the command's right, tells
-- the caster its quest value of the number the word says (read as a clamped
-- strtol, Game.toInt; 0 to MAX_QUESTS - 1, another number is told the caster).
-- player:getStorageValue prints the value at level 3.

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local number = Game.toInt(words[3])
	if number >= 0 and number <= MAX_QUESTS - 1 then
		local value = caster:getStorageValue(number)
		caster:sendTextMessage(MESSAGE_INFO_DESCR, string.format("Quest value %d is %d.", number, value))
	else
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "Invalid quest number.")
	end
end
