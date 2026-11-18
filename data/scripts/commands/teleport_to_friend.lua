-- commands/teleport_to_friend.lua: Teleport to Friend ("alani sio para"), what
-- TeleportToCreature (the original magic.cc) did: with the command's right,
-- sends the caster to the player online the word names (lib/commands.lua's
-- identifyPlayer: not online, PLAYERNOTONLINE; ambiguous, NAMEAMBIGUOUS): to
-- the first free field within 1 of the player's (jumping; a field of no house
-- or of the house of the player's field), or, where there is none, onto the
-- player's field unless that is in a protection zone (NOROOM then), with the
-- puff where the caster was, the teleport effect where it lands and a line in
-- the log "banish". The caster naming itself only gets the teleport effect.

local commands = require("lib.commands")

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local target = commands.identifyPlayer(caster, words[3])
	if target:getId() == caster:getId() then
		caster:sendMagicEffect(CONST_ME_TELEPORT)
		return
	end

	caster:sendMagicEffect(CONST_ME_POFF)
	local position = target:getPosition()
	local field = Game.searchFreeField(position, 1, Game.getHouseId(position), true)
	if field == nil then
		if Game.isProtectionZone(position) then
			Game.throwResult(RETURNVALUE_NOTENOUGHROOM)
		end

		field = position
	end

	Game.move(0, caster, Game.getMapContainer(field), -1, false, nil)
	field:sendMagicEffect(CONST_ME_TELEPORT)
	Game.log("banish", caster:getName() .. " teleports to " .. target:getName() .. ".\n")
end
