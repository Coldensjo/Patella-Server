-- items/food.lua: the use of food, what Use's branch FOOD did with UseFood
-- (triggers.cc): adds the food's nutrition to the eating timer and deletes
-- one piece, unless the player is too full.
function onUse(player, item, target, window)
	if not item:exists() then
		debugPrint("UseFood: Given object does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not item:hasFlag(FLAG_FOOD) then
		debugPrint("UseFood: Given object is not food.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if player:isRemoved() then
		debugPrint(string.format("UseFood: Creature %d does not exist.\n", player:getId()))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	-- (int)Obj.getAttribute(NUTRITION) * 12: the attribute as an int.
	local timer = player:getSkillTimer(SKILL_EATING)
	local max = player:getSkillMax(SKILL_EATING)
	local nutrition = bit.tobit(item:getTypeAttribute(ITEMTYPE_ATTRIBUTE_NUTRITION))
	if timer > max - nutrition * 12 then
		Game.throwResult(RETURNVALUE_FEDUP)
	end

	-- TimerValue() + Obj.getAttribute(NUTRITION) * 12: a uint32 sum, passed
	-- as an int.
	local value = bit.tobit(player:getSkillTimer(SKILL_EATING)
		+ item:getTypeAttribute(ITEMTYPE_ATTRIBUTE_NUTRITION) * 12)
	player:setSkillTimer(SKILL_EATING, value, 0, 0, -1)
	Game.deleteObject(item, 1)
	return true
end
