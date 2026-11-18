-- commands/kill_all_creatures.lua: Kill All Creatures ("alito mas res"), what
-- KillAllMonsters(cr, EFFECT_DEATH, 2) (the original magic.cc) did: with the
-- command's right, kills every monster on the fields of the circles 0 to 2
-- around the caster (circles.toml) that a throw from the caster's field
-- reaches and that are no protection zone, each with a line at debug level 3,
-- and shows the death effect on each such field.

-- The largest circle.
local RADIUS = 2

function onSay(caster, command, words)
	if not caster:hasCommandRight(command) then
		return
	end

	for radius = 0, RADIUS do
		for _, offset in ipairs(Magic.getCircle(radius)) do
			-- The caster's field is read anew for each field (a monster's
			-- death runs its hooks, which may use the API).
			local position = caster:getPosition()
			local field = Position(position.x + offset.x, position.y + offset.y, position.z)
			if Game.isThrowPossible(position, field, 0) and not Game.isProtectionZone(field) then
				-- The next object is taken before the current one is handled
				-- (a death changes the field).
				local object = Game.getFirstObject(field)
				while object ~= nil do
					local current = object
					object = object:getNextObject()
					if current:isCreature() then
						local creature = current:getCreature()
						if creature == nil then
							debugPrint("KillAllMonsters: Invalid creature.\n")
						elseif creature:getId() ~= caster:getId() and creature:isMonster() then
							Game.print(3, "Killing " .. creature:getName() .. "...\n")
							creature:setHealth(0)
							creature:die()
						end
					end
				end

				Position(field.x, field.y, caster:getPosition().z):sendMagicEffect(CONST_ME_MORTAREA)
			end
		end
	end
end
