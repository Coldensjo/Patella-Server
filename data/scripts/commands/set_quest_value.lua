-- commands/set_quest_value.lua: Set Quest Value ("aleta cogni para para"),
-- what SetQuestValue (the original magic.cc) did: with the command's right,
-- sets the caster's quest value of the first word's number (0 to MAX_QUESTS -
-- 1, another number is told the caster) to the second word's number (both read
-- as a clamped strtol, Game.toInt) and tells the caster.
-- player:setStorageValue prints the new value at level 3.

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local number = Game.toInt(words[3])
	local value = Game.toInt(words[4])
	if not (number >= 0 and number <= MAX_QUESTS - 1) then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "Invalid quest number.")
		return
	end

	caster:setStorageValue(number, value)
	caster:sendTextMessage(MESSAGE_INFO_DESCR, string.format("Quest value %d set to %d.", number, value))
end
