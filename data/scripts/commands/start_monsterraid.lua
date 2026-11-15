-- commands/start_monsterraid.lua: Start Monsterraid ("alevo mas res para"), what
-- CharacterRightSpell's case SPELL_START_MONSTERRAID did: starts the raid named.
return {
	on_say = function(caster, number, words)
		command.start_monsterraid(caster, words[4])
	end,
}
