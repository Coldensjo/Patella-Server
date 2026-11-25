-- admin/raid.lua: raid <name>. Starts the raid of raids.toml whose key is
-- name (any case) now, as Start Monsterraid does: its waves, its messages and
-- its onRaid hook.

local commands = require("lib.commands")

function onCommand(words)
	if #words ~= 1 then
		Admin.fail("Usage: raid <name>")
		return
	end

	if not Game.startRaid(commands.lower(words[1])) then
		Admin.fail("There is no raid \"" .. words[1] .. "\" (raids.toml names them by key).")
		return
	end
	Admin.reply("Raid " .. words[1] .. " started.")
end
