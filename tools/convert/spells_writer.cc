// spells_writer.cc: writes the game's spell table as spells.toml
// (spells_writer.hh).

#include "spells_writer.hh"

#include <cstring>
#include <vector>

#include "magicdata.hh"
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
	Text += "# ([paths] data of config.toml). What a spell does stays in the code; this\n";
	Text += "# file says what it is called, how it is cast, what it needs and costs. An\n";
	Text += "# unknown key or flag, or a value of the wrong type, is an error.\n";
	Text += "#\n";
	Text += "# Each [[spell]] is one spell:\n";
	Text += "#   number       The spell number, 1 to 255, each at most once. It ties the\n";
	Text += "#                spell to what it does in the code, and the player files keep\n";
	Text += "#                the numbers of the spells a player knows, so a spell keeps\n";
	Text += "#                its number.\n";
	Text += "#   name         The name the spellbook shows, at most 30 characters.\n";
	Text += "#   words        What is said to cast it, as the spellbook shows it: the\n";
	Text += "#                class syllable and the next one as one word, every further\n";
	Text += "#                syllable a word of its own, separated by single spaces; at\n";
	Text += "#                most 10 syllables, and no two spells with the same. The class\n";
	Text += "#                says how the spell is cast: \"ex\" and \"ut\" at once, \"ad\"\n";
	Text += "#                makes a rune of a blank rune in a hand, \"al\" and \"om\" are\n";
	Text += "#                commands of the gamemasters' character and account rights.\n";
	Text += "#                \"para\" stands for a word the caster chooses (a name, a\n";
	Text += "#                number; in quotes if it has spaces).\n";
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

std::string SpellsToToml() {
	int KnownFlags = 0;
	for (const auto &Flag : FlagKeys) {
		KnownFlags |= Flag.Flag;
	}

	std::string Text = Header();
	for (int SpellNr = 1; SpellNr < MAX_SPELLS; SpellNr++) {
		const TSpellList &Spell = SpellList[SpellNr];
		if (IsEmpty(Spell)) {
			continue;
		}

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
	}
	return Text;
}
