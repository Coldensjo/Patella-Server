-- commands/get_item_count.lua: Get Item ("alevo para para"), what
-- CharacterRightSpell's case SPELL_GET_ITEM_COUNT did: creates the object named by the first word, as many as the second says.
function onSay(caster, command, words)
	Command.createThing(caster, words[2], words[3])
end
