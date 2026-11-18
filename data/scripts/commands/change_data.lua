-- commands/change_data.lua: Change Data ("aleta para"), what ChangeData (the
-- original magic.cc) did: with the command's right, sets a number of the
-- object in the caster's hand (one hand only; with nothing in hand, the first
-- key door on the caster's field) to the word's number: a liquid container's
-- liquid, a key's number, a key door's keyhole number, a cumulative object's
-- amount (1 to MAX_AMOUNT) or a rune's charges (1 to 99): the first of these
-- whose flag the object has and whose range the number is in (a liquid any
-- number); and shows the green sparkles on the caster. The caster is told when
-- both hands hold an object or the object cannot take the number.

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local object = Game.getBodyObject(caster:getId(), CONST_SLOT_RIGHT)
	local leftObject = Game.getBodyObject(caster:getId(), CONST_SLOT_LEFT)
	if object ~= nil and leftObject ~= nil then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "First drop one object.")
		return
	end

	if leftObject ~= nil then
		object = leftObject
	end

	if object == nil then
		object = Game.getFirstObject(caster:getPosition())
		while object ~= nil do
			if object:hasFlag(FLAG_KEYDOOR) then
				break
			end
			object = object:getNextObject()
		end
	end

	if object == nil then
		return
	end

	-- (uint32_t)atol(word): the low 32 bits of strtol's long, as an unsigned
	-- number (Game.atoi has the same bits as a signed int).
	local value = Game.atoi(words[2])
	if value < 0 then
		value = value + 4294967296
	end

	local attribute
	if object:hasFlag(FLAG_LIQUIDCONTAINER) then
		attribute = ITEM_ATTRIBUTE_CONTAINERLIQUIDTYPE
	elseif object:hasFlag(FLAG_KEY) and value ~= 0 then
		attribute = ITEM_ATTRIBUTE_KEYNUMBER
	elseif object:hasFlag(FLAG_KEYDOOR) and value ~= 0 then
		attribute = ITEM_ATTRIBUTE_KEYHOLENUMBER
	elseif object:hasFlag(FLAG_CUMULATIVE) and value ~= 0 and value <= MAX_AMOUNT then
		attribute = ITEM_ATTRIBUTE_AMOUNT
	elseif object:hasFlag(FLAG_RUNE) and value ~= 0 and value <= 99 then
		attribute = ITEM_ATTRIBUTE_CHARGES
	else
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "You can't change " .. object:getName() .. " in this way.")
		return
	end

	object:setAttribute(attribute, value)
	caster:sendMagicEffect(CONST_ME_MAGIC_GREEN)
end
