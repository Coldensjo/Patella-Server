-- admin/removedepot.lua: removedepot <player> <item> [count|all] [town|depot
-- number]. Removes an item from a player's depots (all of them, or the one of
-- the town or number given), all of it or up to count units; online or
-- offline. A depot the player has open is changed where it stands.

local admin = require("lib.admin")

local USAGE = "Usage: removedepot <player> <item> [count|all] [town|depot number]"

function onCommand(words)
	if #words < 2 or #words > 4 then
		Admin.fail(USAGE)
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

	local depot = nil
	local where = "the depots"
	if words[4] ~= nil then
		depot = admin.findDepot(words[4])
		if depot == nil then
			Admin.fail("There is no depot \"" .. words[4] .. "\".")
			return
		end
		where = "the depot of " .. Admin.getDepotTown(depot)
	end

	local online = Admin.openCharacter(characterId)
	local removed = admin.removeFromDepots(typeId, limit, depot)
	Admin.closeCharacter()
	Admin.reply(string.format("Removed %d of %s from %s of %s%s.", removed, admin.itemName(typeId), where, name,
			online and "" or " (offline)"))
end
