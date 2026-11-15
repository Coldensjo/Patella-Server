-- commands/invite_subowners.lua: Invite Subowners ("aleta som"), what
-- CharacterRightSpell's case SPELL_INVITE_SUBOWNERS did: opens the subowner list of the house the caster stands in.
function onSay(caster, command, words)
	Command.editSubowners(caster)
end
