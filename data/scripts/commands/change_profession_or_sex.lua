-- commands/change_profession_or_sex.lua: Change Profession or Sex ("aleta vita
-- para"), what ChangeProfession (the original magic.cc) did: with the
-- command's right, by the word (in any case): "male" or "female" sets the
-- caster's sex, "none" clears its profession, "knight", "paladin", "sorcerer"
-- or "druid" clears it and sets that one, "promote" promotes a caster that has
-- a profession and is not promoted; each shows the blue sparkles on the
-- caster. Another word, or "promote" for a caster it does not fit, does
-- nothing.

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	-- stricmp with a lower-case ASCII word: the same as comparing the lower
	-- case of the word.
	local word = string.lower(words[3])
	if word == "male" then
		caster:setSex(PLAYERSEX_MALE)
	elseif word == "female" then
		caster:setSex(PLAYERSEX_FEMALE)
	elseif word == "none" then
		caster:clearVocation()
	elseif word == "knight" then
		caster:clearVocation()
		caster:setVocation(VOCATION_KNIGHT)
	elseif word == "paladin" then
		caster:clearVocation()
		caster:setVocation(VOCATION_PALADIN)
	elseif word == "sorcerer" then
		caster:clearVocation()
		caster:setVocation(VOCATION_SORCERER)
	elseif word == "druid" then
		caster:clearVocation()
		caster:setVocation(VOCATION_DRUID)
	elseif word == "promote" then
		local vocation = caster:getVocation()
		if vocation:getId() == VOCATION_NONE or vocation:getId() >= VOCATION_PROMOTION then
			return
		end

		caster:setVocation(VOCATION_PROMOTION)
	else
		return
	end

	caster:sendMagicEffect(CONST_ME_MAGIC_BLUE)
end
