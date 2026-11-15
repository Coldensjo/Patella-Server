-- commands/kill_all_creatures.lua: Kill All Creatures ("alito mas res"), what
-- CharacterRightSpell's case SPELL_KILL_ALL_CREATURES did: kills the monsters within 2 fields of the caster, with the death effect.
function onSay(caster, command, words)
	Magic.killAllMonsters(caster, CONST_ME_MORTAREA, 2)
end
