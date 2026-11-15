-- runes/antidote_rune.lua: Antidote Rune ("adana pox"), what UseMagicItem's
-- case SPELL_ANTIDOTE_RUNE did: cures the poison of the creature on the target
-- field.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	if targetCreature == nil then
		Game.throwResult(RETURNVALUE_CANONLYUSETHISRUNEONCREATURES)
	end
	Magic.negatePoison(user, 0, 0, targetCreature)
end
