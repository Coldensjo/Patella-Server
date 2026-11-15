-- spells/creature_illusion.lua: Creature Illusion ("utevo res ina para"), what
-- CastSpell's case SPELL_CREATURE_ILLUSION did: gives the caster the look of
-- the creature named in words[4] for 200 (CreatureIllusion).
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.creatureIllusion(caster, mana, soulPoints, words[4], 200)
end
