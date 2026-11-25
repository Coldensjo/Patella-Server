-- admin/sethome.lua: sethome <player> <town|x,y,z>. Makes a town (one of
-- admin.toml's [[town]], by name in any case) or a field of the map the
-- player's home: its temple, where it comes back after a death and where
-- temple sends it. Online or offline. The depot of the home town follows the
-- temple (give's depot is the town nearest it).

local admin = require("lib.admin")

function onCommand(words)
	if #words ~= 2 then
		Admin.fail("Usage: sethome <player> <town|x,y,z>")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local temple = admin.parsePosition(words[2])
	local where
	if temple ~= nil then
		where = admin.positionText(temple)
	else
		temple = Admin.getTownTemple(words[2])
		if temple == nil then
			Admin.fail("There is no town \"" .. words[2] .. "\". Towns: " .. table.concat(Admin.getTowns(), ", ") .. ".")
			return
		end
		where = words[2] .. " " .. admin.positionText(temple)
	end

	local online = Admin.openCharacter(characterId)
	Admin.setStartPosition(temple)
	Admin.closeCharacter()
	Admin.reply(string.format("The home of %s is %s%s.", name, where, online and "" or " (offline)"))
end
