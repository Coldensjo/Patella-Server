-- commands/banish_account.lua: Banish Account ("omana liber para para para"), what
-- AccountRightSpell's case SPELL_BANISH_ACCOUNT did: tells the caster the command is gone and to use the dialog (Ctrl+Y).
function onSay(caster, command, words)
	Command.banishAccount(caster, words[3], Game.toInt(words[4]), words[5])
end
