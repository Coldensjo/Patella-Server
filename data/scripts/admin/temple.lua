-- admin/temple.lua: temple <player>. Moves a player online to the start
-- field of the home town (its temple), as Temple Teleport does: the puff
-- before, the blue sparkles after. An offline player's saved position becomes
-- the temple, where the player logs in next.

local admin = require("lib.admin")

function onCommand(words)
	if #words ~= 1 then
		Admin.fail("Usage: temple <player>")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local online = Admin.openCharacter(characterId)
	local start = Admin.getStartPosition()
	if online then
		local player = Admin.getPlayer()
		Admin.closeCharacter()
		player:sendMagicEffect(CONST_ME_POFF)
		Game.move(0, player, Game.getMapContainer(start), -1, false, nil)
		player:sendMagicEffect(CONST_ME_MAGIC_BLUE)
		Admin.reply(name .. " has been moved to the temple " .. admin.positionText(start) .. ".")
	else
		Admin.setSavedPosition(start)
		Admin.closeCharacter()
		Admin.reply(name .. " is offline and will log in at the temple " .. admin.positionText(start) .. ".")
	end
end
