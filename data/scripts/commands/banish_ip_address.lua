-- commands/banish_ip_address.lua: Banish IP Address ("omana vis para para"),
-- what IPBanishment (the original magic.cc) did: tells the caster the command
-- is gone and to use the dialog (Ctrl+Y). It checks no right and reads no
-- word.

function onSay(caster, command, words)
	caster:sendTextMessage(MESSAGE_STATUS_SMALL, "Spell is not available any more. Press Ctrl+Y for the dialog.")
end
