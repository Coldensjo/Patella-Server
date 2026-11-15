-- items/container.lua: the use of a container, what Use's branch CONTAINER
-- did with UseContainer (triggers.cc): opens the container in the window
-- the client asked for, or closes it when it is open.
function onUse(player, item, target, window)
	player:useContainer(item, window)
	return true
end
