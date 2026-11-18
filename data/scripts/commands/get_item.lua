-- commands/get_item.lua: Get Item ("alevo para"), what CreateThing (the
-- original magic.cc) did for it: with the command's right, creates at the
-- caster one object of the type the word names, by number (a type above the
-- creature's that exists and is not unmovable) or, for a word whose number is
-- 0, by name (a movable type's), and shows the green sparkles on the caster's
-- field. A word that names no such type is told the caster. (The count is 1,
-- so CreateThing's count checks cannot fail and are left out.)

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

	local container = caster:getMapContainer()
	if ItemType(typeId):hasFlag(FLAG_CUMULATIVE) then
		Game.createAtCreature(caster:getId(), typeId, 1)
	else
		Game.createAtCreature(caster:getId(), typeId, 0)
	end

	Item.sendMagicEffect(container, CONST_ME_MAGIC_GREEN)
end
