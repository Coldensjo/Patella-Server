-- commands/online_list.lua: Online List ("aliva mas"), ours, not the
-- original's: with the command's right, tells the caster how many players are
-- online and their names (Game.getPlayers: every player online, gamemasters
-- too), in one message or, for a long list, in several of at most
-- MESSAGE_LENGTH bytes each.

-- The longest text of one message.
local MESSAGE_LENGTH = 250

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local players = Game.getPlayers()
	local text
	if #players == 1 then
		text = "1 player online: "
	else
		text = string.format("%d players online: ", #players)
	end

	for i, player in ipairs(players) do
		local name = player:getName()
		local separator = i < #players and ", " or "."
		if #text + #name + #separator > MESSAGE_LENGTH then
			caster:sendTextMessage(MESSAGE_INFO_DESCR, text)
			text = ""
		end
		text = text .. name .. separator
	end
	caster:sendTextMessage(MESSAGE_INFO_DESCR, text)
end
