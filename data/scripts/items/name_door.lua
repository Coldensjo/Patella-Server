-- items/name_door.lua: a labelled house door (flag NAMEDOOR), what UseNameDoor
-- (triggers.cc) did: the house owner, a player with the right to open name
-- doors or one the door names opens it (turns it into its target door).
--
-- The calls are UseNameDoor's, in its order, with its arguments; its error()
-- texts are logged with debugPrint, its throws raise the same result codes.
-- The use always ends here (true).

-- A uint32_t attribute as the C++ int it was converted to.
local function int32(value)
	if value >= 2147483648 then
		return value - 4294967296
	end
	return value
end

function onUse(player, item, target, window)
	local door = item
	if not Item.exists(door) then
		debugPrint("UseNameDoor: Given object Door does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not Item.hasFlag(door, FLAG_NAMEDOOR) then
		debugPrint("UseNameDoor: Given object Door is not a labeled door.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not Item.hasFlag(door, FLAG_TEXT) then
		debugPrint("UseNameDoor: Given object Door has no text.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if player:isRemoved() then
		debugPrint("UseNameDoor: Player does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	local position = Item.getPosition(door)
	local houseId = Game.getHouseId(position)
	if houseId == 0 then
		debugPrint(string.format("UseNameDoor: Coordinate [%d,%d,%d] belongs to no house.\n",
				position.x, position.y, position.z))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not player:isHouseOwner(houseId) and not player:hasRight(RIGHT_OPEN_NAMEDOORS)
			and not player:mayOpenDoor(door) then
		Game.throwResult(RETURNVALUE_NOTPOSSIBLE)
	end

	local doorTarget = ItemType(int32(Item.getTypeAttribute(door, ITEMTYPE_ATTRIBUTE_NAMEDOORTARGET)))
	if doorTarget:getId() == 0 then
		debugPrint(string.format("UseNameDoor: Target door for door %d not specified.\n", Item.getId(door)))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if doorTarget:hasFlag(FLAG_UNPASS) then
		Item.clearField(door, nil)
	end

	Item.transform(door, doorTarget:getId(), 0)
	return true
end
