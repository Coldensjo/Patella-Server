-- commands/invite_guests.lua: Invite Guests ("aleta sio"), what
-- CharacterRightSpell's case SPELL_INVITE_GUESTS did: opens the guest list of the house the caster stands in.
return {
	on_say = function(caster, number, words)
		command.edit_guests(caster)
	end,
}
