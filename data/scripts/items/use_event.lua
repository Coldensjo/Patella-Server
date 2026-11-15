-- items/use_event.lua: the use of a USEEVENT object, what Use's branches
-- USEEVENT did with UseObject (no target) and UseObjects (triggers.cc): runs
-- the trigger rules of use or multi_use.
function onUse(player, item, target, window)
	if target == nil then
		Game.useObject(player, item)
	else
		Game.useObjects(player, item, target)
	end
	return true
end
