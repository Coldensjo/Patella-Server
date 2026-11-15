-- items/announcer.lua: the use of an INFORMATION object, what Use's branch
-- INFORMATION did with UseAnnouncer (triggers.cc): tells the date and time,
-- the time or the blessings received, or opens the spellbook's text.
function onUse(player, item, target, window)
	if not item:exists() then
		debugPrint("UseAnnouncer: Given object does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not item:hasFlag(FLAG_INFORMATION) then
		debugPrint("UseAnnouncer: Given object gives no information.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if player:isRemoved() then
		debugPrint(string.format("UseAnnouncer: Creature %d does not exist.\n", player:getId()))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	local informationType = item:getTypeAttribute(ITEMTYPE_ATTRIBUTE_INFORMATIONTYPE)
	if informationType == INFORMATION_DATE then
		local year, cycle, day = Game.getDate()
		local hour, minute = Game.getTime()
		player:sendTextMessage(MESSAGE_INFO_DESCR, string.format(
			"It is the %dth day of the %dth cycle in the year %d. The time is %d:%.2d.",
			day, cycle, year, hour, minute))
	elseif informationType == INFORMATION_TIME then
		local hour, minute = Game.getTime()
		player:sendTextMessage(MESSAGE_INFO_DESCR, string.format("The time is %d:%.2d.", hour, minute))
	elseif informationType == INFORMATION_BLESSINGS then
		local help = "Received blessings:"
		local blessings = 0
		if player:getStorageValue(FIRST_BLESSING_QUEST) ~= 0 then
			help = help .. "\nWisdom of Solitude"
			blessings = blessings + 1
		end
		if player:getStorageValue(FIRST_BLESSING_QUEST + 1) ~= 0 then
			help = help .. "\nSpark of the Phoenix"
			blessings = blessings + 1
		end
		if player:getStorageValue(FIRST_BLESSING_QUEST + 2) ~= 0 then
			help = help .. "\nFire of the Suns"
			blessings = blessings + 1
		end
		if player:getStorageValue(FIRST_BLESSING_QUEST + 3) ~= 0 then
			help = help .. "\nSpiritual Shielding"
			blessings = blessings + 1
		end
		if player:getStorageValue(LAST_BLESSING_QUEST) ~= 0 then
			help = help .. "\nEmbrace of Tibia"
			blessings = blessings + 1
		end
		if blessings == 0 then
			help = "No blessings received."
		end
		player:sendTextMessage(MESSAGE_INFO_DESCR, help)
	elseif informationType == INFORMATION_SPELLBOOK then
		player:sendEditText(item)
	else
		-- %d of a uint32: the attribute as an int.
		debugPrint(string.format("UseAnnouncer: Invalid information type %d.\n",
			bit.tobit(item:getTypeAttribute(ITEMTYPE_ATTRIBUTE_INFORMATIONTYPE))))
	end
	return true
end
