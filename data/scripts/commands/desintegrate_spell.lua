-- commands/desintegrate_spell.lua: Desintegrate Spell ("alito tera"), what
-- CleanupField(cr) (the original magic.cc) did: with the command's right,
-- clears the field the caster faces of its movable objects as the rune
-- Desintegrate does (Magic.cleanupFieldOn, with the puff), for no mana and no
-- soul points.

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	local direction = caster:getDirection()
	local position = caster:getPosition()
	local field
	if direction == DIRECTION_NORTH then
		field = Position(position.x, position.y - 1, position.z)
	elseif direction == DIRECTION_EAST then
		field = Position(position.x + 1, position.y, position.z)
	elseif direction == DIRECTION_SOUTH then
		field = Position(position.x, position.y + 1, position.z)
	elseif direction == DIRECTION_WEST then
		field = Position(position.x - 1, position.y, position.z)
	else
		return
	end

	Magic.cleanupFieldOn(caster, Game.getMapContainer(field), 0, 0)
end
