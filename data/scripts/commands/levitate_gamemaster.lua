-- commands/levitate_gamemaster.lua: Levitate Gamemaster ("alani hur para"),
-- what MagicClimbing(cr, Direction) (the original magic.cc) did: with the
-- command's right, moves the caster a floor up or down ("up", "down") onto the
-- field in front of it, as the spell Levitate does (Magic.magicClimbing), for
-- no mana and no soul points.

function onSay(caster, command, words)
	if caster:hasCommandRight(command) then
		Magic.magicClimbing(caster, 0, 0, words[3])
	end
end
