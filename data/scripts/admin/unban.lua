-- admin/unban.lua: unban <player>. Ends the banishment of the account the
-- character belongs to at once (the query manager's liftBanishment).

local admin = require("lib.admin")

function onCommand(words)
	if #words ~= 1 then
		Admin.fail("Usage: unban <player>")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local code = Admin.liftBanishment(name)
	if code == 1 then
		Admin.fail(name .. " has no account.")
		return
	elseif code == 2 then
		Admin.fail("The account of " .. name .. " is not banished.")
		return
	elseif code ~= 0 then
		Admin.fail("The query manager did not lift the banishment.")
		return
	end

	Game.log("banish", "The admin port lifts the banishment of the account of " .. name .. ".\n")
	Admin.reply("The banishment of the account of " .. name .. " is lifted.")
end
