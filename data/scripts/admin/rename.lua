-- admin/rename.lua: rename <player> <new name>. Gives a character a new name
-- (the query manager's renameCharacter, then the game's player index and
-- house owners), which also lifts a namelock. A player online is kicked
-- first, and the rename follows when it has left (Admin.retry, a few
-- seconds). The new name must follow the name rules (admin.checkName) and
-- be free in any case.

local admin = require("lib.admin")

function onCommand(words)
	if #words < 2 then
		Admin.fail("Usage: rename <player> <new name>")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local newName = admin.joinWords(words, 2)
	local why = admin.checkName(newName)
	if why ~= nil then
		Admin.fail(why)
		return
	end

	local online = Admin.openCharacter(characterId)
	local player = Admin.getPlayer()
	Admin.closeCharacter()
	if online then
		if Admin.getTries() == 0 then
			player:sendMagicEffect(CONST_ME_MAGIC_GREEN)
			player:startLogout()
			Admin.reply(name .. " was online and has been kicked.")
		end
		Admin.retry()
		return
	end

	local code = Admin.renameCharacter(name, newName)
	if code == 1 then
		Admin.fail("The query manager has no character " .. name .. ".")
		return
	elseif code == 2 then
		Admin.fail("The name \"" .. newName .. "\" is taken.")
		return
	elseif code == 3 then
		Admin.fail("The name \"" .. newName .. "\" is too long.")
		return
	elseif code ~= 0 then
		Admin.fail("The query manager did not take the new name.")
		return
	end

	Game.log("banish", "The admin port renames " .. name .. " to " .. newName .. ".\n")
	Admin.reply(name .. " is now called " .. newName .. ".")
end
