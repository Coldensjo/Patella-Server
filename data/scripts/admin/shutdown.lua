-- admin/shutdown.lua: shutdown [minutes] [--restart] | shutdown cancel |
-- shutdown status. Brings the game down in that many minutes (5 when not
-- given, 0 to 1440; 0: at once), as the daily reboot does: warnings to the
-- players at 5, 3 and 1 minutes (and every 10 before), no new logins from 5
-- minutes on, then every player logged out, the map saved and the game
-- stopped. --restart: then started again, as after the daily reboot
-- (bin/reboot-daily of the installation). cancel stops a countdown; status
-- tells what is scheduled.

local DEFAULT_MINUTES = 5
local USAGE = "Usage: shutdown [minutes] [--restart] | shutdown cancel | shutdown status"

function onCommand(words)
	local first = words[1]
	if first == "cancel" and #words == 1 then
		if Admin.cancelShutdown() then
			Admin.reply("The shutdown is cancelled.")
		else
			Admin.fail("No shutdown is scheduled.")
		end
		return
	end

	if first == "status" and #words == 1 then
		local seconds, restart = Admin.getShutdown()
		if seconds == nil then
			Admin.reply("No shutdown is scheduled.")
		else
			Admin.reply(string.format("The game goes down in %d:%02d%s.", math.floor(seconds / 60), seconds % 60,
					restart and " and restarts" or ""))
		end
		return
	end

	local minutes = DEFAULT_MINUTES
	local restart = false
	for _, word in ipairs(words) do
		if word == "--restart" then
			restart = true
		else
			local number = tonumber(word)
			if number == nil or number ~= math.floor(number) or number < 0 or number > 1440 then
				Admin.fail(USAGE)
				return
			end
			minutes = number
		end
	end

	Admin.scheduleShutdown(minutes, restart)
	if minutes == 0 then
		Admin.reply("The game goes down now" .. (restart and " and restarts." or "."))
	else
		Admin.reply(string.format("The game goes down in %d minute%s%s; the players are warned.", minutes,
				minutes == 1 and "" or "s", restart and " and restarts" or ""))
	end
end
