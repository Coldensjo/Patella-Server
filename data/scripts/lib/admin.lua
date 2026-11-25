-- lib/admin.lua: the helpers the admin port's commands share (admin/<name>.lua;
-- scripting.hh, section "The admin port"). A module:
--	local admin = require("lib.admin")
-- The table it returns holds the functions below and is read-only (rule 9):
-- reading a name it does not have is an error ("unknown name 'admin.x'"), and
-- so is writing one.
--   admin.parseItem(word, carried)   the item type a word names, or nil and
--                                    why not.
--   admin.itemName(typeId)           the type's name.
--   admin.parseCount(word, maximum)  a count of 1 to maximum, or nil and why
--                                    not.
--   admin.parsePosition(word)        the Position of "x,y,z", or nil.
--   admin.findCharacter(word)        a character's ID and name, or nil and
--                                    why not.
--   admin.positionText(position)     "[x,y,z]".
--   admin.removeFrom(container, typeId, limit)
--                                    removes up to limit of a type below a
--                                    container; gives how many.
--   admin.removeFromInventory(typeId, limit)
--   admin.removeFromDepots(typeId, limit, depot)
--                                    the same for the open character's
--                                    inventory, its depots or one depot.
--   admin.findDepot(word)            the depot number a town name or a number
--                                    names, or nil.
--   admin.homeDepot(start)           the depot of the town nearest a temple.
--   admin.moveOnline(player, position)
--                                    moves a player online to the first free
--                                    field near position; false if none.
--   admin.units(typeId, count)       the values of the objects count of a
--                                    type are made of.
--   admin.countFrom(container, typeId), admin.countInInventory(typeId),
--   admin.countInDepots(typeId)      how many units of a type are there.
--   admin.joinWords(words, first)    the words from first on, with spaces.
--   admin.parseDays(word)            the days of a banishment (0: for good),
--                                    or nil and why not.
--   admin.daysText(days)             "for 7 days" or "for good".
--   admin.checkName(name)            nil for a valid character name, else
--                                    why not.

local commands = require("lib.commands")

local floor = math.floor
local format = string.format

-- SearchFreeField's house number for "any field".
local ANY_HOUSE = 0xFFFF

-- The most objects of a type that is not cumulative one command makes, and
-- the most of a cumulative type (in stacks of MAX_AMOUNT).
local MAX_OBJECTS = 100
local MAX_UNITS = 10000

-- The depots a player has: 0 to MAX_DEPOT.
local MAX_DEPOT = 8

-- parseItem(word, carried) -> typeId, or nil and the reason: word is a type
-- number above the creatures' that exists, or a name (ItemType.fromName, the
-- first exact name in any case, else the first name that holds the word).
-- carried: only a type a player can carry (FLAG_TAKE).
local function parseItem(word, carried)
	if word == nil then
		return nil, "No item given."
	end

	local typeId
	local number = tonumber(word)
	if number ~= nil then
		if number ~= floor(number) or number <= TYPEID_CREATURE or not ItemType.exists(number) then
			return nil, format("There is no item type %s.", word)
		end
		typeId = number
	else
		typeId = ItemType.fromName(word, carried)
		if typeId == 0 then
			return nil, format("There is no item called \"%s\".", word)
		end
	end

	if carried and not ItemType(typeId):hasFlag(FLAG_TAKE) then
		return nil, format("%s (%d) cannot be carried.", ItemType(typeId):getName(-1), typeId)
	end
	return typeId
end

-- itemName(typeId) -> "<name> (<number>)".
local function itemName(typeId)
	return format("%s (%d)", ItemType(typeId):getName(-1), typeId)
end

-- parseCount(word, maximum) -> a whole number from 1 to maximum, or nil and
-- the reason.
local function parseCount(word, maximum)
	local number = tonumber(word)
	if number == nil or number ~= floor(number) or number < 1 or number > maximum then
		return nil, format("The count must be a whole number from 1 to %d.", maximum)
	end
	return number
end

-- parsePosition(word) -> the Position of "x,y,z" or "[x,y,z]" on the map, or
-- nil.
local function parsePosition(word)
	if word == nil then
		return nil
	end
	local x, y, z = commands.scanCoordinates(word)
	if x == nil then
		return nil
	end
	local position = Position(x, y, z)
	if not Game.isOnMap(position) then
		return nil
	end
	return position
end

-- findCharacter(word) -> the character's ID and its name as written, or nil
-- and the reason (a name of no character).
local function findCharacter(word)
	if word == nil then
		return nil, "No player given."
	end
	local characterId, name = Admin.findCharacter(word)
	if characterId == nil then
		return nil, format("There is no character called \"%s\".", word)
	end
	return characterId, name
end

-- positionText(position) -> "[x,y,z]".
local function positionText(position)
	return format("[%d,%d,%d]", position.x, position.y, position.z)
end

-- How many units an object is: its amount for a cumulative one, else 1.
local function unitsOf(item)
	if item:hasFlag(FLAG_CUMULATIVE) then
		return item:getAttribute(ITEM_ATTRIBUTE_AMOUNT)
	end
	return 1
end

-- removeFrom(container, typeId, limit) -> how many units it removed: every
-- object of the type below container, depth first, until limit units are
-- gone (of a cumulative object only what is still missing). An object of the
-- type that is a container goes with what it holds.
local function removeFrom(container, typeId, limit)
	local removed = 0
	local item = container:getFirstObject()
	while item ~= nil and removed < limit do
		local nextItem = item:getNextObject()
		if item:getId() == typeId then
			local units = unitsOf(item)
			if units > limit - removed then
				Game.deleteObject(item, limit - removed)
				removed = limit
			else
				Game.deleteObject(item, -1)
				removed = removed + units
			end
		elseif item:hasFlag(FLAG_CONTAINER) then
			removed = removed + removeFrom(item, typeId, limit - removed)
		end
		item = nextItem
	end
	return removed
end

-- removeFromInventory(typeId, limit) -> how many units it removed from the
-- open character's inventory (every body position).
local function removeFromInventory(typeId, limit)
	local removed = 0
	for slot = CONST_SLOT_HEAD, CONST_SLOT_AMMO do
		if removed >= limit then
			break
		end
		removed = removed + removeFrom(Admin.getInventoryContainer(slot), typeId, limit - removed)
	end
	return removed
end

-- removeFromDepots(typeId, limit, depot) -> how many units it removed from
-- the open character's depots (depot nil) or from the depot numbered depot.
local function removeFromDepots(typeId, limit, depot)
	local first = depot or 0
	local last = depot or MAX_DEPOT
	local removed = 0
	for number = first, last do
		if removed >= limit then
			break
		end
		removed = removed + removeFrom(Admin.getDepotContainer(number), typeId, limit - removed)
	end
	return removed
end

-- findDepot(word) -> the depot number of a town's name (any case) or of a
-- number 0 to 8 that a depot has, or nil.
local function findDepot(word)
	local number = tonumber(word)
	if number ~= nil then
		if number == floor(number) and number >= 0 and number <= MAX_DEPOT and Admin.getDepotTown(number) ~= nil then
			return number
		end
		return nil
	end
	local wanted = commands.lower(word)
	for depot = 0, MAX_DEPOT do
		local town = Admin.getDepotTown(depot)
		if town ~= nil and commands.lower(town) == wanted then
			return depot
		end
	end
	return nil
end

-- homeDepot(start) -> the depot of the town whose mark (map.toml, the mark
-- named as the depot's town) is nearest the temple start, on any floor by x
-- and y; the first such depot at the same distance; 0 when no town has a
-- mark.
local function homeDepot(start)
	local best = 0
	local bestDistance = nil
	for depot = 0, MAX_DEPOT do
		local town = Admin.getDepotTown(depot)
		local mark = town and Game.getMarkPosition(town)
		if mark ~= nil then
			local distance = math.abs(mark.x - start.x) + math.abs(mark.y - start.y)
			if bestDistance == nil or distance < bestDistance then
				best = depot
				bestDistance = distance
			end
		end
	end
	return best
end

-- moveOnline(player, position) -> the field it moved the player to, or nil
-- when there is no free field within 1 of position: the puff where it stood,
-- the move, the teleport effect where it stands then.
local function moveOnline(player, position)
	local field = Game.searchFreeField(position, 1, ANY_HOUSE, true)
	if field == nil then
		return nil
	end
	player:sendMagicEffect(CONST_ME_POFF)
	Game.move(0, player, Game.getMapContainer(field), -1, false, nil)
	field:sendMagicEffect(CONST_ME_TELEPORT)
	return field
end

-- units(typeId, count) -> the array of the values of the objects that make
-- count of the type: for a cumulative type its amounts, MAX_AMOUNT each and
-- the rest (count up to MAX_UNITS); for another type count times 0 (count up
-- to MAX_OBJECTS). Or nil and the reason for a count above that.
local function units(typeId, count)
	local values = {}
	if ItemType(typeId):hasFlag(FLAG_CUMULATIVE) then
		if count > MAX_UNITS then
			return nil, format("At most %d of a stackable item at once.", MAX_UNITS)
		end
		while count > 0 do
			local amount = math.min(count, MAX_AMOUNT)
			values[#values + 1] = amount
			count = count - amount
		end
	else
		if count > MAX_OBJECTS then
			return nil, format("At most %d of an item that does not stack at once.", MAX_OBJECTS)
		end
		for i = 1, count do
			values[i] = 0
		end
	end
	return values
end

-- countFrom(container, typeId) -> how many units of the type are below
-- container (depth first, in containers of the type too).
local function countFrom(container, typeId)
	local count = 0
	local item = container:getFirstObject()
	while item ~= nil do
		if item:getId() == typeId then
			count = count + unitsOf(item)
		end
		if item:hasFlag(FLAG_CONTAINER) then
			count = count + countFrom(item, typeId)
		end
		item = item:getNextObject()
	end
	return count
end

-- countInInventory(typeId), countInDepots(typeId) -> the units of the type
-- the open character carries, or has in all its depots.
local function countInInventory(typeId)
	local count = 0
	for slot = CONST_SLOT_HEAD, CONST_SLOT_AMMO do
		count = count + countFrom(Admin.getInventoryContainer(slot), typeId)
	end
	return count
end

local function countInDepots(typeId)
	local count = 0
	for depot = 0, MAX_DEPOT do
		count = count + countFrom(Admin.getDepotContainer(depot), typeId)
	end
	return count
end

-- joinWords(words, first) -> the words from first on, separated by spaces
-- ("" when there are none).
local function joinWords(words, first)
	return table.concat(words, " ", first)
end

-- parseDays(word) -> the days of a banishment: a whole number from 1 to
-- 3650, or 0 for "permanent" (also "perm", "forever"); nil and the reason
-- otherwise.
local function parseDays(word)
	if word == nil then
		return nil, "No length given: a number of days, or permanent."
	end
	local lowered = commands.lower(word)
	if lowered == "permanent" or lowered == "perm" or lowered == "forever" then
		return 0
	end
	local days = tonumber(word)
	if days == nil or days ~= floor(days) or days < 1 or days > 3650 then
		return nil, "The length must be a number of days from 1 to 3650, or permanent."
	end
	return days
end

-- daysText(days) -> "for 7 days", "for 1 day" or "for good" (0).
local function daysText(days)
	if days == 0 then
		return "for good"
	end
	return format("for %d day%s", days, days == 1 and "" or "s")
end

-- checkName(name) -> nil when name is a character name the game takes, else
-- the reason: 2 to 29 characters, letters with single spaces, apostrophes
-- or hyphens between them, beginning with a capital letter.
local function checkName(name)
	if #name < 2 or #name > 29 then
		return "A name has 2 to 29 characters."
	end
	if not string.find(name, "^[A-Z]") then
		return "A name begins with a capital letter."
	end
	if string.find(name, "[^A-Za-z '%-]") then
		return "A name has only letters, spaces, apostrophes and hyphens."
	end
	if string.find(name, "[ '%-][ '%-]") or string.find(name, "[ '%-]$") then
		return "Spaces, apostrophes and hyphens go between letters, one at a time."
	end
	return nil
end

local fields = {
	countFrom = countFrom,
	countInInventory = countInInventory,
	countInDepots = countInDepots,
	joinWords = joinWords,
	parseDays = parseDays,
	daysText = daysText,
	checkName = checkName,
	parseItem = parseItem,
	itemName = itemName,
	parseCount = parseCount,
	parsePosition = parsePosition,
	findCharacter = findCharacter,
	positionText = positionText,
	removeFrom = removeFrom,
	removeFromInventory = removeFromInventory,
	removeFromDepots = removeFromDepots,
	findDepot = findDepot,
	homeDepot = homeDepot,
	moveOnline = moveOnline,
	units = units,
	MAX_UNITS = MAX_UNITS,
}

-- admin: the fields, read-only, an unknown name an error.
local function keyText(key)
	if type(key) == "string" then
		return key
	end
	return "<" .. type(key) .. ">"
end

return setmetatable({}, {
	__index = function(_, key)
		local value = fields[key]
		if value == nil then
			error(format("unknown name 'admin.%s'", keyText(key)), 2)
		end
		return value
	end,
	__newindex = function(_, key)
		error(format("admin.%s is read-only", keyText(key)), 2)
	end,
	__metatable = false,
})
