-- commands/invite_guests.lua: Invite Guests ("aleta sio"), what
-- CharacterRightSpell's case SPELL_INVITE_GUESTS did: opens the guest list of the house the caster stands in.
function onSay(caster, command, words)
	Command.editGuests(caster)
end
