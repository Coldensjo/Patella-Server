-- admin/give.lua: give <player> <item> [count]. Gives count (1 when not
-- given) of an item a player can carry, object by object (a cumulative type
-- in stacks of MAX_AMOUNT). Online, as the command Get Item creates at a
-- player (Game.createAtCreature): into a free hand or a container worn, else,
-- when nothing fits or it is too heavy, at the player's feet. Offline: into the
-- backpack (the container worn on CONST_SLOT_BACKPACK) while it has room, else
-- into the depot of the home town (the town nearest the temple), as mail is.

local admin = require("lib.admin")

function onCommand(words)
	if #words < 2 or #words > 3 then
		Admin.fail("Usage: give <player> <item> [count]")
		return
	end

	local characterId, name = admin.findCharacter(words[1])
	if characterId == nil then
		Admin.fail(name)
		return
	end

	local typeId, why = admin.parseItem(words[2], true)
	if typeId == nil then
		Admin.fail(why)
		return
	end

	local count = 1
	if words[3] ~= nil then
		count, why = admin.parseCount(words[3], admin.MAX_UNITS)
		if count == nil then
			Admin.fail(why)
			return
		end
	end

	local values
	values, why = admin.units(typeId, count)
	if values == nil then
		Admin.fail(why)
		return
	end

	local item = admin.itemName(typeId)
	if Admin.openCharacter(characterId) then
		local player = Admin.getPlayer()
		Admin.closeCharacter()
		local carried = 0
		local dropped = 0
		for _, value in ipairs(values) do
			local made = Game.createAtCreature(player:getId(), typeId, value)
			if made ~= nil and Admin.getCarrier(made) == player:getId() then
				carried = carried + math.max(value, 1)
			else
				dropped = dropped + math.max(value, 1)
			end
		end
		player:sendMagicEffect(CONST_ME_MAGIC_GREEN)
		Admin.reply(string.format("Gave %d of %s to %s.", count, item, name))
		if dropped > 0 then
			Admin.reply(string.format("%d did not fit and lie at the player's feet.", dropped))
		end
		return
	end

	local backpack = Admin.getInventoryContainer(CONST_SLOT_BACKPACK):getFirstObject()
	if backpack ~= nil and not backpack:hasFlag(FLAG_CONTAINER) then
		backpack = nil
	end
	local depot = admin.homeDepot(Admin.getStartPosition())
	local packed = 0
	local stored = 0
	for _, value in ipairs(values) do
		if backpack ~= nil and Admin.create(backpack, typeId, value) ~= nil then
			packed = packed + math.max(value, 1)
		else
			Admin.addToDepot(depot, typeId, value)
			stored = stored + math.max(value, 1)
		end
	end
	Admin.closeCharacter()
	Admin.reply(string.format("Gave %d of %s to %s (offline).", count, item, name))
	if packed > 0 then
		Admin.reply(string.format("%d went into the backpack.", packed))
	end
	if stored > 0 then
		Admin.reply(string.format("%d went into the depot of %s.", stored, Admin.getDepotTown(depot) or "depot " .. depot))
	end
end
