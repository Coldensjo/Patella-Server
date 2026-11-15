-- runes/paralyze.lua: Paralyze ("adana ani"), what UseMagicItem's case
-- SPELL_PARALYZE did: slows the creature on the target field (druids only).
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	if targetCreature == nil then
		Game.throwResult(RETURNVALUE_CANONLYUSETHISRUNEONCREATURES)
	end
	if user:getVocation():getBaseId() ~= VOCATION_DRUID then
		Game.throwResult(RETURNVALUE_CANNOTUSETHISOBJECT)
	end
	Magic.magicGoStrength(user, targetCreature, mana, soulPoints, -101, 1)
end
