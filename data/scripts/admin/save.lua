-- admin/save.lua: save. Saves now what the game otherwise saves at logout,
-- every 15 minutes and at the shutdown: the data of every player online
-- (with the query manager), the data of those who left (the writer thread),
-- and the map with the houses' contents. The game stands still while the
-- map is written.

function onCommand(words)
	if #words ~= 0 then
		Admin.fail("Usage: save")
		return
	end

	local saved, failed = Admin.save()
	Admin.reply(string.format("Saved %d player%s online and the map; the others' data is being saved.", saved,
			saved == 1 and "" or "s"))
	if failed > 0 then
		Admin.fail(string.format("%d player%s could not be saved with the query manager (kept in files under save/).",
				failed, failed == 1 and "" or "s"))
	end
end
