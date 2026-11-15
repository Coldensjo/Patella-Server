-- commands/kick_guest.lua: Kick Guest ("alana sio para"), what
-- CharacterRightSpell's case SPELL_KICK_GUEST did: sends the player named out of the house it stands in.
return {
	on_say = function(caster, number, words)
		command.kick_guest(caster, words[3])
	end,
}
