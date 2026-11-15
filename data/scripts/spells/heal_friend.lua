-- spells/heal_friend.lua: Heal Friend ("exura sio para"), what CastSpell's case
-- SPELL_HEAL_FRIEND did: heals the player named in words[3] by 120 plus or
-- minus 40.
return {
	on_cast = function(caster, spell, mana, soul_points, words)
		magic.heal_friend(caster, words[3], mana, soul_points, magic.compute_damage(caster, spell, 120, 40))
	end,
}
