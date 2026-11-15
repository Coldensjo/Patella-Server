-- commands/get_item.lua: Get Item ("alevo para"), what
-- CharacterRightSpell's case SPELL_GET_ITEM did: creates one object named by the word.
function onSay(caster, command, words)
	Command.createThing(caster, words[2], nil)
end
