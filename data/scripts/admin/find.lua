-- admin/find.lua: find <item> [inventory|depot|everywhere]. Counts an item in
-- every character of the player index, online and offline, in the
-- inventories, the depots or both (everywhere, when not given), and names
-- each character that has some. Nothing is changed. A run like removeall's.

local admin = require("lib.admin")

local USAGE = "Usage: find <item> [inventory|depot|everywhere]"

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
	local typeId, why = admin.parseItem(words[1], false)
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
	local carried = 0
	local stored = 0
	if place ~= "depot" then
		carried = admin.countInInventory(typeId)
	end
	if place ~= "inventory" then
		stored = admin.countInDepots(typeId)
	end
	local name = Admin.getName()
	Admin.closeCharacter()
	if carried + stored > 0 then
		Admin.reply(string.format("%s: %d (inventory %d, depots %d)", name, carried + stored, carried, stored))
	end
	return carried + stored
end

function onRunEnd(words, total, characters)
	local typeId = parse(words)
	Admin.reply(string.format("%d of %s in all, %d characters looked at.", total, admin.itemName(typeId),
			characters))
end
