-- commands/desintegrate_spell.lua: Desintegrate Spell ("alito tera"), what
-- CharacterRightSpell's case SPELL_DESINTEGRATE_SPELL did: clears the field in front of the caster.
return {
	on_say = function(caster, number, words)
		magic.cleanup_field(caster)
	end,
}
