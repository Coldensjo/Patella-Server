-- commands/kick_guest.lua: Kick Guest ("alana sio para"), what
-- CharacterRightSpell's case SPELL_KICK_GUEST did: sends the player named out of the house it stands in.
function onSay(caster, command, words)
	Command.kickGuest(caster, words[3])
end
