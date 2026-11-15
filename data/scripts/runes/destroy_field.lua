-- runes/destroy_field.lua: Destroy Field ("adito grav"), what UseMagicItem's
-- case SPELL_DESTROY_FIELD did: removes a field from the target.
function onCastSpell(user, spell, mana, soulPoints, target, targetCreature)
	Magic.deleteField(user, target, 0, 0)
end
