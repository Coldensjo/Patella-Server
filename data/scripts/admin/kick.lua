-- admin/kick.lua: kick <player>. Logs a player online out at once, as Kick
-- Player does (the green sparkles, a logout that a fight does not block);
-- writes a line in the log "banish".

local admin = require("lib.admin")

function onCommand(words)
	if #words ~= 1 then
		Admin.fail("Usage: kick <player>")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local online = Admin.openCharacter(characterId)
	local player = Admin.getPlayer()
	Admin.closeCharacter()
	if not online then
		Admin.fail(name .. " is not online.")
		return
	end

	player:sendMagicEffect(CONST_ME_MAGIC_GREEN)
	player:startLogout()
	Game.log("banish", "The admin port kicks " .. name .. ".\n")
	Admin.reply(name .. " has been kicked out of the game.")
end
