-- admin/namelock.lua: namelock <player> [reason]. Locks a character's name
-- (the query manager's setNamelock): it cannot log in until it is renamed
-- (rename lifts the lock). The character is kicked when it is online.

local admin = require("lib.admin")

function onCommand(words)
	if #words < 1 then
		Admin.fail("Usage: namelock <player> [reason]")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local reason = admin.joinWords(words, 2)
	local code = Admin.namelock(name, reason)
	if code == 1 then
		Admin.fail("The query manager has no character " .. name .. ".")
		return
	elseif code == 3 then
		Admin.fail(name .. " is namelocked already.")
		return
	elseif code ~= 0 then
		Admin.fail("The query manager did not take the namelock.")
		return
	end

	local online = Admin.openCharacter(characterId)
	local player = Admin.getPlayer()
	Admin.closeCharacter()
	if online then
		player:sendMagicEffect(CONST_ME_MAGIC_GREEN)
		player:startLogout()
	end

	Game.log("banish", "The admin port namelocks " .. name .. ".\n")
	Admin.reply(name .. " is namelocked: it must be renamed (rename) before it can log in"
			.. (online and "; kicked." or "."))
end
