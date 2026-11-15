-- runes/intense_healing_rune.lua: Intense Healing Rune ("adura gran"), what
-- UseMagicItem's case SPELL_INTENSE_HEALING_RUNE did: heals the creature on
-- the target field.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	if targetCreature == nil then
		Game.throwResult(RETURNVALUE_CANONLYUSETHISRUNEONCREATURES)
	end
	local amount = Magic.computeDamage(user, spell, 70, 30)
	Magic.heal(targetCreature, -1, 0, amount)
end
