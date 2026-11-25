-- admin/mute.lua: mute <player> <minutes>. The player may not talk (say,
-- whisper, yell, private messages, the public channels) for that many
-- minutes, as when the game mutes a spammer; 0 lifts a muting. A player
-- online is told; an offline one is muted from its next login on (the game
-- counts the time in its rounds, which start again after a restart).

local admin = require("lib.admin")

function onCommand(words)
	if #words ~= 2 then
		Admin.fail("Usage: mute <player> <minutes> (0 lifts it)")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local minutes = tonumber(words[2])
	if minutes == nil or minutes ~= math.floor(minutes) or minutes < 0 or minutes > 525600 then
		Admin.fail("The minutes must be a whole number from 0 to 525600.")
		return
	end

	local online = Admin.openCharacter(characterId)
	Admin.setMuting(minutes)
	local player = Admin.getPlayer()
	Admin.closeCharacter()

	if minutes == 0 then
		if online then
			player:sendTextMessage(MESSAGE_INFO_DESCR, "You may talk again.")
		end
		Admin.reply(name .. " may talk again.")
		return
	end

	if online then
		player:sendTextMessage(MESSAGE_INFO_DESCR,
				string.format("You have been muted for %d minute%s.", minutes, minutes == 1 and "" or "s"))
	end
	Admin.reply(string.format("%s is muted for %d minute%s%s.", name, minutes, minutes == 1 and "" or "s",
			online and "" or " (offline: from the next login on)"))
end
