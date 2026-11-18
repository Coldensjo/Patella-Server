-- commands/kick_player.lua: Kick Player ("omana para"), what KickPlayer (the
-- original magic.cc) did: with the command's right, logs the player online the
-- word names (lib/commands.lua's identifyPlayer: not online, PLAYERNOTONLINE;
-- ambiguous, NAMEAMBIGUOUS) out at once, with the green sparkles on it, tells
-- the caster and writes a line in the log "banish".

local commands = require("lib.commands")

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local target = commands.identifyPlayer(caster, words[2])
	target:sendMagicEffect(CONST_ME_MAGIC_GREEN)
	target:startLogout()
	caster:sendTextMessage(MESSAGE_INFO_DESCR, "Player " .. target:getName() .. " kicked out of the game.")
	Game.log("banish", caster:getName() .. " kicks " .. target:getName() .. ".\n")
end
