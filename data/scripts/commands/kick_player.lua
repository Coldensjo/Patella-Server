-- commands/kick_player.lua: Kick Player ("omana para"), what
-- AccountRightSpell's case SPELL_KICK_PLAYER did: logs the player named out.
function onSay(caster, command, words)
	Command.kickPlayer(caster, words[2])
end
