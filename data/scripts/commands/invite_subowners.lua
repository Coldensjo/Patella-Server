-- commands/invite_subowners.lua: Invite Subowners ("aleta som"), what
-- CharacterRightSpell's case SPELL_INVITE_SUBOWNERS did: opens the subowner list of the house the caster stands in.
return {
	on_say = function(caster, number, words)
		command.edit_subowners(caster)
	end,
}
