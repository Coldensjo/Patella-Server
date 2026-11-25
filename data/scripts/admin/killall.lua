-- admin/killall.lua: killall <x,y,z> [radius]. Kills every monster within
-- radius (0 to 7, 2 when not given; the circles of circles.toml) of a field
-- of the map, as Kill All Creatures does around a gamemaster: they die as if
-- slain (their corpses, their loot, their hooks). Players and NPCs are not
-- touched.

local admin = require("lib.admin")

local DEFAULT_RADIUS = 2
local MAX_RADIUS = 7

function onCommand(words)
	if #words < 1 or #words > 2 then
		Admin.fail("Usage: killall <x,y,z> [radius]")
		return
	end

	local center = admin.parsePosition(words[1])
	if center == nil then
		Admin.fail("\"" .. words[1] .. "\" is no field of the map (x,y,z).")
		return
	end

	local radius = DEFAULT_RADIUS
	if words[2] ~= nil then
		radius = tonumber(words[2])
		if radius == nil or radius ~= math.floor(radius) or radius < 0 or radius > MAX_RADIUS then
			Admin.fail(string.format("The radius must be a whole number from 0 to %d.", MAX_RADIUS))
			return
		end
	end

	local killed = 0
	for ring = 0, radius do
		for _, offset in ipairs(Magic.getCircle(ring)) do
			local field = Position(center.x + offset.x, center.y + offset.y, center.z)
			if Game.isOnMap(field) then
				-- The next object is taken before the current one is handled
				-- (a death changes the field).
				local object = Game.getFirstObject(field)
				while object ~= nil do
					local current = object
					object = object:getNextObject()
					if current:isCreature() then
						local creature = current:getCreature()
						if creature ~= nil and creature:isMonster() then
							creature:setHealth(0)
							creature:die()
							killed = killed + 1
						end
					end
				end
			end
		end
	end

	center:sendMagicEffect(CONST_ME_MORTAREA)
	Admin.reply(string.format("Killed %d monster%s within %d of %s.", killed, killed == 1 and "" or "s", radius,
			admin.positionText(center)))
end
