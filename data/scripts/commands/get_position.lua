-- commands/get_position.lua: Get Position ("aliva tera"), what GetPosition
-- (the original magic.cc) did: with the command's right, tells the caster its
-- coordinate.

function onSay(caster, command, words)
	if caster:hasCommandRight(command) then
		local position = caster:getPosition()
		caster:sendTextMessage(MESSAGE_EVENT_ADVANCE,
				string.format("Your position is [%d,%d,%d].", position.x, position.y, position.z))
	end
end
