-- items/readable_text.lua: the use of a TEXT object, what Use's branch
-- TEXT && FONTSIZE == 1 did with UseTextObject (triggers.cc): opens the text
-- dialog for a document of font size 1; any other size is not handled here.
function onUse(player, item, target, window)
	if item:getTypeAttribute(ITEMTYPE_ATTRIBUTE_FONTSIZE) ~= 1 then
		return false
	end

	if not item:exists() then
		debugPrint("UseTextObject: Given object does not exist.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if not item:hasFlag(FLAG_TEXT) then
		debugPrint("UseTextObject: Given object is not a document.\n")
		Game.throwResult(RETURNVALUE_ERROR)
	end

	if player:isRemoved() then
		debugPrint(string.format("UseTextObject: Creature %d does not exist.\n", player:getId()))
		Game.throwResult(RETURNVALUE_ERROR)
	end

	player:sendEditText(item)
	return true
end
