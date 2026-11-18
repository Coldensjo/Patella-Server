// spells_writer.cc: writes the game's spell table as spells.toml and, with the
// rights of the commands, commands.toml (spells_writer.hh).

#include "spells_writer.hh"

#include <cstring>
#include <vector>

#include "magicdata.hh"
#include "commands_reference.hh"
#include "spells_reference.hh"
#include "tomlwrite.hh"

// The flags as spells.toml names them, in the order it writes them.
static const struct {
	const char *Key;
	int Flag;
} FlagKeys[] = {
	{"aggressive", SPELLFLAG_AGGRESSIVE},
	{"premium", SPELLFLAG_PREMIUM},
	{"factor_max_100", SPELLFLAG_FACTOR_MAX_100},
	{"factor_min_100", SPELLFLAG_FACTOR_MIN_100},
};

// Words as comment lines of at most 80 characters, each line starting with
// Indent, the words separated by Separator.
static std::string CommentList(const std::vector<std::string> &Words, const std::string &Indent,
		const std::string &Separator) {
	std::string Text;
	std::string Line = Indent;
	for (size_t i = 0; i < Words.size(); i++) {
		std::string Word = Words[i] + (i + 1 < Words.size() ? Separator : "");
		std::string Trimmed = Word;
		while (!Trimmed.empty() && Trimmed.back() == ' ') {
			Trimmed.pop_back();
		}
		if (Line.size() > Indent.size() && Line.size() + Trimmed.size() > 80) {
			while (Line.back() == ' ') {
				Line.pop_back();
			}
			Text += Line + "\n";
			Line = Indent;
		}
		Line += Word;
	}
	while (!Line.empty() && Line.back() == ' ') {
		Line.pop_back();
	}
	return Text + Line + "\n";
}

static std::string Header() {
	std::string Text;
	Text += "# spells.toml: the spells of the game server, which players and gamemasters\n";
	Text += "# cast by saying their words, written by patella-convert from the spell table\n";
	Text += "# the game had in its code. The game reads spells.toml in its data folder\n";
	Text += "# ([paths] data of config.toml). This file says what a spell is called, how it\n";
	Text += "# is cast, what it needs and costs, and which script does what it does: a Lua\n";
	Text += "# file in the scripts folder (data/scripts/ in the repository). An unknown key\n";
	Text += "# or flag, or a value of the wrong type, is an error.\n";
	Text += "#\n";
	Text += "# Each [[spell]] is one spell:\n";
	Text += "#   number       The spell number, 1 to 255, each at most once and none of a\n";
	Text += "#                command in commands.toml. The player files keep the numbers\n";
	Text += "#                of the spells a player knows, so a spell keeps its number. It\n";
	Text += "#                does not say what the spell does; script does.\n";
	Text += "#   name         The name the spellbook shows, at most 30 characters.\n";
	Text += "#   words        What is said to cast it, as the spellbook shows it: the\n";
	Text += "#                class syllable and the next one as one word, every further\n";
	Text += "#                syllable a word of its own, separated by single spaces; at\n";
	Text += "#                most 10 syllables, and no two spells (or commands in\n";
	Text += "#                commands.toml) with the same. The class says how the spell\n";
	Text += "#                is cast: \"ex\" and \"ut\" at once, \"ad\" makes a rune of a\n";
	Text += "#                blank rune in a hand; the classes \"al\" and \"om\" are the\n";
	Text += "#                commands, in commands.toml. \"para\" stands for a word the\n";
	Text += "#                caster chooses (a name, a number; in quotes if it has\n";
	Text += "#                spaces).\n";
	Text += "#   level        The character level needed to cast it, or for a rune to\n";
	Text += "#                make it. Left out, 0.\n";
	Text += "#   mana         The mana it costs. Left out, 0.\n";
	Text += "#   soul_points  The soul points it costs. Left out, 0.\n";
	Text += "#   flags        By these names; left out, none:\n";
	Text += "#                aggressive      An attack: not in a protection zone (a rune\n";
	Text += "#                                neither at one) without the right to attack\n";
	Text += "#                                everywhere; it keeps the caster from\n";
	Text += "#                                logging out for 60 seconds; a rune aims at\n";
	Text += "#                                another creature on the field.\n";
	Text += "#                premium         For premium accounts only.\n";
	Text += "#                factor_max_100  The damage or healing factor (2 * level +\n";
	Text += "#                                3 * magic level, in percent) is at most 100.\n";
	Text += "#                factor_min_100  That factor is at least 100.\n";
	Text += "#   rune         The rune a rune spell (\"ad\") makes: group and number, the\n";
	Text += "#                object's old numbering (conversion.toml gives the type),\n";
	Text += "#                group 1 to 255 and number 0 to 255; magic_level, the magic\n";
	Text += "#                level needed to use the rune (its description says it); and\n";
	Text += "#                charges, the charges the rune is made with. magic_level and\n";
	Text += "#                charges left out are 0. Left out, the spell makes no rune.\n";
	Text += "#   script       The script that does what the spell does, its path in the\n";
	Text += "#                scripts folder: \"spells/<name>.lua\" for a spell cast at once\n";
	Text += "#                (\"ex\", \"ut\"), \"runes/<name>.lua\" for a rune spell (\"ad\"). Its\n";
	Text += "#                global function onCastSpell runs when the spell is cast or\n";
	Text += "#                its rune is used (making the rune is the game's). Left out,\n";
	Text += "#                casting the spell does nothing beyond its checks, and its\n";
	Text += "#                rune cannot be used.\n";
	Text += "#\n";
	Text += "# The syllables:\n";
	std::vector<std::string> Words;
	for (int i = 1; i < MAX_SYLLABLES; i++) {
		if (SpellSyllable[i][0] != 0) {
			Words.push_back(SpellSyllable[i]);
		}
	}
	Text += CommentList(Words, "#   ", ", ");
	return Text;
}

std::string SpellWords(const unsigned char *Syllable) {
	std::string Words;
	for (int i = 0; i < MAX_SPELL_SYLLABLES && Syllable[i] != 0; i++) {
		Words += std::string(i > 1 ? " " : "") + SpellSyllable[Syllable[i]];
	}
	return Words;
}

// Whether a spell table entry is all zero: no spell.
static bool IsEmpty(const TSpellList &Spell) {
	for (int i = 0; i < MAX_SPELL_SYLLABLES; i++) {
		if (Spell.Syllable[i] != 0) {
			return false;
		}
	}
	return Spell.RuneGr == 0 && Spell.RuneNr == 0 && Spell.Comment == nullptr && Spell.Level == 0
			&& Spell.RuneLevel == 0 && Spell.Flags == 0 && Spell.Mana == 0 && Spell.SoulPoints == 0
			&& Spell.Amount == 0;
}

// Whether the words of a spell table entry make it a command: class al or om.
static bool IsCommand(const TSpellList &Spell) {
	return Spell.Syllable[0] == SYLLABLE_AL || Spell.Syllable[0] == SYLLABLE_OM;
}

// Throws for what neither spells.toml nor commands.toml can hold of the spell
// table entry SpellNr, which is not empty. KnownFlags are the flags with a
// name.
static void CheckSpell(int SpellNr, int KnownFlags) {
	const TSpellList &Spell = SpellList[SpellNr];
	if (Spell.Comment == nullptr) {
		throw "a spell without a name, which spells.toml cannot hold";
	}
	if (strlen(Spell.Comment) > static_cast<size_t>(MAX_SPELL_NAME_LENGTH)) {
		throw "a spell name longer than 30 characters, which spells.toml refuses";
	}
	if (Spell.Syllable[0] < SYLLABLE_AL || Spell.Syllable[0] > SYLLABLE_OM) {
		throw "a spell whose words do not start with al, ad, ex, ut or om, which spells.toml cannot hold";
	}
	for (int i = 1; i < MAX_SPELL_SYLLABLES; i++) {
		if (Spell.Syllable[i] != 0 && (Spell.Syllable[i - 1] == 0 || Spell.Syllable[i] >= MAX_SYLLABLES
					|| SpellSyllable[Spell.Syllable[i]][0] == 0)) {
			throw "a spell with an unknown syllable or a gap in its words, which spells.toml cannot hold";
		}
	}
	for (int Other = 1; Other < SpellNr; Other++) {
		if (memcmp(SpellList[Other].Syllable, Spell.Syllable, sizeof(Spell.Syllable)) == 0) {
			throw "two spells with the same words, which spells.toml refuses";
		}
	}
	if (Spell.Mana < 0 || Spell.SoulPoints < 0 || Spell.Amount < 0) {
		throw "a negative mana, soul points or charges, which spells.toml refuses";
	}
	if ((Spell.Flags & ~KnownFlags) != 0) {
		throw "a spell flag without a name, which spells.toml cannot hold";
	}
	if (Spell.RuneGr == 0 && (Spell.RuneNr != 0 || Spell.RuneLevel != 0 || Spell.Amount != 0)) {
		throw "rune values without a rune group, which spells.toml cannot hold";
	}
	if (IsCommand(Spell) && (Spell.Level != 0 || Spell.Mana != 0 || Spell.SoulPoints != 0 || Spell.Flags != 0
				|| Spell.RuneGr != 0)) {
		throw "a command (al or om) with a level, mana, soul points, flags or a rune, which commands.toml cannot hold";
	}
	if (GetReferenceScriptName(SpellNr) == nullptr && GetOwnCommandScriptName(SpellNr) == nullptr) {
		throw "a spell number without a script name (a SPELL_ constant in spells_reference.hh), which spells.toml "
				"and commands.toml refuse";
	}
}

std::string SpellScript(int SpellNr) {
	const char *Name = GetReferenceScriptName(SpellNr);
	if (Name == nullptr) {
		Name = GetOwnCommandScriptName(SpellNr);
	}
	if (Name == nullptr) {
		throw "a spell number without a script name (a SPELL_ constant in spells_reference.hh), which spells.toml "
				"and commands.toml refuse";
	}
	const char *Folder = IsCommand(SpellList[SpellNr]) ? "commands/"
			: SpellList[SpellNr].Syllable[0] == SYLLABLE_AD ? "runes/" : "spells/";
	return std::string(Folder) + Name + ".lua";
}

// Checks the whole table (CheckSpell) and returns the flags with a name.
static int CheckSpells() {
	int KnownFlags = 0;
	for (const auto &Flag : FlagKeys) {
		KnownFlags |= Flag.Flag;
	}
	for (int SpellNr = 1; SpellNr < MAX_SPELLS; SpellNr++) {
		if (!IsEmpty(SpellList[SpellNr])) {
			CheckSpell(SpellNr, KnownFlags);
		}
	}
	return KnownFlags;
}

std::string SpellsToToml() {
	CheckSpells();
	std::string Text = Header();
	for (int SpellNr = 1; SpellNr < MAX_SPELLS; SpellNr++) {
		const TSpellList &Spell = SpellList[SpellNr];
		if (IsEmpty(Spell) || IsCommand(Spell)) {
			continue;
		}

		Text += "\n[[spell]]\n";
		Text += "number = " + std::to_string(SpellNr) + "\n";
		Text += "name = " + TomlQuote(Spell.Comment) + "\n";
		Text += "words = " + TomlQuote(SpellWords(Spell.Syllable)) + "\n";
		Text += "level = " + std::to_string(Spell.Level) + "\n";
		Text += "mana = " + std::to_string(Spell.Mana) + "\n";
		Text += "soul_points = " + std::to_string(Spell.SoulPoints) + "\n";
		std::string Flags;
		for (const auto &Flag : FlagKeys) {
			if ((Spell.Flags & Flag.Flag) != 0) {
				Flags += std::string(Flags.empty() ? "" : ", ") + "\"" + Flag.Key + "\"";
			}
		}
		if (!Flags.empty()) {
			Text += "flags = [" + Flags + "]\n";
		}
		if (Spell.RuneGr != 0) {
			Text += "rune = { group = " + std::to_string(Spell.RuneGr) + ", number = " + std::to_string(Spell.RuneNr)
					+ ", magic_level = " + std::to_string(Spell.RuneLevel) + ", charges = "
					+ std::to_string(Spell.Amount) + " }\n";
		}
		Text += "script = " + TomlQuote(SpellScript(SpellNr)) + "\n";
	}
	return Text;
}

static std::string CommandsHeader() {
	std::string Text;
	Text += "# commands.toml: the commands of the game server, the spells of the classes al\n";
	Text += "# and om, which gamemasters give by saying their words (and players, those of\n";
	Text += "# their houses), written by patella-convert from the spell table the game had\n";
	Text += "# in its code and the rights its code checked. The game reads commands.toml in\n";
	Text += "# its data folder ([paths] data of config.toml), after spells.toml. This file\n";
	Text += "# says what a command is called, what is said to give it, which rights it\n";
	Text += "# needs and which script does what it does: a Lua file in the scripts folder\n";
	Text += "# (data/scripts/ in the repository). An unknown key or right, or a value of\n";
	Text += "# the wrong type, is an error.\n";
	Text += "#\n";
	Text += "# Command 103 (Reload Scripts) reloads the scripts and the data while the game\n";
	Text += "# runs, command 104 (Online List) tells the caster the names of the players\n";
	Text += "# online; patella-convert adds them to the commands of the spell table.\n";
	Text += "#\n";
	Text += "# Each [[command]] is one command:\n";
	Text += "#   number  The command number, 1 to 255, each at most once and none of a\n";
	Text += "#           spell in spells.toml: the game keeps spells and commands in one\n";
	Text += "#           table by number. It does not say what the command does; script\n";
	Text += "#           does.\n";
	Text += "#   name    Its name, at most 30 characters.\n";
	Text += "#   words   What is said to give it, written as spells.toml writes words:\n";
	Text += "#           the class syllable and the next one as one word, every further\n";
	Text += "#           syllable a word of its own, separated by single spaces; at most\n";
	Text += "#           10 syllables, and no two commands or spells with the same. The\n";
	Text += "#           class is \"al\" for the commands of character rights, \"om\" for\n";
	Text += "#           those of account rights. \"para\" stands for a word the caster\n";
	Text += "#           chooses (a name, a number; in quotes if it has spaces).\n";
	Text += "#   right   The right a player needs for the command to do anything, by its\n";
	Text += "#           name below. Command 37 (Move) needs one of four rights, by the\n";
	Text += "#           word given, so its right is a table of them: vertical (\"up\" and\n";
	Text += "#           \"down\"), speed (\"fast\", \"fastest\", \"slow\" and \"normal\"),\n";
	Text += "#           coordinate (x,y,z or [x,y,z]) and mark (the name of a mark). The\n";
	Text += "#           code checks no right for the house commands and for those that\n";
	Text += "#           point to the rule violation dialog (Ctrl+Y): they have none.\n";
	Text += "#           Every other command needs it.\n";
	Text += "#   script  The script that does what the command does, \"commands/<name>.lua\"\n";
	Text += "#           in the scripts folder, whose global function onSay runs when it is\n";
	Text += "#           given. Left out, giving the command does nothing.\n";
	Text += "#\n";
	Text += "# The rights:\n";
	std::vector<std::string> Names(RightNames, RightNames + NUMBER_OF_RIGHTS);
	Text += CommentList(Names, "#   ", ", ");
	return Text;
}

std::string CommandsToToml() {
	CheckSpells();
	for (int SpellNr = 0; SpellNr < MAX_SPELLS; SpellNr++) {
		TCommandRightChecks Checks = GetCommandRightChecks(SpellNr);
		bool Command = !IsEmpty(SpellList[SpellNr]) && IsCommand(SpellList[SpellNr]);
		for (int i = 0; i < MAX_COMMAND_RIGHTS; i++) {
			int Right = CommandRight[SpellNr][i];
			if (Right != NO_RIGHT && (Right < 0 || Right >= NUMBER_OF_RIGHTS)) {
				throw "a right without a name, which commands.toml cannot hold";
			}
			if (Right != NO_RIGHT && (!Command || i >= Checks.Count)) {
				throw "a right for a check that no command has, which commands.toml cannot hold";
			}
			if (Right == NO_RIGHT && Command && i < Checks.Count) {
				throw "a command without a right for one of its checks, which commands.toml refuses";
			}
		}
	}

	std::string Text = CommandsHeader();
	for (int SpellNr = 1; SpellNr < MAX_SPELLS; SpellNr++) {
		const TSpellList &Spell = SpellList[SpellNr];
		if (IsEmpty(Spell) || !IsCommand(Spell)) {
			continue;
		}

		Text += "\n[[command]]\n";
		Text += "number = " + std::to_string(SpellNr) + "\n";
		Text += "name = " + TomlQuote(Spell.Comment) + "\n";
		Text += "words = " + TomlQuote(SpellWords(Spell.Syllable)) + "\n";
		TCommandRightChecks Checks = GetCommandRightChecks(SpellNr);
		bool OneRight = Checks.Count == 1 && Checks.Names[0][0] == 0;
		if (OneRight) {
			Text += "right = " + TomlQuote(RightNames[CommandRight[SpellNr][COMMAND_RIGHT]]) + "\n";
		}
		// The last key of the entry, before a right that is a table of its own
		// (which takes the keys after it).
		Text += "script = " + TomlQuote(SpellScript(SpellNr)) + "\n";
		if (!OneRight && Checks.Count > 0) {
			Text += "[command.right]\n";
			for (int i = 0; i < Checks.Count; i++) {
				Text += std::string(Checks.Names[i]) + " = " + TomlQuote(RightNames[CommandRight[SpellNr][i]]) + "\n";
			}
		}
	}
	return Text;
}
