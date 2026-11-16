-- lib/monsters.lua: the monster spell library: it checks and keeps the spells
-- of the races and casts them, the spell loop of TMonster::IdleStimulus
-- (crnonpl.cc). A script, not a module: InitMonsterSpells loads it and the C++
-- side calls its two global functions, prepareSpell at start for each spell of
-- each race (monsters.toml's [[monster.spell]], in order) and castSpells from
-- IdleStimulus for a race with spells. The spells' keys, the shapes and impacts and the API this library uses
-- are in scripting.hh, section "Monsters, raids and monster homes".

-- The keys of each shape and each impact, in the order of the original's
-- parameters (Shape (p1, ...) -> Impact (q1, ...)).
local SHAPE_KEYS = {
	actor = {"effect"},
	victim = {"range", "animation", "effect"},
	origin = {"radius", "effect"},
	destination = {"range", "animation", "radius", "effect"},
	angle = {"angle", "range", "effect"},
}

local IMPACT_KEYS = {
	damage = {"damage_type", "damage", "variation"},
	field = {"field_type"},
	healing = {"amount", "variation"},
	speed = {"percent", "variation", "duration"},
	drunken = {"strength", "variation", "duration"},
	strength = {"skills", "percent", "variation", "duration"},
	outfit = {"outfit", "duration"},
	summon = {"race", "maximum"},
}

-- The prepared spells, by race number: Spells[race] is an array in the order
-- of the race's spells, each {oneIn = ..., shape = ..., impact = ..., p =
-- {<the shape's numbers in order>}, q = {<the impact's values in order>}}
-- (q of an outfit: id, colors (an array of four bytes), duration; q of a
-- summon: the race number, maximum). FleeThreshold[race] is the race's
-- FleeThreshold. Built by prepareSpell at start, never changed after (rule
-- 9: a cache).
local Spells = {}
local FleeThreshold = {}

-- A value as an error message shows it, without an address (rule 4).
local function describe(value)
	if type(value) == "string" then
		return "'" .. value .. "'"
	elseif type(value) == "number" then
		return tostring(value)
	end
	return "a " .. type(value)
end

local function isInteger(value)
	return type(value) == "number" and value == math.floor(value)
		and value >= -2147483648 and value <= 2147483647
end

-- Raises an error unless every key of t is one of allowed (an array of
-- names) or extra (a set of names). The unknown keys are named in sorted
-- order, so the message does not depend on the order of pairs (rule 4); pairs
-- is used only here, at start, to find keys that are not allowed.
local function checkKeys(t, what, allowed, extra)
	local known = {}
	for _, name in ipairs(allowed) do
		known[name] = true
	end
	if extra ~= nil then
		for _, name in ipairs(extra) do
			known[name] = true
		end
	end
	local unknown = {}
	for key in pairs(t) do
		if not known[key] then
			if type(key) == "string" then
				unknown[#unknown + 1] = key
			else
				unknown[#unknown + 1] = describe(key)
			end
		end
	end
	if #unknown > 0 then
		table.sort(unknown)
		error(what .. " has the unknown key(s) " .. table.concat(unknown, ", "), 0)
	end
end

local function getInteger(t, key, what)
	local value = t[key]
	if value == nil then
		error(what .. " needs " .. key, 0)
	end
	if not isInteger(value) then
		error(key .. " of " .. what .. " must be an integer, not " .. describe(value), 0)
	end
	return value
end

local function getRangedInteger(t, key, what, low, high)
	local value = getInteger(t, key, what)
	if value < low or value > high then
		error(key .. " of " .. what .. " must be " .. low .. " to " .. high .. ", not " .. value, 0)
	end
	return value
end

-- The outfit of an outfit impact, { id = <1 to 65535>, colors = [<four bytes>]
-- } or { id = 0, object_type = <0 to 65535> }, as the id and the four bytes
-- IdleStimulus made of the loaded parameters: an object type's two bytes
-- (low first, as the union holds it) and two zero bytes.
local function readOutfit(outfit, what)
	if type(outfit) ~= "table" then
		error("outfit of " .. what .. " must be a table { id, colors } or { id = 0, object_type }, not "
			.. describe(outfit), 0)
	end
	what = "the outfit of " .. what
	local id = getRangedInteger(outfit, "id", what, 0, 65535)
	if id == 0 then
		checkKeys(outfit, what, {"id", "object_type"})
		local objectType = getRangedInteger(outfit, "object_type", what, 0, 65535)
		return id, {objectType % 256, math.floor(objectType / 256), 0, 0}
	end
	checkKeys(outfit, what, {"id", "colors"})
	local colors = outfit.colors
	if type(colors) ~= "table" or #colors ~= 4 then
		error("colors of " .. what .. " must be an array of four bytes", 0)
	end
	checkKeys(colors, "colors of " .. what, {}, {1, 2, 3, 4})
	local bytes = {}
	for i = 1, 4 do
		local byte = colors[i]
		if not isInteger(byte) or byte < 0 or byte > 255 then
			error("color " .. i .. " of " .. what .. " must be 0 to 255, not " .. describe(byte), 0)
		end
		bytes[i] = byte
	end
	return id, bytes
end

-- Checks the spell table of race (a race number) and keeps it prepared, after
-- the race's spells prepared before it.
function prepareSpell(race, spell)
	if not isInteger(race) then
		error("prepareSpell: the race must be a race number, not " .. describe(race), 0)
	end
	if type(spell) ~= "table" then
		error("a spell must be a table, not " .. describe(spell), 0)
	end
	local shape = spell.shape
	local shapeKeys = type(shape) == "string" and SHAPE_KEYS[shape] or nil
	if shape == nil then
		error("the spell needs shape", 0)
	elseif shapeKeys == nil then
		error("unknown spell shape " .. describe(shape), 0)
	end
	local impact = spell.impact
	local impactKeys = type(impact) == "string" and IMPACT_KEYS[impact] or nil
	if impact == nil then
		error("the spell needs impact", 0)
	elseif impactKeys == nil then
		error("unknown spell impact " .. describe(impact), 0)
	end
	local what = "the spell " .. shape .. " -> " .. impact
	local allowed = {"shape", "impact", "one_in"}
	for _, key in ipairs(shapeKeys) do
		allowed[#allowed + 1] = key
	end
	checkKeys(spell, what, allowed, impactKeys)

	local oneIn = getInteger(spell, "one_in", what)
	if oneIn < 1 then
		error("one_in of " .. what .. " must be at least 1, not " .. oneIn, 0)
	end

	local p = {}
	for i, key in ipairs(shapeKeys) do
		p[i] = getInteger(spell, key, what)
	end

	local q = {}
	if impact == "outfit" then
		if spell.outfit == nil then
			error(what .. " needs outfit", 0)
		end
		q[1], q[2] = readOutfit(spell.outfit, what)
		q[3] = getInteger(spell, "duration", what)
	elseif impact == "summon" then
		local key = spell.race
		if key == nil then
			error(what .. " needs race", 0)
		end
		if type(key) ~= "string" then
			error("race of " .. what .. " must be a race key, not " .. describe(key), 0)
		end
		local summoned = MonsterType(key)
		if summoned == nil then
			error("race of " .. what .. ": unknown race " .. describe(key), 0)
		end
		q[1] = summoned:getId()
		q[2] = getInteger(spell, "maximum", what)
	else
		for i, key in ipairs(impactKeys) do
			q[i] = getInteger(spell, key, what)
		end
	end

	if Spells[race] == nil then
		local monsterType = MonsterType(race)
		if monsterType == nil then
			error("prepareSpell: no race " .. race, 0)
		end
		FleeThreshold[race] = monsterType:runHealth()
		Spells[race] = {}
	end
	local list = Spells[race]
	list[#list + 1] = {oneIn = oneIn, shape = shape, impact = impact, p = p, q = q}
end

-- The spell's impact as IdleStimulus made it, or nil where it made none (a
-- summon of a monster with a master).
local function makeImpact(monster, s)
	local impact = s.impact
	local q = s.q
	if impact == "damage" then
		return Impact.damage(monster, q[1], Magic.computeDamage(monster, 0, q[2], q[3]), true)
	elseif impact == "field" then
		return Impact.field(monster, q[1])
	elseif impact == "healing" then
		return Impact.healing(monster, Magic.computeDamage(monster, 0, q[1], q[2]))
	elseif impact == "speed" then
		return Impact.speed(monster, Magic.computeDamage(monster, 0, q[1], q[2]), q[3])
	elseif impact == "drunken" then
		return Impact.drunken(monster, Magic.computeDamage(monster, 0, q[1], q[2]), q[3])
	elseif impact == "strength" then
		return Impact.strength(monster, q[1], Magic.computeDamage(monster, 0, q[2], q[3]), q[4])
	elseif impact == "outfit" then
		return Impact.outfit(monster, q[1], q[2], q[3])
	elseif impact == "summon" then
		if monster:getMasterId() == 0 then
			return Impact.summon(monster, q[1], q[2])
		end
		return nil
	end
	-- prepareSpell lets only the impacts above through.
	error("unknown spell impact " .. describe(impact))
end

-- Casts the spell's shape with impact, turning to target first as
-- IdleStimulus did.
local function castShape(monster, target, s, impact)
	local shape = s.shape
	local p = s.p
	if shape == "actor" then
		Magic.actorShapeSpell(monster, impact, p[1])
	elseif shape == "victim" then
		if target ~= nil then
			monster:turnTo(target)
			Magic.victimShapeSpell(monster, target, p[1], p[2], impact, p[3])
		end
	elseif shape == "origin" then
		Magic.originShapeSpell(monster, p[1], impact, p[2])
	elseif shape == "destination" then
		if target ~= nil then
			monster:turnTo(target)
			Magic.destinationShapeSpell(monster, target, p[1], p[2], p[3], impact, p[4])
		end
	elseif shape == "angle" then
		if target ~= nil then
			monster:turnTo(target)
		end
		Magic.angleShapeSpell(monster, p[1], p[2], impact, p[3])
	else
		-- prepareSpell lets only the shapes above through.
		error("unknown spell shape " .. describe(shape))
	end
end

-- IdleStimulus's spell loop for monster (a Monster) of race (its race
-- number), target the creature of its Target when the loop began (nil for
-- none).
function castSpells(monster, race, target)
	local spells = Spells[race]
	if spells == nil then
		error("castSpells: race " .. race .. " has no prepared spells")
	end
	local fleeThreshold = FleeThreshold[race]
	for _, s in ipairs(spells) do
		-- rand() is not negative and oneIn at least 1, so Lua's % is C's.
		if Game.rand() % s.oneIn == 0 then
			if not (monster:getMasterId() == 0
					and monster:getHealth() <= fleeThreshold
					and math.random(1, 3) ~= 1) then
				local impact = makeImpact(monster, s)
				if impact ~= nil then
					if not impact:isAggressive() then
						castShape(monster, target, s, impact)
					else
						local targetId = monster:getTargetId()
						if targetId ~= 0 and targetId ~= monster:getMasterId() then
							castShape(monster, target, s, impact)
						end
					end
				end
			end
		end
	end
end
