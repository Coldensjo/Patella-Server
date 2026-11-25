-- admin/spawnmonster.lua: spawnmonster <race> <x,y,z> [count]. Creates
-- count (1 when not given, at most 50) monsters of a race (by its name, as
-- Summon Wild Creature finds it, or its number) near a field of the map: each
-- on a free field Game.searchSummonField finds within 3 of it. They belong
-- to no spawn and do not come back when killed.

local admin = require("lib.admin")
local commands = require("lib.commands")

-- The most monsters one command makes, and how far from the field they may
-- stand.
local MAX_MONSTERS = 50
local SPAWN_DISTANCE = 3

function onCommand(words)
	if #words < 2 or #words > 3 then
		Admin.fail("Usage: spawnmonster <race> <x,y,z> [count]")
		return
	end

	local race = tonumber(words[1])
	if race == nil then
		race = commands.findRace(words[1])
	elseif race ~= math.floor(race) or race < 1 or race >= MAX_RACES or MonsterType(race) == nil then
		race = 0
	end
	if race == 0 then
		Admin.fail("There is no race \"" .. words[1] .. "\".")
		return
	end

	local position = admin.parsePosition(words[2])
	if position == nil then
		Admin.fail("\"" .. words[2] .. "\" is no field of the map (x,y,z).")
		return
	end

	local count = 1
	if words[3] ~= nil then
		local why
		count, why = admin.parseCount(words[3], MAX_MONSTERS)
		if count == nil then
			Admin.fail(why)
			return
		end
	end

	local made = 0
	for _ = 1, count do
		local field = Game.searchSummonField(position, SPAWN_DISTANCE)
		if field == nil then
			break
		end
		Game.createMonsterOfRace(race, field)
		made = made + 1
	end

	if made == 0 then
		Admin.fail("There is no free field near " .. admin.positionText(position) .. ".")
		return
	end
	position:sendMagicEffect(CONST_ME_MAGIC_BLUE)
	Admin.reply(string.format("Created %d %s near %s.", made, MonsterType(race):name(), admin.positionText(position)))
	if made < count then
		Admin.fail(string.format("No room for the other %d.", count - made))
	end
end
