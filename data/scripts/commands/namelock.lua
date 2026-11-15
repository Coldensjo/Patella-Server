-- commands/namelock.lua: Namelock ("omana hora para"), what
-- AccountRightSpell's case SPELL_NAMELOCK did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
function onSay(caster, command, words)
	Command.nameLock(caster, words[3])
end
