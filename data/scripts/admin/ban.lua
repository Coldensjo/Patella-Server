-- admin/ban.lua: ban <player> <days|permanent> [reason], also given as
-- banaccount. Banishes the account the character belongs to for that many
-- days (1 to 3650) or for good (the query manager's banishAccountFor): no
-- character of it can log in until then. A ban running already ends at the
-- later of the two ends (unban lifts it). The character is kicked when it is
-- online; a line goes into the log "banish".

local admin = require("lib.admin")

function onCommand(words)
	if #words < 2 then
		Admin.fail("Usage: ban <player> <days|permanent> [reason]")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local days, why = admin.parseDays(words[2])
	if days == nil then
		Admin.fail(why)
		return
	end

	local reason = admin.joinWords(words, 3)
	local code = Admin.banishAccount(name, days, reason)
	if code == 1 then
		Admin.fail(name .. " has no account to banish.")
		return
	elseif code == 2 then
		Admin.fail(name .. " belongs to a gamemaster's account, which cannot be banished.")
		return
	elseif code ~= 0 then
		Admin.fail("The query manager did not take the banishment.")
		return
	end

	local online = Admin.openCharacter(characterId)
	local player = Admin.getPlayer()
	Admin.closeCharacter()
	if online then
		player:sendMagicEffect(CONST_ME_MAGIC_GREEN)
		player:startLogout()
	end

	Game.log("banish", string.format("The admin port banishes the account of %s %s%s.\n", name,
			admin.daysText(days), reason ~= "" and (": " .. reason) or ""))
	Admin.reply(string.format("The account of %s is banished %s%s.", name, admin.daysText(days),
			online and " (kicked)" or ""))
end
