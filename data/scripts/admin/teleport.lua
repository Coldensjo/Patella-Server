-- admin/teleport.lua: teleport <player> <x,y,z | player>. Moves a player to a
-- field of the map, or to where another character is (online: where it
-- stands; offline: where it logs in next). A player online goes to the first
-- free field within 1 of it (the puff before, the teleport effect after); an
-- offline player's saved position becomes the field itself.

local admin = require("lib.admin")

function onCommand(words)
	if #words ~= 2 then
		Admin.fail("Usage: teleport <player> <x,y,z | player>")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local destination = admin.parsePosition(words[2])
	local destinationText
	if destination == nil then
		local targetId, targetName = Admin.findCharacter(words[2])
		if targetId == nil then
			Admin.fail("\"" .. words[2] .. "\" is neither a field of the map (x,y,z) nor a character.")
			return
		end
		if targetId == characterId then
			Admin.fail("A player cannot be teleported to itself.")
			return
		end
		Admin.openCharacter(targetId)
		destination = Admin.getPosition()
		Admin.closeCharacter()
		destinationText = targetName .. " " .. admin.positionText(destination)
	else
		destinationText = admin.positionText(destination)
	end

	if Admin.openCharacter(characterId) then
		local player = Admin.getPlayer()
		Admin.closeCharacter()
		local field = admin.moveOnline(player, destination)
		if field == nil then
			Admin.fail("There is no free field near " .. destinationText .. ".")
			return
		end
		Admin.reply(name .. " has been moved to " .. destinationText .. ".")
	else
		Admin.setSavedPosition(destination)
		Admin.closeCharacter()
		Admin.reply(name .. " is offline and will log in at " .. destinationText .. ".")
	end
end
