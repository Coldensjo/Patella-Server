-- commands/kick_guest.lua: Kick Guest ("alana sio para"), what KickGuest (the
-- original magic.cc) did: sends the player online the word names
-- (lib/commands.lua's identifyPlayer: not online, PLAYERNOTONLINE; ambiguous,
-- NAMEAMBIGUOUS) out of the house it stands in (house:kickPlayer: its checks
-- of the caster's and the guest's rights, the move to the exit); a player
-- outside every house: RETURNVALUE_NOTPOSSIBLE (NOTACCESSIBLE). It has no
-- right of its own: who may kick is the house's matter.

local commands = require("lib.commands")

function onSay(caster, command, words)
	local guest = commands.identifyPlayer(caster, words[3])
	local houseId = Game.getHouseId(guest:getPosition())
	if houseId == 0 then
		Game.throwResult(RETURNVALUE_NOTPOSSIBLE)
	end

	House(houseId):kickPlayer(caster, guest)
end
