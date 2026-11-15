-- lib/triggers.lua: the trigger library, what CheckCondition, HandleEvent's
-- check of one rule and ExecuteAction's catch (triggers.cc) did for the rules
-- of triggers.toml. The C++ side calls its global functions: prepareCondition
-- and prepareAction at start, for each condition and action of each rule in
-- the file's order, and evaluateRule for one rule of an event (HandleEvent
-- keeps the loop over the rules). The conditions are here, keyed by their
-- triggers.toml names; the actions are the module lib/triggers_actions.lua.
--
-- The context of one rule evaluation (ctx), which every condition and action
-- takes as its first argument:
--   ctx.event      the event's name: "use", "multi_use", "movement",
--                  "collision" or "separation".
--   ctx.user, ctx.obj1, ctx.obj2
--                  the event's objects, Items, nil for NONE.
--   ctx.temp       the rule's temp object, an Item or nil (NONE); nil when
--                  the rule's check starts (Temp = NONE). is_object_there,
--                  is_creature_there, is_player_there and
--                  is_object_in_inventory set it; actions read and set it.
--   ctx:get(name)  GetEventObject: the object an argument of kind object
--                  names, ctx.obj1 for "obj1", ctx.obj2 for "obj2", ctx.user
--                  for "user", ctx.temp (as it is at the call) for "temp",
--                  nil for "null".
-- An action is Actions[name](ctx, ...), the arguments those prepareAction
-- got, in triggers.toml's order.
--
-- An object a rule names may be NONE (nil). The C++ code called the object's
-- functions on NONE too, and they log and give what they give for it (as
-- AccessObject's "Invalid object number zero" and the object type 0); so this
-- library calls the Item methods as functions of the class, Item.isCreature(obj)
-- rather than obj:isCreature(), wherever the object may be NONE, and the
-- binding does for nil what the C++ function does for NONE.

local Actions = require("lib.triggers_actions")

-- The prepared rules, by rule number: Rules[rule] = {conditions = {...},
-- actions = {...}}, each entry {name = ..., fn = ..., args = {n = ..., ...}}
-- (a condition also negate and ignoreResult). Built at start, never changed
-- after.
local Rules = {}

local function getEventObject(ctx, name)
	if name == "obj1" then
		return ctx.obj1
	elseif name == "obj2" then
		return ctx.obj2
	elseif name == "user" then
		return ctx.user
	elseif name == "temp" then
		return ctx.temp
	elseif name == "null" then
		return nil
	end
	-- The loader passes only the five names.
	error("unknown event object '" .. tostring(name) .. "'")
end

-- Compare: Value1 and Value2 are C++ ints there (an unsigned attribute
-- becomes one); the operator is the comparison's text.
local function compare(value1, operator, value2)
	value1 = bit.tobit(value1)
	value2 = bit.tobit(value2)
	if operator == "<" then
		return value1 < value2
	elseif operator == ">" then
		return value1 > value2
	elseif operator == "=" then
		return value1 == value2
	elseif operator == "<>" then
		return value1 ~= value2
	elseif operator == "<=" then
		return value1 <= value2
	elseif operator == ">=" then
		return value1 >= value2
	end
	-- The loader checks the comparisons, so this cannot happen.
	debugPrint(string.format("Compare: Invalid operator %s.\n",
			type(operator) == "string" and operator or type(operator)))
	return false
end

-- The conditions, each the case of CheckCondition's switch, making its calls
-- in its order; each gives whether it is fulfilled (before negate and
-- ignoreResult).
local Conditions = {}

Conditions.is_position = function(ctx, object, position)
	local obj = ctx:get(object)
	local where = Item.getPosition(obj)
	return position.x == where.x and position.y == where.y and position.z == where.z
end

Conditions.is_type = function(ctx, object, typeId)
	local obj = ctx:get(object)
	return Item.getId(obj) == typeId
end

Conditions.is_creature = function(ctx, object)
	local obj = ctx:get(object)
	return Item.isCreature(obj)
end

Conditions.is_player = function(ctx, object)
	local obj = ctx:get(object)
	local fulfilled = false
	if Item.isCreature(obj) then
		local cr = Item.getCreature(obj)
		fulfilled = cr ~= nil and cr:isPlayer()
	end
	return fulfilled
end

Conditions.has_flag = function(ctx, object, flag)
	local obj = ctx:get(object)
	return Item.hasFlag(obj, flag)
end

Conditions.has_type_attribute = function(ctx, object, attribute, operator, value)
	local obj = ctx:get(object)
	return compare(Item.getTypeAttribute(obj, attribute), operator, value)
end

Conditions.has_instance_attribute = function(ctx, object, attribute, operator, value)
	local obj = ctx:get(object)
	return compare(Item.getAttribute(obj, attribute), operator, value)
end

-- The parameter's string first, then the object's (both can log).
Conditions.has_text = function(ctx, object, text)
	local obj = ctx:get(object)
	local wanted = Game.getDynamicString(text)
	local written = Game.getDynamicString(Item.getAttribute(obj, ITEM_ATTRIBUTE_TEXTSTRING))
	return written == wanted
end

Conditions.is_peaceful = function(ctx, object)
	local obj = ctx:get(object)
	local fulfilled = false
	if Item.isCreature(obj) then
		local cr = Item.getCreature(obj)
		fulfilled = cr ~= nil and cr:isPeaceful()
	end
	return fulfilled
end

Conditions.may_logout = function(ctx, object)
	local obj = ctx:get(object)
	local fulfilled = false
	if Item.isCreature(obj) then
		local cr = Item.getCreature(obj)
		fulfilled = cr ~= nil and cr:isPlayer() and cr:canLogout()
	end
	return fulfilled
end

Conditions.has_profession = function(ctx, object, profession)
	local obj = ctx:get(object)
	local fulfilled = false
	if Item.isCreature(obj) then
		local cr = Item.getCreature(obj)
		fulfilled = cr ~= nil and cr:isPlayer() and cr:getVocation():getBaseId() == profession
	end
	return fulfilled
end

-- Any creature's level (Skills[SKILL_LEVEL]->Get()), not only a player's.
Conditions.has_level = function(ctx, object, operator, value)
	local obj = ctx:get(object)
	local fulfilled = false
	if Item.isCreature(obj) then
		local cr = Item.getCreature(obj)
		fulfilled = cr ~= nil and compare(cr:getSkillValue(SKILL_LEVEL), operator, value)
	end
	return fulfilled
end

Conditions.has_right = function(ctx, object, right)
	local obj = ctx:get(object)
	local fulfilled = false
	if Item.isCreature(obj) then
		local cr = Item.getCreature(obj)
		fulfilled = cr ~= nil and cr:isPlayer() and cr:hasRight(right)
	end
	return fulfilled
end

Conditions.has_quest_value = function(ctx, object, number, operator, value)
	local obj = ctx:get(object)
	local fulfilled = false
	if Item.isCreature(obj) then
		local cr = Item.getCreature(obj)
		fulfilled = cr ~= nil and cr:isPlayer() and compare(cr:getStorageValue(number), operator, value)
	end
	return fulfilled
end

-- The creature is looked up before the skill number is checked.
Conditions.test_skill = function(ctx, object, skill, a, b)
	local obj = ctx:get(object)
	local fulfilled = false
	if Item.isCreature(obj) then
		local cr = Item.getCreature(obj)
		if skill >= 0 and skill <= SKILL_LAST then
			fulfilled = cr ~= nil and cr:probeSkill(skill, a, b)
		else
			debugPrint(string.format("CheckCondition (TestSkill): Invalid skill number %d.\n", skill))
		end
	end
	return fulfilled
end

Conditions.count_objects = function(ctx, object, operator, value)
	local obj = ctx:get(object)
	return compare(Item.countObjects(Item.getMapContainer(obj)), operator, value)
end

Conditions.count_objects_on_map = function(ctx, position, operator, value)
	return compare(Item.countObjects(Game.getMapContainer(position)), operator, value)
end

Conditions.is_object_there = function(ctx, position, typeId)
	ctx.temp = Game.getFirstObjectOfType(position, typeId)
	return ctx.temp ~= nil
end

Conditions.is_creature_there = function(ctx, position)
	ctx.temp = Game.getFirstObjectOfType(position, TYPEID_CREATURE)
	return ctx.temp ~= nil
end

Conditions.is_player_there = function(ctx, position)
	ctx.temp = Game.getFirstObjectOfType(position, TYPEID_CREATURE)
	local fulfilled = false
	if ctx.temp ~= nil then
		local cr = ctx.temp:getCreature()
		fulfilled = cr ~= nil and cr:isPlayer()
	end
	return fulfilled
end

Conditions.is_object_in_inventory = function(ctx, object, typeId, value)
	local obj = ctx:get(object)
	local fulfilled = false
	if Item.isCreature(obj) then
		ctx.temp = Game.getInventoryObject(Item.getCreatureId(obj), typeId, value)
		fulfilled = ctx.temp ~= nil
	end
	return fulfilled
end

Conditions.is_protection_zone = function(ctx, object)
	local obj = ctx:get(object)
	return Game.isProtectionZone(Item.getPosition(obj))
end

Conditions.is_house = function(ctx, object)
	local obj = ctx:get(object)
	return Game.isHouse(Item.getPosition(obj))
end

-- The field of the first object, the owner the second.
Conditions.is_house_owner = function(ctx, house, owner)
	local where = Item.getPosition(ctx:get(house))
	local obj = ctx:get(owner)
	local fulfilled = false
	if Item.isCreature(obj) then
		local cr = Item.getCreature(obj)
		fulfilled = cr ~= nil and cr:isPlayer() and cr:isHouseOwner(Game.getHouseId(where))
	end
	return fulfilled
end

Conditions.is_dressed = function(ctx, object)
	local obj = ctx:get(object)
	local fulfilled = false
	if Item.hasFlag(obj, FLAG_CLOTHES) then
		fulfilled = Item.getBodyPosition(obj) == Item.getTypeAttribute(obj, ITEMTYPE_ATTRIBUTE_BODYPOSITION)
	else
		debugPrint("CheckCondition (IsDressed): Object is not a piece of clothing.\n")
	end
	return fulfilled
end

Conditions.random = function(ctx, percent)
	return math.random(1, 100) <= percent
end

local function prepared(rule)
	local entry = Rules[rule]
	if entry == nil then
		entry = {conditions = {}, actions = {}}
		Rules[rule] = entry
	end
	return entry
end

function prepareCondition(rule, name, negate, ignoreResult, ...)
	local fn = Conditions[name]
	if fn == nil then
		error("unknown condition '" .. tostring(name) .. "'", 0)
	end
	local conditions = prepared(rule).conditions
	conditions[#conditions + 1] = {name = name, fn = fn, negate = negate, ignoreResult = ignoreResult,
			args = {n = select('#', ...), ...}}
end

function prepareAction(rule, name, ...)
	local fn = Actions[name]
	if fn == nil then
		error("unknown action '" .. tostring(name) .. "'", 0)
	end
	local actions = prepared(rule).actions
	actions[#actions + 1] = {name = name, fn = fn, args = {n = select('#', ...), ...}}
end

-- The context the conditions of every rule are checked with (evaluateRule).
local CheckContext = {get = getEventObject}

function evaluateRule(rule, event, user, item1, item2)
	local entry = Rules[rule]
	if entry == nil then
		error("rule " .. tostring(rule) .. " has no conditions or actions")
	end
	-- The conditions only read and never set off another event, so one table
	-- serves every rule's check (no table is made per rule); the actions,
	-- which can set off events and so a nested evaluateRule, get a table of
	-- their own below.
	local ctx = CheckContext
	ctx.event, ctx.user, ctx.obj1, ctx.obj2, ctx.temp = event, user, item1, item2, nil

	-- A result error a condition raises propagates: CheckCondition did not
	-- catch.
	local conditions = entry.conditions
	for i = 1, #conditions do
		local condition = conditions[i]
		local args = condition.args
		local fulfilled = condition.fn(ctx, unpack(args, 1, args.n))
		if condition.ignoreResult then
			fulfilled = true
		elseif condition.negate then
			fulfilled = not fulfilled
		end
		if not fulfilled then
			return false
		end
	end

	ctx = {event = event, user = user, obj1 = item1, obj2 = item2, temp = ctx.temp, get = getEventObject}

	-- ExecuteAction caught RESULT, logged it and went on with the next
	-- action; any other error passes on.
	local actions = entry.actions
	for i = 1, #actions do
		local action = actions[i]
		local args = action.args
		local ok, err = pcall(action.fn, ctx, unpack(args, 1, args.n))
		if not ok then
			local code = Game.getResultOf(err)
			if code == nil then
				error(err, 0)
			end
			debugPrint(string.format("ExecuteAction: Exception %d (action %s).\n", code, action.name))
		end
	end
	return true
end
