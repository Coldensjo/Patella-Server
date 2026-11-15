-- items/chest.lua: a treasure chest (flag CHEST), what UseChest (triggers.cc)
-- did: gives the player a copy of the chest's one object once per quest number.
--
-- The calls are UseChest's, in its order, with its arguments; its error()
-- texts are logged with debugPrint and its throw ERROR raises
-- RETURNVALUE_ERROR. UseChest's try/catch around the moves is a pcall that
-- handles the result codes (Game.getResultOf) and raises any other error
-- again. The use always ends here (true), also where UseChest returned early.

-- A uint32_t attribute as the C++ int it was converted to.
local function int32(value)
	if value >= 2147483648 then
		return value - 4294967296
	end
	return value
end

function onUse(player, item, target, window)
	local chest = item
	if not Item.exists(chest) then
		debugPrint("UseChest: Given object does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not Item.hasFlag(chest, FLAG_CHEST) then
		debugPrint("UseChest: Given object is not a treasure chest.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	local position = Item.getPosition(chest)
	local number = int32(Item.getAttribute(chest, ITEM_ATTRIBUTE_CHESTQUESTNUMBER))
	if number < 1 or number > MAX_QUESTS - 1 then
		debugPrint(string.format("UseChest: Invalid number %d on treasure chest at position [%d,%d,%d].\n",
				number, position.x, position.y, position.z))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if Item.countObjects(chest) ~= 1 then
		debugPrint(string.format("UseChest: Treasure chest at position [%d,%d,%d] does not hold exactly one object.\n",
				position.x, position.y, position.z))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	local treasure = Item.getFirstObject(chest)
	if Item.hasFlag(treasure, FLAG_UNMOVE) or not Item.hasFlag(treasure, FLAG_TAKE) then
		-- UseChest passed no coordinates for the three %d (undefined in C++);
		-- the text is logged with the three %d as they stand.
		debugPrint("UseChest: Treasure at position [%d,%d,%d] cannot be taken.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	local creatureId = player:getId()
	if player:isRemoved() then
		debugPrint(string.format("UseChest: Player %d does not exist.\n", creatureId))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if player:getStorageValue(number) ~= 0 then
		Game.print(3, "Treasure chest is already known.\n")
		player:sendTextMessage(MESSAGE_INFO_DESCR,
				string.format("The %s is empty.", ItemType(Item.getId(chest)):getName(-1)))
		return true
	end

	if not player:hasRight(RIGHT_UNLIMITED_CAPACITY) then
		local weight = Item.getWeight(treasure)
		if weight + Game.getInventoryWeight(creatureId) > player:getSkillValue(SKILL_CARRYSTRENGTH) * 100
				or player:hasRight(RIGHT_ZERO_CAPACITY) then
			local name = Item.getName(treasure)
			local pronoun = "it is"
			if Item.hasFlag(treasure, FLAG_CUMULATIVE) and Item.getAttribute(treasure, ITEM_ATTRIBUTE_AMOUNT) > 1 then
				pronoun = "they are"
			end
			player:sendTextMessage(MESSAGE_INFO_DESCR,
					string.format("You have found %s. Weighing %d.%02d oz %s too heavy.",
							name, math.floor(weight / 100), weight % 100, pronoun))
			return true
		end
	end

	treasure = Game.copyObject(Item.getContainer(treasure), treasure)
	local first = true
	local pos = 0
	local invert = Item.hasFlag(treasure, FLAG_MOVEMENTEVENT)

	-- The try block of one body position.
	local function tryPosition()
		local obj = Game.getBodyObject(creatureId, pos)
		if first ~= invert and obj == nil then
			Game.move(creatureId, treasure, Game.getBodyContainer(creatureId, pos), -1, true, nil)
		elseif first == invert and obj ~= nil and Item.hasFlag(obj, FLAG_CONTAINER) then
			Game.move(creatureId, treasure, obj, -1, true, nil)
		else
			Game.throwResult(RETURNVALUE_NOTENOUGHROOM)
		end
	end

	while true do
		pos = pos + 1
		local ok, err = pcall(tryPosition)
		if ok then
			break
		end

		local result = Game.getResultOf(err)
		if result == nil then
			error(err, 0)
		end

		if result == RETURNVALUE_NOTENOUGHCAPACITY then
			debugPrint(string.format("UseChest: Treasure at position [%d,%d,%d] is too heavy after all.\n",
					position.x, position.y, position.z))
		end

		if pos >= LAST_BODY_POSITION then
			if first then
				first = false
				pos = 0
			else
				local name = Item.getName(treasure)
				local pronoun = "it"
				if Item.hasFlag(treasure, FLAG_CUMULATIVE)
						and Item.getAttribute(treasure, ITEM_ATTRIBUTE_AMOUNT) > 1 then
					pronoun = "them"
				end
				player:sendTextMessage(MESSAGE_INFO_DESCR,
						string.format("You have found %s, but you have no room to take %s.\n", name, pronoun))
				Game.deleteObject(treasure, -1)
				return true
			end
		end
	end

	player:sendTextMessage(MESSAGE_INFO_DESCR, string.format("You have found %s.\n", Item.getName(treasure)))
	player:setStorageValue(number, 1)
	return true
end
