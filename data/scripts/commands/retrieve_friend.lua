-- commands/retrieve_friend.lua: Retrieve Friend ("alliber sio para"), what
-- CharacterRightSpell's case SPELL_RETRIEVE_FRIEND did: brings the player named to the caster.
return {
	on_say = function(caster, number, words)
		command.teleport_player_to_me(caster, words[3])
	end,
}
