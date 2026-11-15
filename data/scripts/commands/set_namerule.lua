-- commands/set_namerule.lua: Set Namerule ("omamo para"), what
-- AccountRightSpell's case SPELL_SET_NAMERULE did: tells the caster the command is gone.
function onSay(caster, command, words)
	Command.setNameRule(caster, words[2])
end
