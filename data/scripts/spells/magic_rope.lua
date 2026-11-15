-- spells/magic_rope.lua: Magic Rope ("exani tera"), what CastSpell's case
-- SPELL_MAGIC_ROPE did: pulls the caster up through the rope spot below.
function onCastSpell(caster, spell, mana, soulPoints, words)
	Magic.magicRope(caster, mana, soulPoints)
end
