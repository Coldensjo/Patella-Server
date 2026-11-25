-- admin/removeall.lua: removeall <item> [inventory|depot|everywhere]. Removes
-- an item from every character of the player index, online and offline: from
-- the inventories, the depots or both (everywhere, when not given). The admin
-- port calls onCharacter for one character after the other, some each beat
-- (Admin.runOverCharacters), and onRunEnd at the end.

local admin = require("lib.admin")

local USAGE = "Usage: removeall <item> [inventory|depot|everywhere]"

local PLACES = {
	inventory = true,
	depot = true,
	everywhere = true,
}

-- The item type and the place the words give, or nil and the reason.
local function parse(words)
	if #words < 1 or #words > 2 then
		return nil, USAGE
	end
	local typeId, why = admin.parseItem(words[1], true)
	if typeId == nil then
		return nil, why
	end
	local place = words[2] or "everywhere"
	if not PLACES[place] then
		return nil, USAGE
	end
	return typeId, place
end

function onCommand(words)
	local typeId, place = parse(words)
	if typeId == nil then
		Admin.fail(place)
		return
	end
	Admin.runOverCharacters()
end

function onCharacter(characterId, words)
	local typeId, place = parse(words)
	Admin.openCharacter(characterId)
	local removed = 0
	if place ~= "depot" then
		removed = removed + admin.removeFromInventory(typeId, math.huge)
	end
	if place ~= "inventory" then
		removed = removed + admin.removeFromDepots(typeId, math.huge, nil)
	end
	local name = Admin.getName()
	Admin.closeCharacter()
	if removed > 0 then
		Admin.reply(string.format("%s: %d", name, removed))
	end
	return removed
end

function onRunEnd(words, total, characters)
	local typeId, place = parse(words)
	local where = place == "everywhere" and "inventories and depots" or (place == "depot" and "depots" or "inventories")
	Admin.reply(string.format("Removed %d of %s from the %s of %d characters.", total, admin.itemName(typeId), where,
			characters))
end
