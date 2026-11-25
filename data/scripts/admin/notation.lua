-- admin/notation.lua: notation <player> <text>. Writes a note into a
-- character's record (the query manager's setNotation), as the rule
-- violation dialog's notation does: no punishment.

local admin = require("lib.admin")

function onCommand(words)
	if #words < 2 then
		Admin.fail("Usage: notation <player> <text>")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local text = admin.joinWords(words, 2)
	local code = Admin.addNotation(name, text)
	if code == 1 then
		Admin.fail("The query manager has no character " .. name .. ".")
		return
	elseif code == 2 then
		Admin.fail(name .. " belongs to a gamemaster's account.")
		return
	elseif code ~= 0 then
		Admin.fail("The query manager did not take the notation.")
		return
	end

	Game.log("banish", "The admin port adds a notation for " .. name .. ": " .. text .. ".\n")
	Admin.reply("Notation for " .. name .. " written.")
end
