-- items/liquid_container.lua: the use of a liquid container, what Use's
-- branch LIQUIDCONTAINER did with UseLiquidContainer (triggers.cc) and
-- DrinkPotion (magic.cc): fills it from a source, pours it into an empty
-- container, lets the player drink it, or pours it out as a pool.

-- DrinkPotion: a mana or life fluid refreshes mana or heals, and the
-- container is emptied.
local function drinkPotion(player, item)
	if player:isRemoved() then
		debugPrint("DrinkPotion: Creature does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not item:exists() then
		debugPrint("DrinkPotion: Given object does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not item:hasFlag(FLAG_LIQUIDCONTAINER) then
		debugPrint("DrinkPotion: Given object is not a liquid container.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	local liquidType = item:getAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE)
	if liquidType == LIQUID_MANAFLUID then
		local refresh = Magic.computeDamage(nil, 0, 100, 50)
		Magic.refreshMana(player, 0, 0, refresh)
	elseif liquidType == LIQUID_LIFEFLUID then
		local refresh = Magic.computeDamage(nil, 0, 50, 25)
		Magic.heal(player, 0, 0, refresh)
	else
		debugPrint("DrinkPotion: Object contains no potion.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	item:setAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE, LIQUID_NONE)
end

function onUse(player, item, target, window)
	if not item:exists() then
		debugPrint("UseLiquidContainer: Given object Obj does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	-- The target may be NONE (nil): the method as a function of the class.
	if not Item.exists(target) then
		debugPrint(string.format("UseLiquidContainer: Given object Dest does not exist (Obj %d).\n", item:getId()))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not item:hasFlag(FLAG_LIQUIDCONTAINER) then
		debugPrint("UseLiquidContainer: Given object is not a liquid container.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	local dest = target
	if dest:hasFlag(FLAG_LIQUIDSOURCE) and item:getAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE) == LIQUID_NONE then
		item:setAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE, dest:getTypeAttribute(ITEMTYPE_ATTRIBUTE_SOURCELIQUIDTYPE))
		return true
	end

	if dest:hasFlag(FLAG_LIQUIDCONTAINER) and item:getAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE) ~= LIQUID_NONE
			and dest:getAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE) == LIQUID_NONE then
		dest:setAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE, item:getAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE))
		item:setAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE, LIQUID_NONE)
		return true
	end

	-- Used on the field the player stands on: the player is the target.
	if not dest:isCreature() then
		local help = Item.getFirstObject(dest:getContainer())
		while help ~= nil do
			if help:isCreature() and help:getCreatureId() == player:getId() then
				dest = help
				break
			end
			help = help:getNextObject()
		end
	end

	if dest:isCreature() and dest:getCreatureId() == player:getId() then
		if player:isRemoved() then
			debugPrint(string.format("UseLiquidContainer: Player %d does not exist.\n", player:getId()))
			Game.throwResult(RETURNVALUE_ERROR)
		end

		local sound
		local liquidType = item:getAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE)
		if liquidType == LIQUID_NONE then
			return true
		elseif liquidType == LIQUID_WINE or liquidType == LIQUID_BEER then
			item:setAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE, LIQUID_NONE)
			sound = "Aah..."
			if player:getSkillTimer(SKILL_DRUNKEN) < 5 then
				player:setSkillTimer(SKILL_DRUNKEN, player:getSkillTimer(SKILL_DRUNKEN) + 1, 120, 120, -1)
			end
		elseif liquidType == LIQUID_SLIME then
			item:setAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE, LIQUID_NONE)
			sound = "Urgh!"
			player:damage(nil, 200, COMBAT_POISONCONDITION)
		elseif liquidType == LIQUID_URINE then
			item:setAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE, LIQUID_NONE)
			sound = "Urgh!"
		elseif liquidType == LIQUID_MANAFLUID or liquidType == LIQUID_LIFEFLUID then
			drinkPotion(player, item)
			sound = "Aaaah..."
		elseif liquidType == LIQUID_LEMONADE then
			item:setAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE, LIQUID_NONE)
			sound = "Mmmh."
		else
			item:setAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE, LIQUID_NONE)
			sound = "Gulp."
		end
		player:say(sound)
		return true
	end

	if item:getAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE) == LIQUID_NONE then
		Game.throwResult(RETURNVALUE_CANNOTUSETHISOBJECT)
	end

	if not Game.isLayable(dest:getPosition()) then
		Game.throwResult(RETURNVALUE_NOTENOUGHROOM)
	end

	-- int LiquidType: Change and CreatePool take it back as a uint32, so the
	-- number passes unchanged.
	local liquidType = item:getAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE)
	item:setAttribute(ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE, LIQUID_NONE)
	Game.createPool(dest:getMapContainer(), Game.getSpecialObject(SPECIAL_BLOOD_POOL), liquidType)
	return true
end
