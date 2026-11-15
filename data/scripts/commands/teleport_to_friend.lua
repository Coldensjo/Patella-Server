-- commands/teleport_to_friend.lua: Teleport to Friend ("alani sio para"), what
-- CharacterRightSpell's case SPELL_TELEPORT_TO_FRIEND did: sends the caster to the creature named.
function onSay(caster, command, words)
	Command.teleportToCreature(caster, words[3])
end
