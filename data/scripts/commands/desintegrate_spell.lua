-- commands/desintegrate_spell.lua: Desintegrate Spell ("alito tera"), what
-- CharacterRightSpell's case SPELL_DESINTEGRATE_SPELL did: clears the field in front of the caster.
function onSay(caster, command, words)
	Magic.cleanupField(caster)
end
