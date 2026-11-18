-- commands/get_experience.lua: Get Experience ("alevo cogni para"), what
-- CreateKnowledge (the original magic.cc) did for it: with the command's
-- right, gives the caster the number of experience points the word says (read
-- as a clamped strtol, Game.toInt) and shows the green sparkles on the caster;
-- a number that is not positive does nothing.

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local amount = Game.toInt(words[3])
	if amount <= 0 then
		return
	end

	caster:addSkillTries(SKILL_LEVEL, amount)
	caster:sendMagicEffect(CONST_ME_MAGIC_GREEN)
end
