-- commands/temple_teleport.lua: Temple Teleport ("omani para"), what
-- AccountRightSpell's case SPELL_TEMPLE_TELEPORT did: sends the player named to its temple.
return {
	on_say = function(caster, number, words)
		command.home_teleport(caster, words[2])
	end,
}
