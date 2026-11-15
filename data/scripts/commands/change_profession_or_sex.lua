-- commands/change_profession_or_sex.lua: Change Profession or Sex ("aleta vita para"), what
-- CharacterRightSpell's case SPELL_CHANGE_PROFESSION_OR_SEX did: changes the caster's sex or profession to the word, or promotes it.
function onSay(caster, command, words)
	Command.changeProfession(caster, words[3])
end
