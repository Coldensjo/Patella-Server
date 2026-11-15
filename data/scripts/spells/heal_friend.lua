-- spells/heal_friend.lua: Heal Friend ("exura sio para"), what CastSpell's case
-- SPELL_HEAL_FRIEND did: heals the player named in words[3] by 120 plus or
-- minus 40.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.healFriend(caster, words[3], mana, soulPoints, Magic.computeDamage(caster, spell, 120, 40))
end
