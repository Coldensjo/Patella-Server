-- commands/get_item_count.lua: Get Item ("alevo para para"), what CreateThing
-- (the original magic.cc) did for it: with the command's right, creates at the
-- caster as many objects (1 to 100; of a type that cannot be taken only one)
-- as the second word says, of the type the first word names, by number (a type
-- above the creature's that exists and is not unmovable) or, for a word whose
-- number is 0, by name (a movable type's): a cumulative type as one object of
-- that amount, any other as that many objects. Shows the green sparkles on the
-- caster's field. A word that names no such type, or a count it refuses, is
-- told the caster.

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local typeId = 0
	if Game.atoi(words[2]) == 0 then
		typeId = ItemType.fromName(words[2], true)
	else
		local number = Game.atoi(words[2])
		if number > TYPEID_CREATURE and ItemType.exists(number) then
			typeId = number
		end

		if ItemType(typeId):hasFlag(FLAG_UNMOVE) then
			typeId = 0
		end
	end

	if typeId == 0 then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "There is no such takeable object.")
		return
	end

	local count = Game.atoi(words[3])
	if count < 1 or count > 100 then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "You may only create 1 to 100 objects.")
		return
	end

	local itemType = ItemType(typeId)
	if not itemType:hasFlag(FLAG_TAKE) and count ~= 1 then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "You may only create one untakeable object.")
		return
	end

	local container = caster:getMapContainer()
	if itemType:hasFlag(FLAG_CUMULATIVE) then
		Game.createAtCreature(caster:getId(), typeId, count)
	else
		for i = 1, count do
			Game.createAtCreature(caster:getId(), typeId, 0)
		end
	end

	Item.sendMagicEffect(container, CONST_ME_MAGIC_GREEN)
end
