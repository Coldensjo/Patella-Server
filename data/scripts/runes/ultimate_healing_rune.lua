-- runes/ultimate_healing_rune.lua: Ultimate Healing Rune ("adura vita"), what
-- UseMagicItem's case SPELL_ULTIMATE_HEALING_RUNE did: heals the creature on
-- the target field.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	if targetCreature == nil then
		Game.throwResult(RETURNVALUE_CANONLYUSETHISRUNEONCREATURES)
	end
	local amount = Magic.computeDamage(user, spell, 250, 0)
	Magic.heal(targetCreature, -1, 0, amount)
end
