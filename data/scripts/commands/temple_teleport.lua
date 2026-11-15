-- commands/temple_teleport.lua: Temple Teleport ("omani para"), what
-- AccountRightSpell's case SPELL_TEMPLE_TELEPORT did: sends the player named to its temple.
function onSay(caster, command, words)
	Command.homeTeleport(caster, words[2])
end
