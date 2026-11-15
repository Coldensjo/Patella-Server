-- commands/kick_player.lua: Kick Player ("omana para"), what
-- AccountRightSpell's case SPELL_KICK_PLAYER did: logs the player named out.
return {
	on_say = function(caster, number, words)
		command.kick_player(caster, words[2])
	end,
}
