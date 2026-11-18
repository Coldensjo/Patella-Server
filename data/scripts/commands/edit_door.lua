-- commands/edit_door.lua: Edit Door ("aleta grav"), what EditNameDoor (the
-- original magic.cc) did: opens the list of names of the name door on the
-- caster's field or, when there is none, of the one on the field in front of
-- the caster (the way it faces), for editing (sendNameDoorWindow checks who
-- may); with no name door on either it prints "No NameDoor found." at level 3
-- and raises RETURNVALUE_NOTPOSSIBLE. It checks no right.

-- The first object with the flag NAMEDOOR on the field, nil for none.
local function findNameDoor(position)
	local door = Game.getFirstObject(position)
	while door ~= nil do
		if door:hasFlag(FLAG_NAMEDOOR) then
			break
		end
		door = door:getNextObject()
	end
	return door
end

function onSay(caster, command, words)
	local position = caster:getPosition()
	local x, y, z = position.x, position.y, position.z
	local door = findNameDoor(Position(x, y, z))
	if door == nil then
		local direction = caster:getDirection()
		if direction == DIRECTION_NORTH then
			y = y - 1
		elseif direction == DIRECTION_EAST then
			x = x + 1
		elseif direction == DIRECTION_SOUTH then
			y = y + 1
		elseif direction == DIRECTION_WEST then
			x = x - 1
		end

		door = findNameDoor(Position(x, y, z))
	end

	if door == nil then
		Game.print(3, "No NameDoor found.\n")
		Game.throwResult(RETURNVALUE_NOTPOSSIBLE)
	end

	caster:sendNameDoorWindow(door)
end
