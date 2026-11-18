-- commands/temple_teleport.lua: Temple Teleport ("omani para"), what
-- HomeTeleport (the original magic.cc) did: with the command's right, moves
-- the player online the word names (lib/commands.lua's identifyPlayer: not
-- online, PLAYERNOTONLINE; ambiguous, NAMEAMBIGUOUS) to its start field (its
-- temple), with the puff before and the blue sparkles after, tells the caster
-- and writes a line in the log "banish".

local commands = require("lib.commands")

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local target = commands.identifyPlayer(caster, words[2])
	target:sendMagicEffect(CONST_ME_POFF)
	Game.move(0, target, Game.getMapContainer(target:getStartPosition()), -1, false, nil)
	target:sendMagicEffect(CONST_ME_MAGIC_BLUE)
	caster:sendTextMessage(MESSAGE_INFO_DESCR, "Player " .. target:getName() .. " has been moved to the temple.")
	Game.log("banish", caster:getName() .. " teleports " .. target:getName() .. " to the temple.\n")
end
