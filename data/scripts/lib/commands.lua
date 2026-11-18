-- lib/commands.lua: the helpers the command scripts share (commands/<name>.lua;
-- scripting.hh, section "Commands in Lua (phase 9 ...)"). A module:
--	local commands = require("lib.commands")
-- The table it returns holds the functions below and is read-only (rule 9):
-- reading a name it does not have is an error ("unknown name
-- 'commands.x'"), and so is writing one.
--   commands.lower(s)               s in lower case as strLower does it.
--   commands.add32(a, b)            a + b of C++ ints.
--   commands.scanCoordinates(text)  the three numbers of "x,y,z" or
--                                   "[x,y,z]", or nil.
--   commands.identifyPlayer(caster, name)
--                                   the player online that name names.
--   commands.findRace(name)         the race number of a race's name, or 0.

local tobit = bit.tobit
local byte = string.byte
local find = string.find
local format = string.format
local gsub = string.gsub
local sub = string.sub

-- The bytes strLower changes, each to its lower case: A to Z and the Latin-1
-- bytes 0xC0 to 0xDE (0xD7, the multiplication sign, too), each plus 32.
local LOWER_CASE = {}
for c = 0x41, 0x5A do
	LOWER_CASE[string.char(c)] = string.char(c + 32)
end
for c = 0xC0, 0xDE do
	LOWER_CASE[string.char(c)] = string.char(c + 32)
end

-- s up to its first zero byte, the C string a char * of s holds.
local function cString(s)
	local zero = find(s, "\0", 1, true)
	if zero ~= nil then
		return sub(s, 1, zero - 1)
	end
	return s
end

-- lower(s): s as strLower (mudinc.cc) leaves it: each byte A to Z and 0xC0 to
-- 0xDE plus 32 (toLower), every other byte as it is. strLower stops at the
-- first zero byte, so the bytes from there on stay as they are. Gives the
-- new string. stricmp(a, b, -1) == 0 of two strings without a zero byte is
-- lower(a) == lower(b): stricmp compares toLower of each byte up to the end of
-- the first, and only a zero byte has a toLower of 0.
local function lower(s)
	local text = cString(s)
	local lowered = gsub(text, "[\65-\90\192-\222]", LOWER_CASE)
	if #text == #s then
		return lowered
	end
	return lowered .. sub(s, #text + 1)
end

-- add32(a, b): a + b of two C++ ints (-2^31 to 2^31 - 1) with the game's
-- wrap-around (it is built with -fwrapv): the sum in -2^31 to 2^31 - 1, the
-- low 32 bits of the exact sum (which a double holds) as a signed int.
local function add32(a, b)
	return tobit(a + b)
end

-- A %d of sscanf at position i of text: the white space it skips (isspace of
-- the C locale: space, \t, \n, \v, \f, \r), a sign and at least one digit.
-- Gives the int it stores, (int)strtol of the sign and the digits
-- (Game.atoi), and the position after the digits; nil when there is no
-- digit there (a matching failure).
local function scanInt(text, i)
	local _, last, number = find(text, "^[ \t\n\v\f\r]*([+-]?[0-9]+)", i)
	if last == nil then
		return nil
	end
	return Game.atoi(number), last + 1
end

-- The three numbers of "%d,%d,%d" at position i of text (each ',' must be
-- the next byte), or nil when fewer than three are stored. What follows the
-- third number is not looked at (sscanf's count is 3 then).
local function scanTriple(text, i)
	local x, y, z
	x, i = scanInt(text, i)
	if x == nil or byte(text, i) ~= 44 then
		return nil
	end
	y, i = scanInt(text, i + 1)
	if y == nil or byte(text, i) ~= 44 then
		return nil
	end
	z = scanInt(text, i + 1)
	if z == nil then
		return nil
	end
	return x, y, z
end

-- scanCoordinates(text) -> x, y, z, or nil: the three ints that
--	sscanf(Text, "%d,%d,%d", &x, &y, &z) == 3
--		|| sscanf(Text, "[%d,%d,%d]", &x, &y, &z) == 3
-- leaves in x, y and z when it holds (Text the C string of text), nil when it
-- does not. A first scan that stores only one or two numbers leaves no trace
-- in the result: the second scan stores all three only for a text that
-- begins with '[', and on such a text the first stores none; and where
-- neither stores three, nil (the caller does not use x, y and z then; Move
-- reads a mark's coordinates instead). The ']' after the third number is not
-- needed (sscanf's count is 3 before it is looked at).
local function scanCoordinates(text)
	text = cString(text)
	local x, y, z = scanTriple(text, 1)
	if x ~= nil then
		return x, y, z
	end
	if byte(text, 1) == 91 then
		x, y, z = scanTriple(text, 2)
		if x ~= nil then
			return x, y, z
		end
	end
	return nil
end

-- identifyPlayer(caster, name) -> the Player: what the commands that name a
-- player (Teleport to Friend, Retrieve Friend, Temple Teleport, Kick Player,
-- Kick Guest) did with IdentifyPlayer, all with the same arguments:
--	IdentifyPlayer(Name, false, !CheckRight(ID, READ_GAMEMASTER_CHANNEL), pl)
-- (the right is checked before the player is looked for), and for -1 throw
-- PLAYERNOTONLINE, for -2 throw NAMEAMBIGUOUS. IdentifyPlayer gives only 0,
-- -1 and -2.
local function identifyPlayer(caster, name)
	local result, player = Game.identifyPlayer(name, false, not caster:hasRight(RIGHT_READ_GAMEMASTER_CHANNEL))
	if result == -1 then
		Game.throwResult(RETURNVALUE_PLAYERWITHTHISNAMEISNOTONLINE)
	end
	if result == -2 then
		Game.throwResult(RETURNVALUE_NAMEISTOOAMBIGUOUS)
	end
	return player
end

-- findRace(name) -> the race number: the first race i from 1 to MAX_RACES - 1
-- with stricmp(name, GetRaceName(i), -1) == 0 (lower(name) ==
-- lower(the race's name)), 0 when there is none, as Summon Wild Creature
-- looked the race up. A race number without a race (MonsterType gives nil:
-- its name is empty) cannot match, as name is not empty (a word never is).
local function findRace(name)
	local wanted = lower(name)
	for race = 1, MAX_RACES - 1 do
		local monsterType = MonsterType(race)
		if monsterType ~= nil and lower(monsterType:name()) == wanted then
			return race
		end
	end
	return 0
end

local fields = {
	lower = lower,
	add32 = add32,
	scanCoordinates = scanCoordinates,
	identifyPlayer = identifyPlayer,
	findRace = findRace,
}

-- commands: the fields, read-only, an unknown name an error.
local function keyText(key)
	if type(key) == "string" then
		return key
	end
	return "<" .. type(key) .. ">"
end

return setmetatable({}, {
	__index = function(_, key)
		local value = fields[key]
		if value == nil then
			error(format("unknown name 'commands.%s'", keyText(key)), 2)
		end
		return value
	end,
	__newindex = function(_, key)
		error(format("commands.%s is read-only", keyText(key)), 2)
	end,
	__metatable = false,
})
