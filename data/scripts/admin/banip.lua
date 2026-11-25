-- admin/banip.lua: banip <player|address> <days|permanent> [reason]. Blocks
-- an IPv4 address for that many days (1 to 3650) or for good (the query
-- manager's banishIPAddressFor): no character can log in from it. A player
-- named must be online (the game knows its address only then); an address is
-- written as 192.0.2.1. Every player online from that address is kicked.

local admin = require("lib.admin")

function onCommand(words)
	if #words < 2 then
		Admin.fail("Usage: banip <player|address> <days|permanent> [reason]")
		return
	end

	local address = nil
	local who = words[1]
	if string.find(words[1], "^%d+%.%d+%.%d+%.%d+$") then
		address = words[1]
	else
		local characterId, name = admin.findCharacter(words[1])
		if characterId == nil then
			Admin.fail(name)
			return
		end
		local online = Admin.openCharacter(characterId)
		local player = Admin.getPlayer()
		Admin.closeCharacter()
		if not online then
			Admin.fail(name .. " is not online; give the address to block instead.")
			return
		end
		address = Admin.getIPAddress(player)
		who = name .. " (" .. tostring(address) .. ")"
	end

	local days, why = admin.parseDays(words[2])
	if days == nil then
		Admin.fail(why)
		return
	end

	local reason = admin.joinWords(words, 3)
	local code = Admin.banishIPAddress(address, days, reason)
	if code == 1 then
		Admin.fail("\"" .. address .. "\" is no IPv4 address.")
		return
	elseif code ~= 0 then
		Admin.fail("The query manager did not take the banishment.")
		return
	end

	local kicked = 0
	for _, player in ipairs(Game.getPlayers()) do
		if Admin.getIPAddress(player) == address then
			player:sendMagicEffect(CONST_ME_MAGIC_GREEN)
			player:startLogout()
			kicked = kicked + 1
		end
	end

	Game.log("banish", string.format("The admin port blocks the IP address %s %s%s.\n", address,
			admin.daysText(days), reason ~= "" and (": " .. reason) or ""))
	Admin.reply(string.format("The IP address of %s is blocked %s; %d player%s kicked.", who, admin.daysText(days),
			kicked, kicked == 1 and "" or "s"))
end
