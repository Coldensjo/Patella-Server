-- commands/start_monsterraid.lua: Start Monsterraid ("alevo mas res para"), what
-- CharacterRightSpell's case SPELL_START_MONSTERRAID did: starts the raid named.
function onSay(caster, command, words)
	Command.startMonsterraid(caster, words[4])
end
