-- commands/teleport_to_friend.lua: Teleport to Friend ("alani sio para"), what
-- CharacterRightSpell's case SPELL_TELEPORT_TO_FRIEND did: sends the caster to the creature named.
return {
	on_say = function(caster, number, words)
		command.teleport_to_creature(caster, words[3])
	end,
}
