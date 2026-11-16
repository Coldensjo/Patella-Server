-- lib/npc.lua: the NPC rule matcher, what TBehaviourDatabase::react and
-- TBehaviourDatabase::evaluate of crnonpl.cc did. An NPC's script
-- (npcs/<name>.lua) builds its rules with the constructors of this module
-- and hands every talk to behaviour.react (scripting.hh, "NPCs"):
--	local behaviour = require("lib.npc")
--	local rules = behaviour.rules { <rule>, ... }
--	function onCreatureSay(npc, creature, text, situation)
--		behaviour.react(npc, rules, text, situation)
--	end
--
-- The module returns one table, behaviour, which no script changes (reading
-- a name it does not have is an error, "unknown name 'behaviour.x'", and so
-- is writing one). It holds:
--   behaviour.rules(list) -> rules
--       A rule list made of list, an array whose elements are rules and rule
--       lists (a block's, lib/npcs/<name>.lua, put in place: its rules count
--       as the list's own, in order, so a previous in a block's first rule
--       names the rule before the block). Every element is checked here, at
--       load; a wrong one is a Lua error at the line that called
--       behaviour.rules, "behaviour.rules: rule <k>, <where>: <what>", k the
--       element's place in the list as written (a block counts as one).
--   behaviour.react(npc, rules, text, situation)
--       Matches text against the rules and runs the actions of the rule it
--       chooses, as react did (below).
--   The constructors and constants of the elements below.
--
-- A rule is {{<condition>, ...}, {<action>, ...}}; the conditions may be
-- empty (the rule holds always, with 0 conditions). The elements (the
-- original's syntax in brackets):
--   Conditions:
--     a string                 a word the text must contain ("hello$"),
--                              searched from where the previous word or
--                              number ended.
--     ADDRESS, BUSY, VANISH, MALE, FEMALE, KNIGHT, PALADIN, SORCERER, DRUID,
--     PREMIUM, PROMOTED, PZBLOCK, NONPVP, PVPENFORCED
--                              a property (ADDRESS also holds for
--                              ADDRESSQUEUE).
--     FORCE                    takes the rule at once if the conditions
--                              before it held (!).
--     P1, P2, P3               the first, second or third number in the text
--                              after the previous word or number (%1 to %3).
--     eq(a, b), ne(a, b), lt(a, b), gt(a, b), le(a, b), ge(a, b)
--                              a comparison of two values (=, <>, <, >, <=,
--                              >=).
--   Values (C++ ints):
--     a number                 an integer, -2^31 to 2^31 - 1.
--     P1, P2, P3, param(n)     the number a condition P1 to P3 took (%1 to
--                              %3; param(n), n an integer, for another n,
--                              which evaluate refuses when it runs).
--     TOPIC, PRICE, AMOUNT, TYPE, DATA
--                              the NPC's Topic, Price, Amount, TypeID, Data.
--     LEVEL, MAGLEVEL, HP, BURNING, POISON
--                              the interlocutor's skill (Burning and Poison:
--                              the skill's timer).
--     COUNTMONEY               the interlocutor's money (CountMoney).
--     count(v), spellKnown(v), spellLevel(v), questValue(v), random(a, b)
--                              (Count, SpellKnown, SpellLevel, QuestValue,
--                              Random).
--     add(a, b), sub(a, b), mul(a, b)
--                              a + b, a - b, a * b, wrapping as C++ ints do.
--   Actions:
--     a string                 a text the NPC says (%N, %A, %P, %T).
--     setTopic(v), setPrice(v), setAmount(v), setType(v), setData(v),
--     setHp(v)                 (Topic=, Price=, Amount=, Type=, Data=, HP=).
--     burning(a, b), poison(a, b), setQuestValue(a, b), effectMe(v),
--     effectOpp(v), profession(v), teachSpell(v), summon(v), create(v),
--     delete(v), teleport(x, y, z), startPosition(x, y, z)
--     createMoney, deleteMoney, queue, idle
--     nop                      no action (the original stored nothing for it).
--     previous                 the actions of the rule before, from its
--                              first; the actions after previous do not run
--                              (*).
-- A word or a text is a string without zero bytes; its bytes are the
-- original's (Latin-1). A constructor takes exactly its arguments; a
-- condition is no value and a value no condition (P1 to P3 are both).
--
-- The data (made once, at load, and never changed after: scripting.hh, rule
-- 9). An element (a constant of behaviour or what a constructor makes) is a
-- table with the private key TAG holding its role (ROLE_*) and the fields
-- name, kind, arity and given (the number of arguments it got), its
-- arguments at [1] to [given]. behaviour.rules checks the elements and
-- compiles each rule into
--   {count = <its number of conditions>, size = 2 * count,
--    conditions = {<kind>, <data>, <kind>, <data>, ...},
--    actions = {<action>, ...}}
-- a condition's kind one of C_* and its data the word (C_WORD), the number's
-- place 1 to 3 (C_NUMBER) or the comparison's node (C_COMPARISON); an action
-- is {<A_*>, <argument>, ...}, its arguments nodes, a text {A_SAY, <text>} or
-- {A_SAY_FORMAT, <pieces>} (nop leaves no action). A node is a number (a
-- constant) or {<N_*>, <left>, <right>} ({N_PARAM, n}; a leaf {<N_*>}). A
-- rule list is {[LIST] = true, count = <n>, <rule 1>, ..., <rule n>}, LIST a
-- private key, so a plain array is no rule list.
--
-- react keeps everything it needs in locals (the numbers the conditions take
-- travel as evaluate's arguments), so it may run nested: an action that
-- moves the interlocutor calls the NPC's handler again, inside the action.
-- It makes the calls of the C++ code it replaces, in the same order, with
-- the same arguments (scripting.hh, rule 6); its texts are main's.

-- The API, read once when the module loads (a name the engine lacks makes
-- the load fail, at start).
local debugPrint = debugPrint
local searchWord = Game.searchWord
local searchNumber = Game.searchNumber
local getWorldType = Game.getWorldType
local getRoundNumber = Game.getRoundNumber
local getTime = Game.getTime
local gamePrint = Game.print
local idiv = Game.idiv
local move = Game.move
local getMapContainer = Game.getMapContainer
local createMonsterOfRace = Game.createMonsterOfRace
local getResultOf = Game.getResultOf
local getSpellLevel = Magic.getSpellLevel
local Position = Position
local mathRandom = math.random
local floor = math.floor
local format = string.format
local byte = string.byte
local strsub = string.sub
local find = string.find
local tobit = bit.tobit
local band = bit.band
local arshift = bit.arshift
local lshift = bit.lshift
local type = type
local error = error
local pcall = pcall
local select = select
local tostring = tostring
local setmetatable = setmetatable

local SITUATION_ADDRESS = NPC_SITUATION_ADDRESS
local SITUATION_ADDRESSQUEUE = NPC_SITUATION_ADDRESSQUEUE
local SITUATION_BUSY = NPC_SITUATION_BUSY
local SITUATION_VANISH = NPC_SITUATION_VANISH
local STATE_IDLE = NPC_STATE_IDLE
local STATE_LEAVING = NPC_STATE_LEAVING
local SEX_MALE = PLAYERSEX_MALE
local SEX_FEMALE = PLAYERSEX_FEMALE
local WORLD_NO_PVP = WORLD_TYPE_NO_PVP
local WORLD_PVP_ENFORCED = WORLD_TYPE_PVP_ENFORCED
local SKILL_LEVEL = SKILL_LEVEL
local SKILL_MAGLEVEL = SKILL_MAGLEVEL
local SKILL_HITPOINTS = SKILL_HITPOINTS
local SKILL_BURNING = SKILL_BURNING
local SKILL_POISON = SKILL_POISON
local VOCATION_KNIGHT = VOCATION_KNIGHT
local VOCATION_PALADIN = VOCATION_PALADIN
local VOCATION_SORCERER = VOCATION_SORCERER
local VOCATION_DRUID = VOCATION_DRUID
local VOCATION_PROMOTION = VOCATION_PROMOTION
local RIGHT_PREMIUM_ACCOUNT = RIGHT_PREMIUM_ACCOUNT

-- react's limits: a number in the text counts as at most MAX_TEXT_NUMBER,
-- and a response of MAX_RESPONSE bytes or more is too long to say (react
-- refused it once Response[256] was set).
local MAX_TEXT_NUMBER = 500
local MAX_RESPONSE = 257
local INT_MIN = -2147483648
local INT_MAX = 2147483647

-- The kinds of a compiled condition (TCondition's Type; a property is a kind
-- of its own, its PROPERTY_ number).
local C_WORD = 1
local C_NUMBER = 2
local C_COMPARISON = 3
local C_FORCE = 4
local C_ADDRESS = 5
local C_BUSY = 6
local C_VANISH = 7
local C_MALE = 8
local C_FEMALE = 9
local C_KNIGHT = 10
local C_PALADIN = 11
local C_SORCERER = 12
local C_DRUID = 13
local C_PREMIUM = 14
local C_PROMOTED = 15
local C_PZBLOCK = 16
local C_NONPVP = 17
local C_PVPENFORCED = 18

-- The kinds of a node (TNode's Type; NODE_SKILL is one kind per skill). The
-- comparisons come first (N_LT to N_GE), then the arithmetic (N_ADD to
-- N_MUL); evaluate tests the groups by range.
local N_LT = 1
local N_GT = 2
local N_EQ = 3
local N_NE = 4
local N_LE = 5
local N_GE = 6
local N_ADD = 7
local N_SUB = 8
local N_MUL = 9
local N_PARAM = 10
local N_TOPIC = 11
local N_PRICE = 12
local N_AMOUNT = 13
local N_TYPE = 14
local N_DATA = 15
local N_LEVEL = 16
local N_MAGLEVEL = 17
local N_HP = 18
local N_BURNING = 19
local N_POISON = 20
local N_COUNT = 21
local N_COUNTMONEY = 22
local N_SPELLKNOWN = 23
local N_SPELLLEVEL = 24
local N_QUESTVALUE = 25
local N_RANDOM = 26

-- The kinds of a compiled action (TAction's Type and Number). A text without
-- %-replacements is A_SAY with the text, one with them A_SAY_FORMAT with its
-- pieces: strings and the T_ numbers of the replacements.
local A_SAY = 1
local A_SAY_FORMAT = 2
local A_TOPIC = 3
local A_PRICE = 4
local A_AMOUNT = 5
local A_TYPE = 6
local A_DATA = 7
local A_HP = 8
local A_BURNING = 9
local A_POISON = 10
local A_QUESTVALUE = 11
local A_EFFECTME = 12
local A_EFFECTOPP = 13
local A_PROFESSION = 14
local A_TEACHSPELL = 15
local A_SUMMON = 16
local A_CREATE = 17
local A_DELETE = 18
local A_CREATEMONEY = 19
local A_DELETEMONEY = 20
local A_QUEUE = 21
local A_IDLE = 22
local A_PREVIOUS = 23
local A_TELEPORT = 24
local A_STARTPOSITION = 25
local A_NOP = 26

local T_NAME = 1
local T_AMOUNT = 2
local T_PRICE = 3
local T_TIME = 4

-- The private keys of the elements and of the rule lists.
local TAG = {}
local LIST = {}

-- The roles of the elements.
local ROLE_PROPERTY = 1
local ROLE_FORCE = 2
local ROLE_NUMBER = 3
local ROLE_COMPARISON = 4
local ROLE_VALUE = 5
local ROLE_ACTION = 6
local ROLE_TEXT = {"the property", "the condition", "the number", "the comparison", "the value", "the action"}

-- 32-bit int arithmetic as the C++ code's on x86-64 (two's complement). The
-- operands are ints. A sum or difference is exact in a double, and tobit
-- wraps it. A product may need 62 bits, more than a double holds exactly,
-- so it is taken in two parts: a = high * 2^16 + low, a * b = (high * b) *
-- 2^16 + low * b, each part below 2^47 and wrapped to 32 bits.
local function add32(a, b)
	return tobit(a + b)
end

local function sub32(a, b)
	return tobit(a - b)
end

local function mul32(a, b)
	local low = band(a, 0xFFFF)
	local high = arshift(a, 16)
	return tobit(lshift(tobit(high * b), 16) + low * b)
end

-- TBehaviourDatabase::evaluate: the value of a node. Every call (the leaves'
-- too) first looks for the interlocutor, as evaluate did (GetPlayer of
-- npc->Interlocutor); o is that player, the C++ code's re-fetched Opp. n1,
-- n2 and n3 are react's Numbers. Both operands of a comparison or an
-- operation are evaluated, left first; random evaluates its maximum first.
local function evaluate(npc, node, n1, n2, n3)
	local o = npc:getInterlocutor()
	if o == nil then
		debugPrint("TBehaviourDatabase::evaluate: Interlocutor does not exist.\n")
		return 0
	end

	if type(node) == "number" then
		return node
	end

	local kind = node[1]
	if kind <= N_GE then
		local left = evaluate(npc, node[2], n1, n2, n3)
		local right = evaluate(npc, node[3], n1, n2, n3)
		local holds
		if kind == N_EQ then
			holds = left == right
		elseif kind == N_LT then
			holds = left < right
		elseif kind == N_GT then
			holds = left > right
		elseif kind == N_NE then
			holds = left ~= right
		elseif kind == N_LE then
			holds = left <= right
		else
			holds = left >= right
		end
		return holds and 1 or 0
	elseif kind <= N_MUL then
		local left = evaluate(npc, node[2], n1, n2, n3)
		local right = evaluate(npc, node[3], n1, n2, n3)
		if kind == N_ADD then
			return add32(left, right)
		elseif kind == N_SUB then
			return sub32(left, right)
		end
		return mul32(left, right)
	elseif kind == N_PARAM then
		local n = node[2]
		if n < 1 or n > 3 then
			debugPrint(format("TBehaviourDatabase::evaluate: Invalid number parameter %d.\n", n))
			return 0
		end
		local value = n3
		if n == 1 then
			value = n1
		elseif n == 2 then
			value = n2
		end
		if value < 0 then
			debugPrint(format("TBehaviourDatabase::evaluate: Number parameter %d not set.\n", n))
			return 0
		end
		return value
	elseif kind == N_TOPIC then
		return npc:getTopic()
	elseif kind == N_PRICE then
		return npc:getPrice()
	elseif kind == N_AMOUNT then
		return npc:getAmount()
	elseif kind == N_TYPE then
		return npc:getTypeId()
	elseif kind == N_DATA then
		return npc:getData()
	elseif kind == N_LEVEL then
		return o:getSkillValue(SKILL_LEVEL)
	elseif kind == N_MAGLEVEL then
		return o:getSkillValue(SKILL_MAGLEVEL)
	elseif kind == N_HP then
		return o:getSkillValue(SKILL_HITPOINTS)
	elseif kind == N_BURNING then
		return o:getSkillTimer(SKILL_BURNING)
	elseif kind == N_POISON then
		return o:getSkillTimer(SKILL_POISON)
	elseif kind == N_COUNT then
		-- CountInventoryObjects(npc->Interlocutor, <value>, npc->Data)
		local value = evaluate(npc, node[2], n1, n2, n3)
		return o:getItemCount(value, npc:getData())
	elseif kind == N_COUNTMONEY then
		return o:getMoney()
	elseif kind == N_SPELLKNOWN then
		local value = evaluate(npc, node[2], n1, n2, n3)
		return o:hasLearnedSpell(value) and 1 or 0
	elseif kind == N_SPELLLEVEL then
		return getSpellLevel(evaluate(npc, node[2], n1, n2, n3))
	elseif kind == N_QUESTVALUE then
		return o:getStorageValue(evaluate(npc, node[2], n1, n2, n3))
	end
	-- N_RANDOM: random(Min, Max) with Max evaluated first.
	local max = evaluate(npc, node[3], n1, n2, n3)
	local min = evaluate(npc, node[2], n1, n2, n3)
	return mathRandom(min, max)
end

-- The properties that ask the interlocutor or the world (the situations are
-- tested in react's loop): whether property kind holds for opp, the one test
-- of react's 14 that matches the kind.
local function propertyHolds(kind, opp)
	if kind == C_MALE then
		return opp:getSex() == SEX_MALE
	elseif kind == C_FEMALE then
		return opp:getSex() == SEX_FEMALE
	elseif kind == C_KNIGHT then
		return opp:getVocation():getBaseId() == VOCATION_KNIGHT
	elseif kind == C_PALADIN then
		return opp:getVocation():getBaseId() == VOCATION_PALADIN
	elseif kind == C_SORCERER then
		return opp:getVocation():getBaseId() == VOCATION_SORCERER
	elseif kind == C_DRUID then
		return opp:getVocation():getBaseId() == VOCATION_DRUID
	elseif kind == C_PREMIUM then
		return opp:hasRight(RIGHT_PREMIUM_ACCOUNT)
	elseif kind == C_PROMOTED then
		return not (opp:getVocation():getId() < VOCATION_PROMOTION)
	elseif kind == C_PZBLOCK then
		return opp:isPzLocked()
	elseif kind == C_NONPVP then
		return getWorldType() == WORLD_NO_PVP
	end
	-- C_PVPENFORCED
	return getWorldType() == WORLD_PVP_ENFORCED
end

-- The number a condition P1 to P3 takes: strtol of the text from at, where
-- SearchForNumber found a digit, capped at INT32_MAX and then at
-- MAX_TEXT_NUMBER; the number is digits only, so the cap at
-- MAX_TEXT_NUMBER is all that remains.
local function textNumber(text, at)
	local value = 0
	local b = byte(text, at)
	while b ~= nil and b >= 48 and b <= 57 do
		value = value * 10 + (b - 48)
		if value > MAX_TEXT_NUMBER then
			return MAX_TEXT_NUMBER
		end
		at = at + 1
		b = byte(text, at)
	end
	return value
end

-- The move of the action teleport, called through pcall: Move(0,
-- Opp->GetCreatureObject(), GetMapContainer(x, y, z), -1, false, NONE), both
-- calls inside react's try.
local function teleportMove(opp, x, y, z)
	move(0, opp, getMapContainer(Position(x, y, z)), -1, false, nil)
end

-- react's choice of the rule (its loop over the Behaviour): the index of the
-- fulfilled rule with the most conditions (the first of equals), or of the
-- first whose conditions before a FORCE held, and Numbers; 0 when no rule
-- holds. Numbers (n1 to n3) are set once per call: a number an earlier rule
-- took stays for the later rules and for the actions. Each rule's search
-- starts at the text's beginning (LastPos = Text).
local function chooseRule(npc, opp, rules, text, situation)
	local n1, n2, n3 = -1, -1, -1
	local maxConditions = -1
	local bestPattern = 0
	for i = 1, rules.count do
		local rule = rules[i]
		local conditions = rule.conditions
		local fulfilled = true
		local lastPos = 1
		for j = 1, rule.size, 2 do
			local kind = conditions[j]
			if kind == C_WORD then
				-- SearchForWord(word, LastPos); LastPos = Pos + strlen(word),
				-- the '$' of "hello$" counted: after such a word at the
				-- text's end LastPos is past the end, where a later search
				-- finds nothing (Game.searchWord).
				local word = conditions[j + 1]
				local at = searchWord(word, text, lastPos)
				if at == nil then
					fulfilled = false
					break
				end
				lastPos = at + #word
			elseif kind == C_ADDRESS then
				if situation ~= SITUATION_ADDRESS and situation ~= SITUATION_ADDRESSQUEUE then
					fulfilled = false
					break
				end
			elseif kind == C_BUSY then
				if situation ~= SITUATION_BUSY then
					fulfilled = false
					break
				end
			elseif kind == C_VANISH then
				if situation ~= SITUATION_VANISH then
					fulfilled = false
					break
				end
			elseif kind == C_COMPARISON then
				if evaluate(npc, conditions[j + 1], n1, n2, n3) == 0 then
					fulfilled = false
					break
				end
			elseif kind == C_NUMBER then
				-- SearchForNumber(n, LastPos); Numbers[n - 1] = the number;
				-- LastPos = Pos + 1
				local n = conditions[j + 1]
				local at = searchNumber(n, text, lastPos)
				if at == nil then
					fulfilled = false
					break
				end
				local value = textNumber(text, at)
				if n == 1 then
					n1 = value
				elseif n == 2 then
					n2 = value
				else
					n3 = value
				end
				lastPos = at + 1
			elseif kind == C_FORCE then
				-- goto Forced: no later condition, no later rule.
				return i, n1, n2, n3
			elseif not propertyHolds(kind, opp) then
				fulfilled = false
				break
			end
		end

		if fulfilled and rule.count > maxConditions then
			maxConditions = rule.count
			bestPattern = i
		end
	end
	return bestPattern, n1, n2, n3
end

-- react's ACTION_TEXT: the response built byte by byte from the text; a
-- replacement (%N, %A, %P, %T) asks for its value where it stands, and the
-- building stops once the response is too long, before the replacements
-- after that point. A too long response is logged with its first 20 bytes
-- and not said. Gives the new TalkDelay and whether the NPC says it.
local function sayText(npc, opp, action, talkDelay)
	local response = action[2]
	if action[1] == A_SAY_FORMAT then
		local pieces = response
		response = ""
		for p = 1, #pieces do
			local piece = pieces[p]
			if piece == T_NAME then
				piece = opp:getName()
			elseif piece == T_AMOUNT then
				piece = format("%d", npc:getAmount())
			elseif piece == T_PRICE then
				piece = format("%d", npc:getPrice())
			elseif piece == T_TIME then
				local hour, minute = getTime()
				piece = format(hour < 12 and "%d:%02d am" or "%d:%02d pm", (hour + 11) % 12 + 1, minute)
			end
			response = response .. piece
			if #response >= MAX_RESPONSE then
				break
			end
		end
	end

	if #response >= MAX_RESPONSE then
		debugPrint(format("TBehaviourDatabase::react: Text of NPC %s gets too long (%s...).\n",
				npc:getName(), strsub(response, 1, 20)))
		return talkDelay, false
	end
	npc:toDoWait(talkDelay)
	npc:toDoSay(response)
	return talkDelay + 3100 + floor(#response / 2) * 100, true
end

-- react from Forced: on: the actions of rule bestPattern, then the start of
-- the to-do list.
local function runActions(npc, opp, rules, bestPattern, text, situation, n1, n2, n3)
	if situation ~= SITUATION_BUSY then
		npc:setTopic(0)
	end

	local startToDo = false
	local talkDelay = 1000
	local actions = rules[bestPattern].actions
	local j = 1
	while true do
		local action = actions[j]
		if action == nil then
			break
		end

		local kind = action[1]
		if kind == A_SAY or kind == A_SAY_FORMAT then
			local said
			talkDelay, said = sayText(npc, opp, action, talkDelay)
			if said then
				startToDo = true
			end
		elseif kind == A_TOPIC then
			-- ACTION_SET_VARIABLE
			npc:setTopic(evaluate(npc, action[2], n1, n2, n3))
		elseif kind == A_PRICE then
			npc:setPrice(evaluate(npc, action[2], n1, n2, n3))
		elseif kind == A_AMOUNT then
			npc:setAmount(evaluate(npc, action[2], n1, n2, n3))
		elseif kind == A_TYPE then
			npc:setTypeId(evaluate(npc, action[2], n1, n2, n3))
		elseif kind == A_DATA then
			npc:setData(evaluate(npc, action[2], n1, n2, n3))
		elseif kind == A_HP then
			-- ACTION_SET_SKILL
			opp:setHealth(evaluate(npc, action[2], n1, n2, n3))
			if opp:getHealth() < 1 then
				debugPrint(format("TBehaviourDatabase::react: NPC %s kills player.\n", npc:getName()))
				opp:die()
			end
		elseif kind == A_BURNING or kind == A_POISON then
			-- ACTION_SET_TIMER
			local value = evaluate(npc, action[2], n1, n2, n3)
			local value2 = evaluate(npc, action[3], n1, n2, n3)
			local skill = kind == A_BURNING and SKILL_BURNING or SKILL_POISON
			if value == 0 or opp:getSkillTimer(skill) < value then
				opp:setSkillTimer(skill, value, value2, value2, -1)
				opp:setDamageOrigin(skill, nil)
			end
		elseif kind == A_QUESTVALUE then
			-- ACTION_FUNCTION2
			local value = evaluate(npc, action[2], n1, n2, n3)
			local value2 = evaluate(npc, action[3], n1, n2, n3)
			opp:setStorageValue(value, value2)
		elseif kind <= A_DELETE then
			-- ACTION_FUNCTION1 (A_EFFECTME to A_DELETE)
			local value = evaluate(npc, action[2], n1, n2, n3)
			if kind == A_EFFECTME then
				npc:sendMagicEffect(value)
			elseif kind == A_EFFECTOPP then
				opp:sendMagicEffect(value)
			elseif kind == A_PROFESSION then
				opp:setVocation(value)
			elseif kind == A_TEACHSPELL then
				opp:learnSpell(value)
			elseif kind == A_SUMMON then
				createMonsterOfRace(value, npc:getPosition())
			elseif kind == A_CREATE then
				npc:giveItems(value, npc:getAmount())
			else
				npc:takeItems(value, npc:getAmount())
			end
		elseif kind == A_CREATEMONEY then
			-- ACTION_FUNCTION0
			npc:giveMoney(npc:getPrice())
		elseif kind == A_DELETEMONEY then
			npc:takeMoney(npc:getPrice())
		elseif kind == A_QUEUE then
			if situation ~= SITUATION_BUSY then
				debugPrint("TBehaviourDatabase::react (6): wrong situation for action \"Queue\".\n")
			end
			npc:enqueue(opp, text)
		elseif kind == A_IDLE then
			-- ACTION_CHANGE_STATE (idle is its only state)
			if not startToDo then
				npc:setState(STATE_IDLE)
				if situation == SITUATION_ADDRESSQUEUE then
					debugPrint(format("TBehaviourDatabase::react: NPC %s does not respond to greeting %s.\n",
							npc:getName(), text))
				else
					startToDo = true
				end
			else
				npc:setState(STATE_LEAVING)
				npc:toDoChangeState(STATE_IDLE)
			end
		elseif kind == A_PREVIOUS then
			-- ACTION_PREVIOUS_RULE: goto PreviousPattern, the actions of the
			-- rule before from its first; the rest of this rule's actions
			-- are not run.
			if bestPattern == 1 then
				debugPrint("TBehaviourDatabase::react (9): No previous pattern.\n")
			else
				bestPattern = bestPattern - 1
				actions = rules[bestPattern].actions
				j = 0
			end
		elseif kind == A_TELEPORT then
			-- ACTION_FUNCTION3, FUNCTION_TELEPORT: a result code from the
			-- move is logged (catch (RESULT r)); any other error goes on.
			local x = evaluate(npc, action[2], n1, n2, n3)
			local y = evaluate(npc, action[3], n1, n2, n3)
			local z = evaluate(npc, action[4], n1, n2, n3)
			gamePrint(3, format("NPC teleports interlocutor to [%d,%d,%d].\n", x, y, z))
			local ok, err = pcall(teleportMove, opp, x, y, z)
			if not ok then
				local code = getResultOf(err)
				if code == nil then
					error(err, 0)
				end
				debugPrint(format("TBehaviourDatabase::react (10): Exception %d.\n", code))
			end
		else
			-- A_STARTPOSITION: ACTION_FUNCTION3, FUNCTION_STARTPOSITION.
			local x = evaluate(npc, action[2], n1, n2, n3)
			local y = evaluate(npc, action[3], n1, n2, n3)
			local z = evaluate(npc, action[4], n1, n2, n3)
			gamePrint(3, format("NPC sets start coordinate for interlocutor to [%d,%d,%d].\n", x, y, z))
			if opp:isPlayer() then
				opp:setStartPosition(Position(x, y, z))
			else
				debugPrint("TBehaviourDatabase::react (10): Interlocutor is not a player.\n")
			end
		end
		j = j + 1
	end

	if not startToDo then
		return
	end

	npc:toDoWait(talkDelay)
	npc:toDoStart()
	if situation ~= SITUATION_BUSY then
		npc:setLastTalk(getRoundNumber() + idiv(talkDelay, 1000))
	end
end

-- TBehaviourDatabase::react.
local function react(npc, rules, text, situation)
	if type(rules) ~= "table" or rules[LIST] ~= true then
		error("behaviour.react: rules is no rule list (behaviour.rules makes one)", 2)
	end

	if npc == nil then
		debugPrint("TBehaviourDatabase::react: NPC does not exist.\n")
		return
	end

	if text == nil then
		debugPrint("TBehaviourDatabase::react: Given text does not exist.\n")
		return
	end

	-- react's Opp: the interlocutor at the start, for the whole call.
	local opp = npc:getInterlocutor()
	if opp == nil then
		debugPrint(format("TBehaviourDatabase::react: Interlocutor does not exist (Text=%s, Situation=%d).\n",
				text, situation))
		return
	end

	-- No rule holds and none was forced: nothing happens (Topic stays).
	local bestPattern, n1, n2, n3 = chooseRule(npc, opp, rules, text, situation)
	if bestPattern == 0 then
		return
	end
	runActions(npc, opp, rules, bestPattern, text, situation, n1, n2, n3)
end

-- Checking and compiling the elements (behaviour.rules). A wrong element
-- raises its message without a position; behaviour.rules adds the position
-- of its caller.

local function fail(...)
	error(format(...), 0)
end

-- What a wrong element is, for the messages (never an address: rule 4).
local function describe(value)
	local t = type(value)
	if t == "table" then
		local role = value[TAG]
		if role ~= nil then
			return ROLE_TEXT[role] .. " " .. value.name
		end
		if value[LIST] == true then
			return "a rule list"
		end
		return "a table that is no element of behaviour"
	elseif t == "function" then
		return "a function (a constructor of behaviour without its arguments?)"
	elseif t == "string" then
		return "a string"
	elseif t == "number" then
		return "the number " .. tostring(value)
	elseif t == "nil" then
		return "nil"
	end
	return "a " .. t
end

local function isInt(value)
	return type(value) == "number" and value == floor(value) and value >= INT_MIN and value <= INT_MAX
end

-- The arguments of a constructor's element: exactly its arity.
local function checkArity(element, where)
	if element.given ~= element.arity then
		fail("%s: %s takes %d argument%s, not %d", where, element.name, element.arity,
				element.arity == 1 and "" or "s", element.given)
	end
end

local function checkString(text, where, what)
	if find(text, "\0", 1, true) ~= nil then
		fail("%s: a zero byte in a %s", where, what)
	end
end

-- A value: a number, or a node.
local function compileValue(value, where)
	if type(value) == "number" then
		if not isInt(value) then
			fail("%s: %s is no integer from -2147483648 to 2147483647", where, tostring(value))
		end
		return value
	end
	if type(value) == "string" then
		fail("%s: a string where a value belongs", where)
	end

	local role = type(value) == "table" and value[TAG] or nil
	if role == ROLE_NUMBER then
		return value.node
	end
	if role ~= ROLE_VALUE then
		fail("%s: %s where a value belongs", where, describe(value))
	end

	if value.arity == 0 then
		return value.node
	end
	checkArity(value, where)
	if value.kind == N_PARAM then
		-- param(n): n is the number's place, an integer, not a value.
		if not isInt(value[1]) then
			fail("%s: param's argument is an integer, not %s", where, describe(value[1]))
		end
		return {N_PARAM, value[1]}
	end
	local node = {value.kind}
	for k = 1, value.arity do
		node[k + 1] = compileValue(value[k], format("%s: %s, argument %d", where, value.name, k))
	end
	return node
end

-- A condition: its kind and data.
local function compileCondition(condition, where)
	if type(condition) == "string" then
		checkString(condition, where, "word")
		return C_WORD, condition
	end

	local role = type(condition) == "table" and condition[TAG] or nil
	if role == ROLE_PROPERTY or role == ROLE_FORCE then
		return condition.kind, 0
	elseif role == ROLE_NUMBER then
		return C_NUMBER, condition.number
	elseif role == ROLE_COMPARISON then
		checkArity(condition, where)
		local left = compileValue(condition[1], format("%s: %s, argument 1", where, condition.name))
		local right = compileValue(condition[2], format("%s: %s, argument 2", where, condition.name))
		return C_COMPARISON, {condition.kind, left, right}
	end
	fail("%s: %s where a condition belongs", where, describe(condition))
end

-- A text: A_SAY with the text when it has no replacements, A_SAY_FORMAT
-- with its pieces otherwise. A '%' takes the byte after it: N, A, P and T are
-- the replacements; any other byte (also the text's end) adds nothing, as
-- react's switch did.
local function compileText(text, where)
	checkString(text, where, "text")
	local pieces = {}
	local start = 1
	local pos = 1
	local length = #text
	local replacements = false
	local function addString(piece)
		if piece ~= "" then
			local last = #pieces
			if last > 0 and type(pieces[last]) == "string" then
				pieces[last] = pieces[last] .. piece
			else
				pieces[last + 1] = piece
			end
		end
	end
	while pos <= length do
		if byte(text, pos) == 37 then
			addString(strsub(text, start, pos - 1))
			local code = byte(text, pos + 1)
			local replacement = nil
			if code == 78 then
				replacement = T_NAME
			elseif code == 65 then
				replacement = T_AMOUNT
			elseif code == 80 then
				replacement = T_PRICE
			elseif code == 84 then
				replacement = T_TIME
			end
			if replacement ~= nil then
				pieces[#pieces + 1] = replacement
				replacements = true
			end
			pos = pos + 2
			start = pos
		else
			pos = pos + 1
		end
	end
	addString(strsub(text, start, length))

	if replacements then
		return {A_SAY_FORMAT, pieces}
	end
	return {A_SAY, pieces[1] or ""}
end

-- An action: its compiled table, nil for nop.
local function compileAction(action, where)
	if type(action) == "string" then
		return compileText(action, where)
	end

	local role = type(action) == "table" and action[TAG] or nil
	if role ~= ROLE_ACTION then
		fail("%s: %s where an action belongs", where, describe(action))
	end
	if action.kind == A_NOP then
		return nil
	end
	checkArity(action, where)
	local compiled = {action.kind}
	for k = 1, action.arity do
		compiled[k + 1] = compileValue(action[k], format("%s: %s, argument %d", where, action.name, k))
	end
	return compiled
end

local function isPlainTable(value)
	return type(value) == "table" and value[TAG] == nil and value[LIST] == nil
end

local function compileRule(rule, where)
	if not isPlainTable(rule) or not isPlainTable(rule[1]) or not isPlainTable(rule[2]) or rule[3] ~= nil then
		fail("%s: a rule is {{<condition>, ...}, {<action>, ...}}, not %s", where, describe(rule))
	end

	local conditions = {}
	local count = #rule[1]
	for k = 1, count do
		local kind, data = compileCondition(rule[1][k], format("%s, condition %d", where, k))
		conditions[2 * k - 1] = kind
		conditions[2 * k] = data
	end

	local actions = {}
	for k = 1, #rule[2] do
		local action = compileAction(rule[2][k], format("%s, action %d", where, k))
		if action ~= nil then
			actions[#actions + 1] = action
		end
	end

	return {count = count, size = 2 * count, conditions = conditions, actions = actions}
end

local function compileList(list)
	if not isPlainTable(list) then
		fail("the rules are an array of rules and rule lists, not %s", describe(list))
	end

	local rules = {[LIST] = true}
	local count = 0
	for k = 1, #list do
		local element = list[k]
		if type(element) == "table" and element[LIST] == true then
			for i = 1, element.count do
				count = count + 1
				rules[count] = element[i]
			end
		else
			count = count + 1
			rules[count] = compileRule(element, format("rule %d", k))
		end
	end
	rules.count = count
	return rules
end

-- behaviour.rules(list): the checked and compiled rule list.
local function rules(list)
	local ok, result = pcall(compileList, list)
	if not ok then
		if type(result) == "string" then
			error("behaviour.rules: " .. result, 2)
		end
		error(result, 0)
	end
	return result
end

-- The elements.

local fields = {rules = rules, react = react}

local function constant(role, kind, name)
	local element = {[TAG] = role, kind = kind, name = name, arity = 0, given = 0}
	fields[name] = element
	return element
end

local function constructor(role, kind, name, arity)
	fields[name] = function(...)
		return {[TAG] = role, kind = kind, name = name, arity = arity, given = select("#", ...), ...}
	end
end

constant(ROLE_PROPERTY, C_ADDRESS, "ADDRESS")
constant(ROLE_PROPERTY, C_BUSY, "BUSY")
constant(ROLE_PROPERTY, C_VANISH, "VANISH")
constant(ROLE_PROPERTY, C_MALE, "MALE")
constant(ROLE_PROPERTY, C_FEMALE, "FEMALE")
constant(ROLE_PROPERTY, C_KNIGHT, "KNIGHT")
constant(ROLE_PROPERTY, C_PALADIN, "PALADIN")
constant(ROLE_PROPERTY, C_SORCERER, "SORCERER")
constant(ROLE_PROPERTY, C_DRUID, "DRUID")
constant(ROLE_PROPERTY, C_PREMIUM, "PREMIUM")
constant(ROLE_PROPERTY, C_PROMOTED, "PROMOTED")
constant(ROLE_PROPERTY, C_PZBLOCK, "PZBLOCK")
constant(ROLE_PROPERTY, C_NONPVP, "NONPVP")
constant(ROLE_PROPERTY, C_PVPENFORCED, "PVPENFORCED")
constant(ROLE_FORCE, C_FORCE, "FORCE")

-- P1 to P3: a condition (the number's place) and a value (its node).
for n = 1, 3 do
	local element = constant(ROLE_NUMBER, C_NUMBER, "P" .. n)
	element.number = n
	element.node = {N_PARAM, n}
end

constructor(ROLE_COMPARISON, N_EQ, "eq", 2)
constructor(ROLE_COMPARISON, N_NE, "ne", 2)
constructor(ROLE_COMPARISON, N_LT, "lt", 2)
constructor(ROLE_COMPARISON, N_GT, "gt", 2)
constructor(ROLE_COMPARISON, N_LE, "le", 2)
constructor(ROLE_COMPARISON, N_GE, "ge", 2)

-- The values without arguments hold their node.
local function leaf(kind, name)
	constant(ROLE_VALUE, kind, name).node = {kind}
end

leaf(N_TOPIC, "TOPIC")
leaf(N_PRICE, "PRICE")
leaf(N_AMOUNT, "AMOUNT")
leaf(N_TYPE, "TYPE")
leaf(N_DATA, "DATA")
leaf(N_LEVEL, "LEVEL")
leaf(N_MAGLEVEL, "MAGLEVEL")
leaf(N_HP, "HP")
leaf(N_BURNING, "BURNING")
leaf(N_POISON, "POISON")
leaf(N_COUNTMONEY, "COUNTMONEY")

constructor(ROLE_VALUE, N_PARAM, "param", 1)
constructor(ROLE_VALUE, N_COUNT, "count", 1)
constructor(ROLE_VALUE, N_SPELLKNOWN, "spellKnown", 1)
constructor(ROLE_VALUE, N_SPELLLEVEL, "spellLevel", 1)
constructor(ROLE_VALUE, N_QUESTVALUE, "questValue", 1)
constructor(ROLE_VALUE, N_RANDOM, "random", 2)
constructor(ROLE_VALUE, N_ADD, "add", 2)
constructor(ROLE_VALUE, N_SUB, "sub", 2)
constructor(ROLE_VALUE, N_MUL, "mul", 2)

constructor(ROLE_ACTION, A_TOPIC, "setTopic", 1)
constructor(ROLE_ACTION, A_PRICE, "setPrice", 1)
constructor(ROLE_ACTION, A_AMOUNT, "setAmount", 1)
constructor(ROLE_ACTION, A_TYPE, "setType", 1)
constructor(ROLE_ACTION, A_DATA, "setData", 1)
constructor(ROLE_ACTION, A_HP, "setHp", 1)
constructor(ROLE_ACTION, A_BURNING, "burning", 2)
constructor(ROLE_ACTION, A_POISON, "poison", 2)
constructor(ROLE_ACTION, A_QUESTVALUE, "setQuestValue", 2)
constructor(ROLE_ACTION, A_EFFECTME, "effectMe", 1)
constructor(ROLE_ACTION, A_EFFECTOPP, "effectOpp", 1)
constructor(ROLE_ACTION, A_PROFESSION, "profession", 1)
constructor(ROLE_ACTION, A_TEACHSPELL, "teachSpell", 1)
constructor(ROLE_ACTION, A_SUMMON, "summon", 1)
constructor(ROLE_ACTION, A_CREATE, "create", 1)
constructor(ROLE_ACTION, A_DELETE, "delete", 1)
constant(ROLE_ACTION, A_CREATEMONEY, "createMoney")
constant(ROLE_ACTION, A_DELETEMONEY, "deleteMoney")
constant(ROLE_ACTION, A_QUEUE, "queue")
constant(ROLE_ACTION, A_IDLE, "idle")
constant(ROLE_ACTION, A_NOP, "nop")
constant(ROLE_ACTION, A_PREVIOUS, "previous")
constructor(ROLE_ACTION, A_TELEPORT, "teleport", 3)
constructor(ROLE_ACTION, A_STARTPOSITION, "startPosition", 3)

-- behaviour: the fields, read-only, an unknown name an error.
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
			error(format("unknown name 'behaviour.%s'", keyText(key)), 2)
		end
		return value
	end,
	__newindex = function(_, key)
		error(format("behaviour.%s is read-only", keyText(key)), 2)
	end,
	__metatable = false,
})
