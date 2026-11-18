-- commands/create_gold.lua: Create Gold ("alevo vis para"), what CreateMoney
-- (the original magic.cc) did: with the command's right, creates at the caster
-- the amount of money the word says (1 to 1,000,000), as coins of 10,000, of
-- 100 and of 1 (the special objects MONEY_TENTHOUSAND, MONEY_HUNDRED,
-- MONEY_ONE; as few as hold it), and shows the green sparkles on the caster's
-- field. Another amount is told the caster.

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local count = Game.atoi(words[3])
	if count < 1 or count > 1000000 then
		caster:sendTextMessage(MESSAGE_STATUS_SMALL, "You may only create 1 to 1,000,000 gold.")
		return
	end

	-- count is positive here, so math.floor divides as C++ ints do.
	local container = caster:getMapContainer()
	if math.floor(count / 10000) > 0 then
		Game.createAtCreature(caster:getId(), Game.getSpecialObject(SPECIAL_MONEY_TENTHOUSAND),
				math.floor(count / 10000))
	end

	if math.floor(count % 10000 / 100) > 0 then
		Game.createAtCreature(caster:getId(), Game.getSpecialObject(SPECIAL_MONEY_HUNDRED),
				math.floor(count % 10000 / 100))
	end

	if count % 100 > 0 then
		Game.createAtCreature(caster:getId(), Game.getSpecialObject(SPECIAL_MONEY_ONE), count % 100)
	end

	Item.sendMagicEffect(container, CONST_ME_MAGIC_GREEN)
end
