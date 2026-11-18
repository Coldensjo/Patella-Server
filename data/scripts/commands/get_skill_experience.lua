-- commands/get_skill_experience.lua: Get Skill Experience ("alevo cogni para
-- para"), what CreateKnowledge (the original magic.cc) did for it: with the
-- command's right, gives the caster's skill numbered by the first word (0,
-- SKILL_LEVEL, to SKILL_LAST) as many points as the second word says (both
-- read as a clamped strtol, Game.toInt) and shows the green sparkles on the
-- caster; a skill number out of range or a number of points that is not
-- positive does nothing.

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local skill = Game.toInt(words[3])
	local amount = Game.toInt(words[4])
	if skill < 0 or skill > SKILL_LAST then
		return
	end

	if amount <= 0 then
		return
	end

	caster:addSkillTries(skill, amount)
	caster:sendMagicEffect(CONST_ME_MAGIC_GREEN)
end
