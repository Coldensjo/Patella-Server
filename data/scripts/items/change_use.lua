-- items/change_use.lua: the use of a CHANGEUSE object, what Use's branch
-- CHANGEUSE did with UseChangeObject (triggers.cc): changes the object into
-- its CHANGETARGET, first clearing its field or moving what lies on it to a
-- free neighbouring field where the new type blocks it.
function onUse(player, item, target, window)
	if not item:exists() or not item:hasFlag(FLAG_CHANGEUSE) then
		debugPrint("UseChangeObject: Object does not exist or is not a CHANGEUSE object.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if player:isRemoved() then
		debugPrint(string.format("UseChangeObject: Player %d does not exist.\n", player:getId()))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	-- ObjectType NewType = Obj.getAttribute(CHANGETARGET): the uint32 as an
	-- int, made a type once (an invalid one is logged and becomes 0).
	local newType = ItemType(bit.tobit(item:getTypeAttribute(ITEMTYPE_ATTRIBUTE_CHANGETARGET)))
	if not item:hasFlag(FLAG_UNPASS) and newType:getId() ~= 0 and newType:hasFlag(FLAG_UNPASS) then
		item:clearField(item)
	elseif not item:hasFlag(FLAG_UNLAY) and newType:getId() ~= 0 and newType:hasFlag(FLAG_UNLAY) then
		local position = item:getPosition()
		local x, y, z = position.x, position.y, position.z
		if Game.isLayable(Position(x + 1, y, z)) then
			Game.moveAllObjects(item:getNextObject(), Game.getMapContainer(Position(x + 1, y, z)), nil, true)
		elseif Game.isLayable(Position(x, y + 1, z)) then
			Game.moveAllObjects(item:getNextObject(), Game.getMapContainer(Position(x, y + 1, z)), nil, true)
		elseif Game.isLayable(Position(x - 1, y, z)) then
			Game.moveAllObjects(item:getNextObject(), Game.getMapContainer(Position(x - 1, y, z)), nil, true)
		elseif Game.isLayable(Position(x, y - 1, z)) then
			Game.moveAllObjects(item:getNextObject(), Game.getMapContainer(Position(x, y - 1, z)), nil, true)
		end
	end

	item:transform(newType:getId(), 0)
	return true
end
