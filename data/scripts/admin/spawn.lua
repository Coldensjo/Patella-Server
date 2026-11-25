-- admin/spawn.lua: spawn <x,y,z> <item> [count]. Creates count (1 when not
-- given) of an item type (by number or name; any type, also one that cannot be
-- carried) on a field of the map: a cumulative type in stacks of MAX_AMOUNT
-- (a stack on top of the field takes what fits), another one object by object.
-- Stops at the first object the field refuses (no room) and tells how many it
-- made; the green sparkles on the field.

local admin = require("lib.admin")

local USAGE = "Usage: spawn <x,y,z> <item> [count]"

function onCommand(words)
	if #words < 2 or #words > 3 then
		Admin.fail(USAGE)
		return
	end

	local position = admin.parsePosition(words[1])
	if position == nil then
		Admin.fail("\"" .. words[1] .. "\" is no field of the map (x,y,z).")
		return
	end

	local typeId, why = admin.parseItem(words[2], false)
	if typeId == nil then
		Admin.fail(why)
		return
	end

	local count = 1
	if words[3] ~= nil then
		count, why = admin.parseCount(words[3], admin.MAX_UNITS)
		if count == nil then
			Admin.fail(why)
			return
		end
	end

	local values
	values, why = admin.units(typeId, count)
	if values == nil then
		Admin.fail(why)
		return
	end

	local container = Game.getMapContainer(position)
	local made = 0
	for _, value in ipairs(values) do
		if Admin.create(container, typeId, value) == nil then
			break
		end
		made = made + math.max(value, 1)
	end

	if made == 0 then
		Admin.fail("There is no room for " .. admin.itemName(typeId) .. " on " .. admin.positionText(position) .. ".")
		return
	end

	position:sendMagicEffect(CONST_ME_MAGIC_GREEN)
	Admin.reply(string.format("Created %d of %s on %s.", made, admin.itemName(typeId), admin.positionText(position)))
	if made < count then
		Admin.fail(string.format("No room for the other %d.", count - made))
	end
end
