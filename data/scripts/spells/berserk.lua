-- spells/berserk.lua: Berserk ("exori"), what CastSpell's case SPELL_BERSERK
-- did: physical damage of level * (80 plus or minus 20, times the caster's
-- damage factor of level and magic level) / 25 in radius 2 around the caster,
-- for 4 mana per level and no soul points.
function onCastSpell(caster, spell, mana, soulPoints, words)
	local amount = Magic.computeDamage(caster, spell, 80, 20)
	amount = Game.idiv(caster:getLevel() * amount, 25)
	Magic.massCombat(caster, caster, caster:getLevel() * 4, 0, amount, CONST_ME_HITAREA, 2, COMBAT_PHYSICALDAMAGE, CONST_ANI_NONE)
end
