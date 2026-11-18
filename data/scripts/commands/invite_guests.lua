-- commands/invite_guests.lua: Invite Guests ("aleta sio"), what EditGuests
-- (the original magic.cc) did: opens the guest list of the house the caster
-- stands in, for editing (sendHouseWindow checks who may see and change it);
-- outside a house it raises RETURNVALUE_NOTPOSSIBLE. It checks no right.

function onSay(caster, command, words)
	local houseId = Game.getHouseId(caster:getPosition())
	if houseId == 0 then
		Game.throwResult(RETURNVALUE_NOTPOSSIBLE)
	end

	caster:sendHouseWindow(House(houseId), GUEST_LIST)
end
