-- commands/retrieve_friend.lua: Retrieve Friend ("alliber sio para"), what
-- TeleportPlayerToMe (the original magic.cc) did: with the command's right,
-- brings the player online the word names (lib/commands.lua's identifyPlayer:
-- not online, PLAYERNOTONLINE; ambiguous, NAMEAMBIGUOUS) to the first free
-- field within 1 of the caster's (walking, not jumping; a field of no house or
-- of the house of the caster's field; none: NOROOM), with the puff where the
-- player was and the teleport effect where it lands.

local commands = require("lib.commands")

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local target = commands.identifyPlayer(caster, words[3])
	target:sendMagicEffect(CONST_ME_POFF)
	local position = caster:getPosition()
	local field = Game.searchFreeField(position, 1, Game.getHouseId(position), false)
	if field == nil then
		Game.throwResult(RETURNVALUE_NOTENOUGHROOM)
	end

	Game.move(0, target, Game.getMapContainer(field), -1, false, nil)
	field:sendMagicEffect(CONST_ME_TELEPORT)
end
