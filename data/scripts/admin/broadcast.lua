-- admin/broadcast.lua: broadcast <text>. Shows the text (all the words) to
-- every player online as the game's red admin message.

local admin = require("lib.admin")

function onCommand(words)
	if #words < 1 then
		Admin.fail("Usage: broadcast <text>")
		return
	end

	local text = admin.joinWords(words, 1)
	if #text > 255 then
		Admin.fail("A broadcast has at most 255 characters.")
		return
	end

	Admin.broadcast(text)
	Admin.reply(string.format("Sent to %d players online.", #Game.getPlayers()))
end
