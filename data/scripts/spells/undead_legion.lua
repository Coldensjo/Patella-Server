-- spells/undead_legion.lua: Undead Legion ("exana mas mort"), what CastSpell's
-- case SPELL_UNDEAD_LEGION did: raises the corpses in radius 4 around the
-- caster.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.massRaiseDead(caster, caster, mana, soulPoints, 4)
end
