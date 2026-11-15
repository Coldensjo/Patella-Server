-- spells/enchant_staff.lua: Enchant Staff ("exeta vis"), what CastSpell's case
-- SPELL_ENCHANT_STAFF did: turns a staff (90, 25) the caster holds into an
-- enchanted staff (90, 57).
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.enchantObject(caster, mana, soulPoints, ItemType.fromOldNumber(90, 25), ItemType.fromOldNumber(90, 57))
end
