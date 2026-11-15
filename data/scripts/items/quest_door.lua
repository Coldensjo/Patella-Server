-- items/quest_door.lua: a quest door (flag QUESTDOOR), what UseQuestDoor
-- (triggers.cc) did: a player whose quest value is the door's opens it and
-- steps onto its field; another one gets the door's description.
--
-- The calls are UseQuestDoor's, in its order, with its arguments; its error()
-- texts are logged with debugPrint, its throws raise the same result codes.
-- The use always ends here (true), also where UseQuestDoor returned early.
-- Door.getContainer().isMapContainer() is the container's type number 0
-- (getObjectType().getTypeID() == 0, as ObjectType::isMapContainer tests).

-- A uint32_t attribute as the C++ int it was converted to.
local function int32(value)
	if value >= 2147483648 then
		return value - 4294967296
	end
	return value
end

function onUse(player, item, target, window)
	local door = item
	if not Item.exists(door) or not Item.hasFlag(door, FLAG_QUESTDOOR)
			or Item.getId(Item.getContainer(door)) ~= 0 then
		debugPrint("UseQuestDoor: Given object Door is not a quest door.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if player:isRemoved() then
		debugPrint("UseQuestDoor: Player does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not Item.hasFlag(door, FLAG_UNPASS) then
		Game.throwResult(RETURNVALUE_CANNOTUSETHISOBJECT)
	end

	if player:getStorageValue(int32(Item.getAttribute(door, ITEM_ATTRIBUTE_DOORQUESTNUMBER)))
			~= int32(Item.getAttribute(door, ITEM_ATTRIBUTE_DOORQUESTVALUE)) then
		player:sendTextMessage(MESSAGE_INFO_DESCR, string.format("%s.", Item.getDescription(door)))
		return true
	end

	local doorTarget = ItemType(int32(Item.getTypeAttribute(door, ITEMTYPE_ATTRIBUTE_QUESTDOORTARGET)))
	if doorTarget:getId() == 0 or doorTarget:hasFlag(FLAG_UNPASS) then
		debugPrint(string.format("UseQuestDoor: Target door for door %d not specified or not passable.\n",
				Item.getId(door)))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	Item.transform(door, doorTarget:getId(), 0)
	-- Move(0, pl->GetCreatureObject(), ...): the player stands for its object.
	Game.move(0, player, Item.getContainer(door), -1, false, nil)
	return true
end
