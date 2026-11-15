-- items/rune.lua: the use of a rune, what Use's branch RUNE did with
-- UseMagicItem (magic.cc): casts the rune's spell on the target.
function onUse(player, item, target, window)
	Magic.useMagicItem(player, item, target)
	return true
end
