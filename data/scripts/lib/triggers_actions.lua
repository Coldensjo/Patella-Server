-- lib/triggers_actions.lua: the actions of the trigger rules, what the cases of
-- ExecuteAction's switch (triggers.cc) did, keyed by their triggers.toml names.
-- lib/triggers.lua requires this module; an action is Actions[name](ctx, ...),
-- the arguments in triggers.toml's order as prepareAction got them: an object
-- as its name ("obj1", "obj2", "user", "temp", "null"), a type, an
-- instance_attribute or a number as a number, a coordinate or a vector as a
-- Position, a text as the number of its dynamic string. ctx is the rule's
-- context (lib/triggers.lua): ctx.event, ctx:get(name), and ctx.temp, which
-- create_on_map, create, change_on_map and change_rel set as they set Temp.
--
-- Each action makes the calls of its case, in its order, with its arguments.
-- Where the case threw ERROR, the action raises RETURNVALUE_ERROR; a result
-- an API function raises passes on (lib/triggers.lua logs it as
-- ExecuteAction's catch did). The case's own error() texts are logged with
-- debugPrint, and the action returns where the case returned.
--
-- NONE. An object a rule names may be NONE (nil here), and so may an object
-- the map gives (a container, a first or next object). The case called the
-- C++ functions on NONE too, and they log and give what they give for it; so
-- where an object may be nil, the action calls the Item method as a function
-- of the class (Item.getPosition(obj) rather than obj:getPosition()) and
-- passes nil to the Game functions, and the binding hands NONE to the same
-- C++ function. Where the case tested for NONE itself (Obj == NONE, Obj !=
-- NONE), the action tests for nil.
local Actions = {}

local TWO_TO_32 = 4294967296

-- An offset added to a field: plain int sums.
local function offset(position, vector)
	return Position(position.x + vector.x, position.y + vector.y, position.z + vector.z)
end

-- Delete's count: one piece when an object is used, all of it otherwise.
local function deleteCount(ctx)
	if ctx.event == "use" or ctx.event == "multi_use" then
		return 1
	end
	return -1
end

Actions.create_on_map = function(ctx, position, typeId, value)
	ctx.temp = Game.createObject(Game.getMapContainer(position), typeId, value)
end

Actions.create = function(ctx, object, typeId, value)
	local obj = ctx:get(object)
	ctx.temp = Game.createObject(Item.getContainer(obj), typeId, value)
end

Actions.monster_on_map = function(ctx, position, race)
	Game.createMonsterOfRace(race, position)
end

Actions.monster = function(ctx, object, race)
	local obj = ctx:get(object)
	Game.createMonsterOfRace(race, Item.getPosition(obj))
end

Actions.effect_on_map = function(ctx, position, effect)
	Item.sendMagicEffect(Game.getMapContainer(position), effect)
end

Actions.effect = function(ctx, object, effect)
	local obj = ctx:get(object)
	Item.sendMagicEffect(Item.getMapContainer(obj), effect)
end

Actions.text_on_map = function(ctx, position, text, radius)
	Game.textEffect(Game.getDynamicString(text), position, radius)
end

-- The coordinates first, then the text.
Actions.text = function(ctx, object, text, radius)
	local obj = ctx:get(object)
	local position = Item.getPosition(obj)
	Game.textEffect(Game.getDynamicString(text), position, radius)
end

Actions.change_on_map = function(ctx, position, typeId, newTypeId, value)
	ctx.temp = Game.getFirstObjectOfType(position, typeId)
	if ctx.temp == nil then
		debugPrint(string.format("ExecuteAction (CHANGEONMAP): No object %d at [%d,%d,%d].\n",
				typeId, position.x, position.y, position.z))
		return
	end
	Game.changeObject(ctx.temp, newTypeId, value)
end

Actions.change = function(ctx, object, typeId, value)
	local obj = ctx:get(object)
	Game.changeObject(obj, typeId, value)
end

Actions.change_rel = function(ctx, object, vector, typeId, newTypeId, value)
	local obj = ctx:get(object)
	local position = offset(Item.getPosition(obj), vector)
	ctx.temp = Game.getFirstObjectOfType(position, typeId)
	if ctx.temp == nil then
		debugPrint(string.format("ExecuteAction (CHANGEREL): No object %d at [%d,%d,%d].\n",
				typeId, position.x, position.y, position.z))
		return
	end
	Game.changeObject(ctx.temp, newTypeId, value)
end

Actions.set_attribute = function(ctx, object, attribute, value)
	local obj = ctx:get(object)
	Item.setAttribute(obj, attribute, value)
end

-- x = attribute + number: the attribute a uint32_t (0 to 2^32 - 1 here), the
-- number an int, so the sum is taken modulo 2^32 and stored in the int x as
-- two's complement; bit.tobit does that (the sum is exact in a double, far
-- below 2^51). x <= 0 becomes whether the attribute, read again, is not 0.
-- Change takes x as a uint32_t; x is 0 or more by then.
Actions.change_attribute = function(ctx, object, attribute, number)
	local obj = ctx:get(object)
	local x = bit.tobit(Item.getAttribute(obj, attribute) + number)
	if x <= 0 then
		if Item.getAttribute(obj, attribute) ~= 0 then
			x = 1
		else
			x = 0
		end
	end
	Item.setAttribute(obj, attribute, x)
end

Actions.set_quest_value = function(ctx, object, number, value)
	local obj = ctx:get(object)
	if not Item.isCreature(obj) then
		debugPrint(string.format("ExecuteAction (SETQUESTVALUE): Object is not a creature but type %d.\n",
				Item.getId(obj)))
		return
	end
	local cr = Item.getCreature(obj)
	if cr == nil or not cr:isPlayer() then
		debugPrint("ExecuteAction (SETQUESTVALUE): Creature does not exist or is not a player.\n")
		return
	end
	cr:setStorageValue(number, value)
end

-- The attacker: the first object's creature, or the creature responsible for
-- a magic field while its first five seconds last (CronInfo before the type
-- attribute). Remaining + 5 > TOTALEXPIRETIME compared uint32_t values: both
-- come as 0 to 2^32 - 1, and the sum wraps modulo 2^32 as the unsigned one
-- did. The target is the second object's creature.
Actions.damage = function(ctx, attackerObject, targetObject, damageType, amount)
	local obj = ctx:get(attackerObject)
	local attacker = nil
	if obj ~= nil then
		if obj:isCreature() then
			attacker = obj:getCreature()
		end
		if obj:hasFlag(FLAG_MAGICFIELD) and obj:getAttribute(ITEM_ATTRIBUTE_RESPONSIBLE) ~= 0 then
			local remaining = obj:getCronInfo()
			if (remaining + 5) % TWO_TO_32 > obj:getTypeAttribute(ITEMTYPE_ATTRIBUTE_TOTALEXPIRETIME) then
				attacker = Game.getCreature(obj:getAttribute(ITEM_ATTRIBUTE_RESPONSIBLE))
			end
		end
	end
	obj = ctx:get(targetObject)
	if not Item.isCreature(obj) then
		debugPrint(string.format("ExecuteAction (DAMAGE): Object is not a creature but type %d.\n", Item.getId(obj)))
		return
	end
	local cr = Item.getCreature(obj)
	if cr == nil then
		debugPrint("ExecuteAction (DAMAGE): Creature does not exist.\n")
		return
	end
	cr:damage(attacker, amount, damageType)
end

Actions.set_start = function(ctx, object, position)
	local obj = ctx:get(object)
	if not Item.isCreature(obj) then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	local cr = Item.getCreature(obj)
	if cr == nil or not cr:isPlayer() then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	cr:setStartPosition(position)
end

-- The text with each "%N" replaced by the creature's name, built as in a
-- zeroed char[256]; ERROR as soon as it is longer than 200 characters
-- (Text[200] != 0 after a step). Neither the text nor a name holds a zero
-- byte, so the length is strlen(Text).
Actions.write_name = function(ctx, object, text, target)
	local obj = ctx:get(object)
	if not Item.isCreature(obj) then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	local cr = Item.getCreature(obj)
	if cr == nil then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	local source = Game.getDynamicString(text)
	if source == nil then
		-- The C++ code took strlen(nullptr) here and crashed; ERROR instead.
		Game.throwResult(RETURNVALUE_ERROR)
	end
	local parts = {}
	local length = 0
	local pos = 1
	local sourceLength = #source
	while pos <= sourceLength do
		local c = string.sub(source, pos, pos)
		if c == "%" and string.sub(source, pos + 1, pos + 1) == "N" then
			local name = cr:getName()
			parts[#parts + 1] = name
			length = length + #name
			pos = pos + 1
		else
			parts[#parts + 1] = c
			length = length + 1
		end
		if length > 200 then
			Game.throwResult(RETURNVALUE_ERROR)
		end
		pos = pos + 1
	end
	obj = ctx:get(target)
	if obj == nil or not obj:hasFlag(FLAG_TEXT) then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	Game.deleteDynamicString(obj:getAttribute(ITEM_ATTRIBUTE_TEXTSTRING))
	obj:setAttribute(ITEM_ATTRIBUTE_TEXTSTRING, Game.addDynamicString(table.concat(parts)))
	obj:setAttribute(ITEM_ATTRIBUTE_EDITOR, 0)
end

Actions.write_text = function(ctx, text, target)
	local obj = ctx:get(target)
	if obj == nil or not obj:hasFlag(FLAG_TEXT) then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	Game.deleteDynamicString(obj:getAttribute(ITEM_ATTRIBUTE_TEXTSTRING))
	local source = Game.getDynamicString(text)
	if source == nil then
		-- The C++ code passed nullptr to AddDynamicString, whose strlen
		-- crashed; ERROR instead.
		Game.throwResult(RETURNVALUE_ERROR)
	end
	obj:setAttribute(ITEM_ATTRIBUTE_TEXTSTRING, Game.addDynamicString(source))
	obj:setAttribute(ITEM_ATTRIBUTE_EDITOR, 0)
end

Actions.logout = function(ctx, object)
	local obj = ctx:get(object)
	if not Item.isCreature(obj) then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	local cr = Item.getCreature(obj)
	if cr == nil then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	cr:startLogout()
end

Actions.move_all_on_map = function(ctx, from, to)
	local obj = Game.getFirstObject(from)
	local con = Game.getMapContainer(to)
	Game.moveAllObjects(obj, con, nil, false)
end

-- From the first object of the object's container on; that first object
-- stays (it is the excluded one).
Actions.move_all = function(ctx, object, position)
	local obj = ctx:get(object)
	obj = Item.getFirstObject(Item.getContainer(obj))
	local con = Game.getMapContainer(position)
	Game.moveAllObjects(obj, con, obj, false)
end

Actions.move_all_rel = function(ctx, object, vector)
	local obj = ctx:get(object)
	obj = Item.getFirstObject(Item.getContainer(obj))
	local con = Game.getMapContainer(offset(Item.getPosition(obj), vector))
	Game.moveAllObjects(obj, con, obj, false)
end

-- What lies on the object of the type, the container first.
Actions.move_top_on_map = function(ctx, from, typeId, to)
	local obj = Game.getFirstObjectOfType(from, typeId)
	if obj == nil then
		debugPrint(string.format("ExecuteAction (MOVETOPONMAP): No object %d at [%d,%d,%d].\n",
				typeId, from.x, from.y, from.z))
		return
	end
	local con = Game.getMapContainer(to)
	Game.moveAllObjects(obj:getNextObject(), con, nil, true)
end

Actions.move_top = function(ctx, object, position)
	local obj = ctx:get(object)
	local con = Game.getMapContainer(position)
	Game.moveAllObjects(Item.getNextObject(obj), con, nil, true)
end

Actions.move_top_rel = function(ctx, object, vector)
	local obj = ctx:get(object)
	local con = Game.getMapContainer(offset(Item.getPosition(obj), vector))
	Game.moveAllObjects(Item.getNextObject(obj), con, nil, true)
end

Actions.move = function(ctx, object, position)
	local obj = ctx:get(object)
	local con = Game.getMapContainer(position)
	Game.moveObject(obj, con)
end

-- The first object goes to the second object's field plus the vector.
Actions.move_rel = function(ctx, object, reference, vector)
	local obj = ctx:get(object)
	local position = Item.getPosition(ctx:get(reference))
	local con = Game.getMapContainer(offset(position, vector))
	Game.moveObject(obj, con)
end

-- The top object of the object's field plus the first vector, unless it is
-- unmovable, goes to the object's field plus the second vector.
Actions.retrieve = function(ctx, object, fromVector, toVector)
	local position = Item.getPosition(ctx:get(object))
	local obj = Game.getTopObject(offset(position, fromVector))
	if obj == nil or obj:hasFlag(FLAG_UNMOVE) then
		return
	end
	local con = Game.getMapContainer(offset(position, toVector))
	Game.moveObject(obj, con)
end

Actions.delete_all_on_map = function(ctx, position)
	local obj = Game.getFirstObject(position)
	Game.deleteAllObjects(obj, obj, false)
end

Actions.delete_top_on_map = function(ctx, position, typeId)
	local obj = Game.getFirstObjectOfType(position, typeId)
	if obj == nil then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	Game.deleteAllObjects(obj:getNextObject(), obj, true)
end

Actions.delete_on_map = function(ctx, position, typeId)
	local obj = Game.getFirstObjectOfType(position, typeId)
	if obj == nil then
		debugPrint(string.format("ExecuteAction (DELETEONMAP): No object %d at [%d,%d,%d].\n",
				typeId, position.x, position.y, position.z))
		return
	end
	Game.deleteObject(obj, deleteCount(ctx))
end

Actions.delete = function(ctx, object)
	local obj = ctx:get(object)
	if obj == nil then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	Game.deleteObject(obj, deleteCount(ctx))
end

Actions.delete_in_inventory = function(ctx, object, typeId, value)
	local obj = ctx:get(object)
	if not Item.isCreature(obj) then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	Game.deleteAtCreature(Item.getCreatureId(obj), typeId, 1, value)
end

-- The second object is the player who reads, the first the object described;
-- the player is looked up first. SendMessage(..., "%s.", GetInfo(Obj))
-- printed glibc's "(null)" for a nullptr description.
Actions.description = function(ctx, object, reader)
	local obj = ctx:get(reader)
	if Item.isCreature(obj) then
		local cr = Item.getCreature(obj)
		if cr ~= nil and cr:isPlayer() then
			obj = ctx:get(object)
			local description = Item.getDescription(obj)
			if description == nil then
				cr:sendTextMessage(MESSAGE_INFO_DESCR, "(null).")
			else
				cr:sendTextMessage(MESSAGE_INFO_DESCR, description .. ".")
			end
		end
	end
end

Actions.load_depot = function(ctx, position, typeId, object, number)
	local obj = ctx:get(object)
	if not Item.isCreature(obj) then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	local con = Game.getFirstObjectOfType(position, typeId)
	if con == nil then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	Game.loadDepotBox(Item.getCreatureId(obj), number, con)
end

Actions.save_depot = function(ctx, position, typeId, object, number)
	local obj = ctx:get(object)
	if not Item.isCreature(obj) then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	local con = Game.getFirstObjectOfType(position, typeId)
	if con == nil then
		Game.throwResult(RETURNVALUE_ERROR)
	end
	Game.saveDepotBox(Item.getCreatureId(obj), number, con)
end

Actions.send_mail = function(ctx, object)
	local obj = ctx:get(object)
	Game.sendMail(obj)
end

Actions.nop = function(ctx)
end

return Actions
