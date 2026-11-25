-- admin/heal.lua: heal <player>. A player online gets full hitpoints and mana
-- and loses poison, burning, energy and drunkenness; the blue sparkles show
-- it.

local admin = require("lib.admin")

function onCommand(words)
	if #words ~= 1 then
		Admin.fail("Usage: heal <player>")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	if not Admin.openCharacter(characterId) then
		Admin.closeCharacter()
		Admin.fail(name .. " is not online.")
		return
	end

	Admin.heal()
	local player = Admin.getPlayer()
	Admin.closeCharacter()
	player:sendMagicEffect(CONST_ME_MAGIC_BLUE)
	Admin.reply(name .. " has been healed.")
end
