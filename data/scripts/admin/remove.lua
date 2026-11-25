-- admin/remove.lua: remove <player> <item> [count|all]. Removes an item from
-- a player's inventory (every body position and the containers in them, depth
-- first), all of it or up to count units (of a stack only what is still
-- missing); online or offline. A container of the type goes with what it
-- holds.

local admin = require("lib.admin")

function onCommand(words)
	if #words < 2 or #words > 3 then
		Admin.fail("Usage: remove <player> <item> [count|all]")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local typeId, why = admin.parseItem(words[2], true)
	if typeId == nil then
		Admin.fail(why)
		return
	end

	local limit = math.huge
	if words[3] ~= nil and words[3] ~= "all" then
		limit, why = admin.parseCount(words[3], admin.MAX_UNITS)
		if limit == nil then
			Admin.fail(why)
			return
		end
	end

	local online = Admin.openCharacter(characterId)
	local removed = admin.removeFromInventory(typeId, limit)
	Admin.closeCharacter()
	Admin.reply(string.format("Removed %d of %s from the inventory of %s%s.", removed, admin.itemName(typeId), name,
			online and "" or " (offline)"))
end
