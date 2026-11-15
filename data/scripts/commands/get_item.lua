-- commands/get_item.lua: Get Item ("alevo para"), what
-- CharacterRightSpell's case SPELL_GET_ITEM did: creates one object named by the word.
return {
	on_say = function(caster, number, words)
		command.create_thing(caster, words[2], nil)
	end,
}
