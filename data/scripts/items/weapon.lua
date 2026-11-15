-- items/weapon.lua: the use of a sword or an axe on a destructible object,
-- what Use's branch WEAPON && WEAPONTYPE in sword..axe && Obj2 DESTROY did
-- with UseWeapon (triggers.cc): a puff on the target, and one time in three
-- the target is destroyed (emptied, then changed into its DESTROYTARGET).
-- Any other weapon use is not handled here.

-- in(Obj.getAttribute(WEAPONTYPE), WEAPON_SWORD, WEAPON_AXE); in() takes the
-- uint32 as an int, and a value of 2^31 or more is out of range either way.
local function isSwordOrAxe(item)
	local weaponType = item:getTypeAttribute(ITEMTYPE_ATTRIBUTE_WEAPONTYPE)
	return weaponType >= WEAPON_SWORD and weaponType <= WEAPON_AXE
end

function onUse(player, item, target, window)
	-- The branch's own condition (Use checked WEAPON), in its order.
	if not isSwordOrAxe(item) or target == nil or not target:hasFlag(FLAG_DESTROY) then
		return false
	end

	if not item:exists() or not item:hasFlag(FLAG_WEAPON) or not isSwordOrAxe(item) then
		debugPrint("UseWeapon: Given weapon does not exist or is not a weapon.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not target:exists() or not target:hasFlag(FLAG_DESTROY) then
		debugPrint("UseWeapon: Given target does not exist or is not destructible.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if player:isRemoved() then
		debugPrint(string.format("UseWeapon: Player %d does not exist.\n", player:getId()))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	-- getContainer().isMapContainer(): the container's type is 0.
	if Item.getId(target:getContainer()) ~= 0 then
		Game.throwResult(RETURNVALUE_NOTPOSSIBLE)
	end

	target:sendMagicEffect(CONST_ME_POFF)
	if math.random(1, 3) ~= 1 then
		return true
	end

	-- ObjectType Dest = Target.getAttribute(DESTROYTARGET): the uint32 as an
	-- int, made a type once (an invalid one is logged and becomes 0).
	local dest = ItemType(bit.tobit(target:getTypeAttribute(ITEMTYPE_ATTRIBUTE_DESTROYTARGET)))
	if target:hasFlag(FLAG_CONTAINER) then
		local rest = 0
		if dest:getId() ~= 0 and dest:hasFlag(FLAG_CONTAINER) then
			-- (int)Dest.getAttribute(CAPACITY).
			rest = bit.tobit(dest:getAttribute(ITEMTYPE_ATTRIBUTE_CAPACITY))
		end
		target:empty(rest)
	end

	target:transform(dest:getId(), 0)
	return true
end
