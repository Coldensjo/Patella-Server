-- items/key_door.lua: a key (flag KEY) used on a door, what UseKeyDoor
-- (triggers.cc) did: a key whose number matches the door's keyhole turns the
-- door into its target door.
--
-- The calls are UseKeyDoor's, in its order, with its arguments; item is the
-- key, target the door (nil for none, NONE; UseKeyDoor ignored the creature).
-- Its error() texts are logged with debugPrint, its throws raise the same
-- result codes. The use always ends here (true).

-- A uint32_t attribute as the C++ int it was converted to.
local function int32(value)
	if value >= 2147483648 then
		return value - 4294967296
	end
	return value
end

function onUse(player, item, target, window)
	local key = item
	local door = target
	if not Item.exists(key) then
		debugPrint("UseKeyDoor: Given object Key does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not Item.exists(door) then
		debugPrint("UseKeyDoor: Given object Door does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not Item.hasFlag(key, FLAG_KEY) then
		debugPrint("UseKeyDoor: Given object is not a key.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not Item.hasFlag(door, FLAG_KEYDOOR) then
		Game.throwResult(RETURNVALUE_CANNOTUSETHISOBJECT)
	end

	if Item.getAttribute(door, ITEM_ATTRIBUTE_KEYHOLENUMBER) == 0
			or Item.getAttribute(key, ITEM_ATTRIBUTE_KEYNUMBER) == 0
			or Item.getAttribute(door, ITEM_ATTRIBUTE_KEYHOLENUMBER) ~= Item.getAttribute(key, ITEM_ATTRIBUTE_KEYNUMBER) then
		Game.throwResult(RETURNVALUE_NOKEYMATCH)
	end

	local doorTarget = ItemType(int32(Item.getTypeAttribute(door, ITEMTYPE_ATTRIBUTE_KEYDOORTARGET)))
	if doorTarget:getId() == 0 then
		debugPrint(string.format("UseKeyDoor: Target door for door %d not specified.\n", Item.getId(door)))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if doorTarget:hasFlag(FLAG_UNPASS) then
		Item.clearField(door, nil)
	end

	Item.transform(door, doorTarget:getId(), 0)
	return true
end
