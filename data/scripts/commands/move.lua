-- commands/move.lua: Move ("alani para"), what Teleport (the original
-- magic.cc) did: by the word, each with the command's right for it, moves the
-- caster a floor up or down ("up", "down": MOVE_RIGHT_VERTICAL), sets its
-- speed ("fast", "fastest", "slow" or "normal": MOVE_RIGHT_SPEED), sends it to
-- a coordinate ("x,y,z" or "[x,y,z]": MOVE_RIGHT_COORDINATE; one with a number
-- below 1 or off the map counts from the caster's field) or to the mark of the
-- map the word names (any other word: MOVE_RIGHT_MARK). A move goes to the
-- first free field within 1 of the destination (one of the mark's house for a
-- mark), with the teleport effect there; none: NOROOM.

local commands = require("lib.commands")

-- SearchFreeField's house number for "any field".
local ANY_HOUSE = 0xFFFF

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

function onSay(caster, command, words)
	local word = words[2]
	local position = caster:getPosition()
	local lowered = commands.lower(word)
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
	elseif lowered == "fast" then
		if not caster:hasCommandRight(command, MOVE_RIGHT_SPEED) then
			return
		end

		setSpeed(caster, 100)
	elseif lowered == "fastest" then
		if not caster:hasCommandRight(command, MOVE_RIGHT_SPEED) then
			return
		end

		setSpeed(caster, 200)
	elseif lowered == "slow" or lowered == "normal" then
		if not caster:hasCommandRight(command, MOVE_RIGHT_SPEED) then
			return
		end

		setSpeed(caster, 0)
	else
		local x, y, z = commands.scanCoordinates(word)
		if x ~= nil then
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
end
