-- commands/create_gold.lua: Create Gold ("alevo vis para"), what
-- CharacterRightSpell's case SPELL_CREATE_GOLD did: creates the amount of money the word says.
function onSay(caster, command, words)
	Command.createMoney(caster, words[3])
end
