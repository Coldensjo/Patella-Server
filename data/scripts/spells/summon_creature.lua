-- spells/summon_creature.lua: Summon Creature ("utevo res para"), what
-- CastSpell's case SPELL_SUMMON_CREATURE did: summons the creature the caster
-- names in words[3].
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.summonCreature(caster, mana, words[3], false)
end
