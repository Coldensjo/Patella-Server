-- commands/reload_scripts.lua: Reload Scripts ("omevo gran cogni"), ours, not
-- the original's: asks for the reload of the scripts and the data, which the
-- main loop does after the command (reload.hh) and whose result it tells the
-- caster; tells the caster when a reload is asked already.
function onSay(caster, command, words)
	if not Command.reloadScripts(caster) then
		caster:sendTextMessage(MESSAGE_INFO_DESCR, "A reload is asked for already.")
	end
end
