-- commands/move.lua: Move ("alani para"), what Teleport (the original
-- magic.cc) did: by the word, each with the command's right for it, moves the
-- caster a floor up or down ("up", "down": MOVE_RIGHT_VERTICAL), sets its
-- speed ("fast", "fastest", "slow" or "normal": MOVE_RIGHT_SPEED; and "max",
-- ours: SPEED_MAX), sends it to a coordinate ("x,y,z" or "[x,y,z]":
-- MOVE_RIGHT_COORDINATE; one with a number below 1 or off the map counts from
-- the caster's field) or to the mark of the map the word names (any other
-- word: MOVE_RIGHT_MARK). A move goes to the first free field within 1 of the
-- destination (one of the mark's house for a mark), with the teleport effect
-- there; none: NOROOM.
--
-- Given by a talkaction (talkactions.toml; its words are onSay's talk), the
-- word may only be of one kind: "/speed" a speed, "/pos" a coordinate,
-- "/town" a mark (another word is told the caster); "/a" (ours) takes a
-- number of fields and moves the caster that many fields the way it faces
-- (backwards for a negative number), to the first free field within 1 of
-- there (MOVE_RIGHT_COORDINATE).

local commands = require("lib.commands")

-- SearchFreeField's house number for "any field".
local ANY_HOUSE = 0xFFFF

-- The speed changes by magic of the speed words. max is ours, for
-- gamemasters: a speed 2000 above the normal one (TCreature::GetSpeed: two
-- per point of go strength).
local SPEED_FAST = 100
local SPEED_FASTEST = 200
local SPEED_MAX = 1000
local SPEEDS = {
	slow = 0,
	normal = 0,
	fast = SPEED_FAST,
	fastest = SPEED_FASTEST,
	max = SPEED_MAX,
}

-- The field offsets of the ways a creature faces.
local AHEAD = {
	[DIRECTION_NORTH] = {0, -1},
	[DIRECTION_EAST] = {1, 0},
	[DIRECTION_SOUTH] = {0, 1},
	[DIRECTION_WEST] = {-1, 0},
}

-- Moves the caster to the first free field within 1 of position that a jump
-- reaches and shows the teleport effect there; throws NOROOM when there is
-- none. The fields SearchFreeField takes: for ANY_HOUSE any, for house 0 only
-- fields of no house, for house H fields of no house or of house H.
local function moveTo(caster, position, houseId)
	local field = Game.searchFreeField(position, 1, houseId, true)
	if field == nil then
		Game.throwResult(RETURNVALUE_NOTENOUGHROOM)
	end

	Game.move(0, caster, Game.getMapContainer(field), -1, false, nil)
	field:sendMagicEffect(CONST_ME_TELEPORT)
end

-- Sets the caster's speed change by magic (the skill go strength's magic
-- delta) to delta, tells the clients and shows the blue sparkles on it.
local function setSpeed(caster, delta)
	caster:setSkillMagicDelta(SKILL_GOSTRENGTH, delta)
	Game.announceChangedCreature(caster:getId(), CREATURE_SPEED)
	caster:sendMagicEffect(CONST_ME_MAGIC_BLUE)
end

-- "/a": the caster that many fields ahead.
local function moveAhead(caster, command, word)
	if not caster:hasCommandRight(command, MOVE_RIGHT_COORDINATE) then
		return
	end

	local steps = tonumber(word)
	if steps == nil or steps ~= math.floor(steps) then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "Invalid number of fields.")
		return
	end

	local position = caster:getPosition()
	local offset = AHEAD[caster:getDirection()]
	local target = Position(position.x + offset[1] * steps, position.y + offset[2] * steps, position.z)
	if not Game.isOnMap(target) then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "You cannot teleport there.")
		return
	end

	moveTo(caster, target, ANY_HOUSE)
end

function onSay(caster, command, words, talk)
	local word = words[2]
	local position = caster:getPosition()
	local lowered = commands.lower(word)
	if talk == "/a" then
		moveAhead(caster, command, word)
		return
	end

	local speed = SPEEDS[lowered]
	local x, y, z = commands.scanCoordinates(word)
	if talk == "/speed" and speed == nil then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "Unknown speed. Use slow, normal, fast, fastest or max.")
		return
	elseif talk == "/pos" and x == nil then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "Invalid coordinates.")
		return
	elseif talk == "/town" and (speed ~= nil or x ~= nil or lowered == "up" or lowered == "down") then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "There is no mark of this name.")
		return
	end

	if lowered == "up" then
		if not caster:hasCommandRight(command, MOVE_RIGHT_VERTICAL) then
			return
		end

		moveTo(caster, Position(position.x, position.y, position.z - 1), ANY_HOUSE)
	elseif lowered == "down" then
		if not caster:hasCommandRight(command, MOVE_RIGHT_VERTICAL) then
			return
		end

		moveTo(caster, Position(position.x, position.y, position.z + 1), ANY_HOUSE)
	elseif speed ~= nil then
		if not caster:hasCommandRight(command, MOVE_RIGHT_SPEED) then
			return
		end

		setSpeed(caster, speed)
	elseif x ~= nil then
		if not caster:hasCommandRight(command, MOVE_RIGHT_COORDINATE) then
			return
		end

		if x < 1 or y < 1 or z < 1 or not Game.isOnMap(Position(x, y, z)) then
			local add32 = commands.add32
			local relative = Position(add32(x, position.x), add32(y, position.y), add32(z, position.z))
			if not Game.isOnMap(relative) then
				caster:sendTextMessage(MESSAGE_STATUS_SMALL, "Invalid coordinates.")
				return
			end

			moveTo(caster, relative, ANY_HOUSE)
		else
			moveTo(caster, Position(x, y, z), ANY_HOUSE)
		end
	else
		if not caster:hasCommandRight(command, MOVE_RIGHT_MARK) then
			return
		end

		local mark = Game.getMarkPosition(word)
		if mark == nil then
			caster:sendTextMessage(MESSAGE_STATUS_SMALL, "There is no mark of this name.")
			return
		end

		moveTo(caster, mark, Game.getHouseId(mark))
	end
end
