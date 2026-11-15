-- commands/retrieve_friend.lua: Retrieve Friend ("alliber sio para"), what
-- CharacterRightSpell's case SPELL_RETRIEVE_FRIEND did: brings the player named to the caster.
function onSay(caster, command, words)
	Command.teleportPlayerToMe(caster, words[3])
end
