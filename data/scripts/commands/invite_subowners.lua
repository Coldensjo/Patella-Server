-- commands/invite_subowners.lua: Invite Subowners ("aleta som"), what
-- EditSubowners (the original magic.cc) did: opens the subowner list of the
-- house the caster stands in, for editing (sendHouseWindow checks who may see
-- and change it); outside a house it raises RETURNVALUE_NOTPOSSIBLE. It checks
-- no right.

function onSay(caster, command, words)
	local houseId = Game.getHouseId(caster:getPosition())
	if houseId == 0 then
		Game.throwResult(RETURNVALUE_NOTPOSSIBLE)
	end

	caster:sendHouseWindow(House(houseId), SUBOWNER_LIST)
end
