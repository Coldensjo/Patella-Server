-- items/text_object.lua: the use of a WRITE or WRITEONCE object, what Use's
-- branch WRITE || WRITEONCE did with UseTextObject (triggers.cc): opens the
-- text dialog for the document.
function onUse(player, item, target, window)
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
