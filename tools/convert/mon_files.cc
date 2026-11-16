// mon_files.cc: the reader of the original monster files (mon_files.hh). This
// is the game's reading of them before monsters.toml, raids.toml and
// monster_homes.toml: LoadRaces's and LoadMonsterRaids's loops over the
// folder, LoadRace (with TOutfit::load), LoadMonsterRaid (as its first pass,
// Start < 0, which checks all that the second checks) and LoadMonsterhomes,
// their logic reproduced on TReadScriptFile (script.cc, unchanged) with their
// error texts. Instead of RaceData, the attack waves and the homes it makes
// the data of monsterdata.hh. The file names are given instead of made from
// MONSTERPATH and DATAPATH, and the log lines ("Loading race data from file
// ...", "Raid ...: Date ...", "Initializing monster homes ...") are left out.
//
// Beside TReadScriptFile, a scanner of its own (TScanner) reads the same
// bytes in step with it, token for token, and each token is checked to be the
// same. It gives what the tokenizer does not tell: the line of each token,
// the comments it skips, the blank lines, the whole bytes of a string (the
// tokenizer's end at a zero byte) and the numbers of a byte sequence.
//
// What it refuses besides what the game refused, each in the form of
// TReadScriptFile's errors, with the file and line:
// - what the new form cannot hold: an include (@"<file>"); a string with a
//   zero byte; a comment of a raid file with a control character other than
//   a tab (raids.toml keeps the comments); a file whose name cannot be a key
//   (1 to MAX_MONSTER_KEY_LENGTH characters of a-z, 0-9 and '_' before
//   ".mon" or ".evt"); a race number that two .mon files give (the game
//   refused the second only when the first had a name); a race number of a
//   wave, a summon or a home that no .mon gives;
// - what the game handled only by accident: a race without a name (the game
//   took it as not defined); a name longer than 29 bytes or an article longer
//   than 2 (they overran Name[30] and Article[3]); an empty talk
//   (AddDynamicString("") gives 0) or one starting with '#' shorter than 3
//   bytes (the game said it from its fourth byte on); an outfit id outside 0
//   to 65535; an object type outside 0 to 65535 (the game cut it to 16
//   bits); colors that are not four numbers of 0 to 255 (TOutfit::load took
//   the first four bytes of the sequence, each cut to 8 bits); a negative
//   spell delay (one_in must be at least 1; the game's rand() % Delay was 0
//   for a 1 in -Delay).
// What the game read and ignored is noted (TMonsterFolder::Ignored): an
// unknown key of a race or of a wave (the game went on to read its value as
// the next key), an unknown flag, a raid type other than SmallRaid and
// BigRaid (the game took it as a small raid), a wave's Location (kept as
// location), a raid's description (kept), a key given twice (read as the
// game read it: the last value, or the lists of both; corpse and corpses
// count as one; raids.toml has the key where it was first given), the rest
// of monster.db after the race 0 that ends it.

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include <dirent.h>

#include "script.hh"
#include "mudinc.hh"
#include "utils.hh"
#include "crmain.hh"

#include "mon_files.hh"

namespace {

// The ranges of an outfit (monsterdata.hh), and the lengths of a race's Name
// and Article (TRaceData's Name[30] and Article[3]).
const int MAX_OUTFIT_ID = 65535;
const int MAX_OUTFIT_OBJECT_TYPE = 65535;
const int NUMBER_OF_COLORS = 4;
const int MAX_COLOR = 255;
const size_t MAX_RACE_NAME_LENGTH = 29;
const size_t MAX_ARTICLE_LENGTH = 2;

// A loud talk ('#', its kind, a blank) is said from its fourth byte on.
const size_t LOUD_TALK_PREFIX = 3;

// The texts of the errors this reader throws itself; they stay valid until
// the next one.
std::string RefusalText;
std::string ErrorText;

// The skill names GetSkillByName (crskill.cc) knows.
const char *const SKILL_NAMES[] = {
	"level", "magiclevel", "hitpoints", "mana", "gostrength", "carrystrength", "shielding",
	"distancefighting", "swordfighting", "clubfighting", "axefighting", "fistfighting", "magicdefense",
	"fishing", "eating", "shining", "illusion", "poison", "burning", "energy", "drunken", "manashield",
	"soulpoints",
};

// The flags LoadRace knows, in the order it tests them, with their names in
// monsters.toml.
const struct {
	const char *Identifier;
	const char *Name;
} FLAGS[] = {
	{"kickboxes", "kick_boxes"}, {"kickcreatures", "kick_creatures"}, {"seeinvisible", "see_invisible"},
	{"unpushable", "unpushable"}, {"distancefighting", "distance_fighting"}, {"nosummon", "no_summon"},
	{"noconvince", "no_convince"}, {"noillusion", "no_illusion"}, {"noburning", "no_burning"},
	{"nopoison", "no_poison"}, {"noenergy", "no_energy"}, {"nohit", "no_hit"},
	{"nolifedrain", "no_life_drain"}, {"noparalyze", "no_paralyze"},
};
const int NUMBER_OF_FLAGS = static_cast<int>(sizeof(FLAGS) / sizeof(FLAGS[0]));

// The blood types LoadRace knows.
const char *const BLOOD_TYPES[] = {"blood", "slime", "bones", "fire", "energy"};

// The spell shapes and impacts LoadRace knows, with the names of their
// parameters in monsters.toml (scripting.hh, "Monsters"), in the order the
// .mon gives them. The outfit impact's outfit is read with TOutfit::load.
struct TSpellPart {
	const char *Name;
	std::vector<const char *> Keys;
};

const TSpellPart SHAPES[] = {
	{"actor", {"effect"}},
	{"victim", {"range", "animation", "effect"}},
	{"origin", {"radius", "effect"}},
	{"destination", {"range", "animation", "radius", "effect"}},
	{"angle", {"angle", "range", "effect"}},
};

const TSpellPart IMPACTS[] = {
	{"damage", {"damage_type", "damage", "variation"}},
	{"field", {"field_type"}},
	{"healing", {"amount", "variation"}},
	{"speed", {"percent", "variation", "duration"}},
	{"drunken", {"strength", "variation", "duration"}},
	{"strength", {"skills", "percent", "variation", "duration"}},
	{"outfit", {"outfit", "duration"}},
	{"summon", {"race", "maximum"}},
};

TScriptValue Number(int Value) {
	TScriptValue Result;
	Result.Kind = TScriptValue::NUMBER;
	Result.Number = Value;
	return Result;
}

TScriptValue Text(const std::string &Value) {
	TScriptValue Result;
	Result.Kind = TScriptValue::TEXT;
	Result.Text = Value;
	return Result;
}

TScriptValue Table() {
	TScriptValue Result;
	Result.Kind = TScriptValue::TABLE;
	return Result;
}

TScriptValue Array() {
	TScriptValue Result;
	Result.Kind = TScriptValue::ARRAY;
	return Result;
}

void AddField(TScriptValue &Table, const char *Key, const TScriptValue &Value) {
	Table.Fields.emplace_back(Key, Value);
}

// An outfit as a value of monsters.toml: { id, colors } or { id = 0,
// object_type }.
TScriptValue OutfitValue(const TOutfit &Outfit) {
	TScriptValue Result = Table();
	AddField(Result, "id", Number(Outfit.OutfitID));
	if (Outfit.OutfitID == 0) {
		AddField(Result, "object_type", Number(Outfit.ObjectType));
	} else {
		TScriptValue Colors = Array();
		for (int i = 0; i < NUMBER_OF_COLORS; i++) {
			Colors.Elements.push_back(Number(Outfit.Colors[i]));
		}
		AddField(Result, "colors", Colors);
	}
	return Result;
}

std::string BaseName(const std::string &Path) {
	size_t Slash = Path.rfind('/');
	return Slash == std::string::npos ? Path : Path.substr(Slash + 1);
}

bool EndsWith(const std::string &Text, const char *Ending) {
	size_t Length = strlen(Ending);
	return Text.size() >= Length && Text.compare(Text.size() - Length, Length, Ending) == 0;
}

std::string Lower(const std::string &Text) {
	std::string Result = Text;
	for (char &c : Result) {
		c = static_cast<char>(toLower(c));
	}
	return Result;
}

// Whether Key can name a race or a raid: 1 to MAX_MONSTER_KEY_LENGTH
// characters of a-z, 0-9 and '_'.
bool ValidKey(const std::string &Key) {
	if (Key.empty() || Key.size() > static_cast<size_t>(MAX_MONSTER_KEY_LENGTH)) {
		return false;
	}
	for (char c : Key) {
		if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
			return false;
		}
	}
	return true;
}

[[noreturn]] void ThrowFor(const std::string &Path, const char *What) {
	ErrorText = Path + ": " + What;
	throw ErrorText.c_str();
}

// The text of a refusal at Line of the file FileName, in the form of
// TReadScriptFile's errors.
[[noreturn]] void Refuse(const std::string &FileName, int Line, const std::string &What) {
	RefusalText = "error in script-file \"" + FileName + "\", line " + std::to_string(Line) + ": " + What;
	throw RefusalText.c_str();
}

// "<path>:<line>", the Where of the data this reader makes.
std::string Where(const std::string &Path, int Line) {
	return Path + ":" + std::to_string(Line);
}

// The key of the race with the number Race, or nullptr.
const TMonsterData *RaceByNumber(const std::vector<TMonsterData> &Races, int Race) {
	for (const TMonsterData &Data : Races) {
		if (Data.Number == Race) {
			return &Data;
		}
	}
	return nullptr;
}

// One token as the scanner read it.
struct TScanToken {
	TOKEN Kind = ENDOFFILE;
	bool Include = false;    // an include (@"<file>") stood here instead
	std::string Text;        // IDENTIFIER: as written; STRING: all its bytes
	int Number = 0;          // NUMBER
	std::vector<int> Bytes;  // BYTES: each number as the tokenizer computed it
	int X = 0, Y = 0, Z = 0; // COORDINATE
	char Special = 0;        // SPECIAL
	int Line = 0;            // where it starts
	int EndLine = 0;         // where it ends
};

// A comment the scanner skipped: its text after the '#' (a CR at its end
// dropped), its line, and the number of tokens scanned before it.
struct TScanComment {
	std::string Text;
	int Line = 0;
	int Before = 0;
};

// The scanner: TReadScriptFile::nextToken's states on the file's bytes, with
// what it skips noted. It assumes the tokenizer accepted the token (the
// reader runs the tokenizer first); where it would have refused, the scanner
// throws. It does not follow includes: it gives a token with Include set.
class TScanner {
public:
	std::vector<TScanComment> Comments;
	std::vector<int> TouchedLines;   // the lines a token or a comment touches
	int TokenCount = 0;

	void open(const std::string &Path) {
		FILE *f = fopen(Path.c_str(), "rb");
		if (f == nullptr) {
			throw "patella-convert's scanner cannot open a file the tokenizer opened";
		}
		char Buffer[65536];
		size_t n;
		while ((n = fread(Buffer, 1, sizeof(Buffer), f)) > 0) {
			Data.append(Buffer, n);
		}
		fclose(f);
	}

	// The lines of the file that hold only blanks (a line feed ends a line;
	// a last line without one counts if it holds anything).
	std::vector<int> blankLines() const {
		int Lines = static_cast<int>(std::count(Data.begin(), Data.end(), '\n'));
		if (!Data.empty() && Data.back() != '\n') {
			Lines++;
		}
		std::vector<int> Touched = TouchedLines;
		std::sort(Touched.begin(), Touched.end());
		std::vector<int> Blank;
		for (int Line = 1; Line <= Lines; Line++) {
			if (!std::binary_search(Touched.begin(), Touched.end(), Line)) {
				Blank.push_back(Line);
			}
		}
		return Blank;
	}

	TScanToken next() {
		TScanToken Token;
		int State = 0;
		int c;
		unsigned int Number = 0;
		unsigned int Sign = 1;
		std::string Text;
		TScanComment Comment;
		while (true) {
			switch (State) {
			case 0:
				c = get();
				if (c == EOF) {
					return finish(Token, ENDOFFILE);
				}
				if (c == '\n') {
					Line++;
				} else if (!isSpace(c)) {
					Token.Line = Line;
					if (c == '#') {
						State = 1;
						Comment = TScanComment();
						Comment.Line = Line;
						Comment.Before = TokenCount;
					} else if (c == '@') {
						Token.Include = true;
						return finish(Token, SPECIAL);
					} else if (isAlpha(c)) {
						State = 2;
						Text += static_cast<char>(c);
					} else if (isDigit(c)) {
						Number = static_cast<unsigned int>(c - '0');
						State = 3;
					} else if (c == '"') {
						State = 6;
					} else if (c == '[') {
						State = 11;
					} else if (c == '<') {
						State = 22;
					} else if (c == '>') {
						State = 25;
					} else if (c == '-') {
						State = 27;
					} else {
						State = 10;
						Token.Special = static_cast<char>(c);
					}
				}
				break;

			case 1:
				c = get();
				if (c == EOF || c == '\n') {
					if (!Comment.Text.empty() && Comment.Text.back() == '\r') {
						Comment.Text.pop_back();
					}
					Comments.push_back(Comment);
					TouchedLines.push_back(Comment.Line);
				}
				if (c == EOF) {
					return finish(Token, ENDOFFILE);
				}
				if (c == '\n') {
					Line++;
					State = 0;
				} else {
					Comment.Text += static_cast<char>(c);
				}
				break;

			case 2:
				c = get();
				if (c != EOF && (isAlpha(c) || isDigit(c) || c == '_')) {
					Text += static_cast<char>(c);
				} else {
					if (c != EOF) {
						unget();
					}
					Token.Text = Text;
					return finish(Token, IDENTIFIER);
				}
				break;

			case 3:
				c = get();
				if (c != EOF && isDigit(c)) {
					Number = Number * 10 + static_cast<unsigned int>(c - '0');
				} else if (c == '-') {
					Token.Bytes.push_back(static_cast<int>(Number));
					State = 4;
				} else {
					if (c != EOF) {
						unget();
					}
					Token.Number = static_cast<int>(Number);
					return finish(Token, NUMBER);
				}
				break;

			case 4:
				c = get();
				if (c == EOF || !isDigit(c)) {
					throw "patella-convert's scanner met a byte sequence the tokenizer accepted";
				}
				Number = static_cast<unsigned int>(c - '0');
				State = 5;
				break;

			case 5:
				c = get();
				if (c != EOF && isDigit(c)) {
					Number = Number * 10 + static_cast<unsigned int>(c - '0');
				} else if (c == '-') {
					Token.Bytes.push_back(static_cast<int>(Number));
					State = 4;
				} else {
					if (c != EOF) {
						unget();
					}
					Token.Bytes.push_back(static_cast<int>(Number));
					return finish(Token, BYTES);
				}
				break;

			case 6:
				c = get();
				if (c == EOF) {
					throw "patella-convert's scanner met a string the tokenizer accepted";
				}
				if (c == '"') {
					Token.Text = Text;
					return finish(Token, STRING);
				} else if (c == '\\') {
					State = 7;
				} else {
					if (c == '\n') {
						Line++;
					}
					Text += static_cast<char>(c);
				}
				break;

			case 7:
				// Quirk of the tokenizer: a line break after a backslash is
				// kept in the string but not counted as a line.
				c = get();
				if (c == EOF) {
					throw "patella-convert's scanner met a string the tokenizer accepted";
				}
				Text += (c == 'n' ? '\n' : static_cast<char>(c));
				State = 6;
				break;

			case 10:
				return finish(Token, SPECIAL);

			case 11:
				c = get();
				Token.Special = '[';
				if (c != EOF && isDigit(c)) {
					Sign = 1;
					Number = static_cast<unsigned int>(c - '0');
					State = 12;
				} else if (c == '-') {
					Sign = static_cast<unsigned int>(-1);
					Number = 0;
					State = 12;
				} else {
					if (c != EOF) {
						unget();
					}
					return finish(Token, SPECIAL);
				}
				break;

			case 12:
			case 14:
			case 16:
				c = get();
				if (c != EOF && isDigit(c)) {
					Number = Number * 10 + static_cast<unsigned int>(c - '0');
				} else if (State == 12 && c == ',') {
					Token.X = static_cast<int>(Sign * Number);
					State = 13;
				} else if (State == 14 && c == ',') {
					Token.Y = static_cast<int>(Sign * Number);
					State = 15;
				} else if (State == 16 && c == ']') {
					Token.Z = static_cast<int>(Sign * Number);
					return finish(Token, COORDINATE);
				} else {
					throw "patella-convert's scanner met coordinates the tokenizer accepted";
				}
				break;

			case 13:
			case 15:
				c = get();
				if (c != EOF && isDigit(c)) {
					Sign = 1;
					Number = static_cast<unsigned int>(c - '0');
				} else if (c == '-') {
					Sign = static_cast<unsigned int>(-1);
					Number = 0;
				} else {
					throw "patella-convert's scanner met coordinates the tokenizer accepted";
				}
				State++;
				break;

			case 22:
				c = get();
				Token.Special = '<';
				if (c == '=') {
					Token.Special = 'L';
				} else if (c == '>') {
					Token.Special = 'N';
				} else if (c != EOF) {
					unget();
				}
				return finish(Token, SPECIAL);

			case 25:
				c = get();
				Token.Special = '>';
				if (c == '=') {
					Token.Special = 'G';
				} else if (c != EOF) {
					unget();
				}
				return finish(Token, SPECIAL);

			case 27:
				c = get();
				Token.Special = '-';
				if (c == '>') {
					Token.Special = 'I';
				} else if (c != EOF) {
					unget();
				}
				return finish(Token, SPECIAL);

			default:
				throw "patella-convert's scanner reached a state it does not have";
			}
		}
	}

private:
	std::string Data;
	size_t Position = 0;
	int Line = 1;

	int get() {
		if (Position < Data.size()) {
			return static_cast<unsigned char>(Data[Position++]);
		}
		return EOF;
	}

	void unget() {
		Position--;
	}

	TScanToken &finish(TScanToken &Token, TOKEN Kind) {
		Token.Kind = Kind;
		if (Kind == ENDOFFILE) {
			Token.Line = Line;
		}
		Token.EndLine = Line;
		if (Kind != ENDOFFILE) {
			for (int i = Token.Line; i <= Token.EndLine; i++) {
				TouchedLines.push_back(i);
			}
		}
		TokenCount++;
		return Token;
	}
};

// One file's reading: TReadScriptFile and the scanner in step, and
// TReadScriptFile's read functions on that step, with their texts.
class TFileReader {
public:
	explicit TFileReader(const std::string &Path) : Path(Path), FileName(BaseName(Path)) {}

	~TFileReader() {
		closeScript();
	}

	TFileReader(const TFileReader &) = delete;
	TFileReader &operator=(const TFileReader &) = delete;

protected:
	std::string Path;
	std::string FileName;
	TReadScriptFile Script;
	TScanner Scan;
	int Levels = 0;             // the files TReadScriptFile has open
	TScanToken Current;
	std::vector<TScanToken> Tokens;

	void open() {
		char Name[4096];
		snprintf(Name, sizeof(Name), "%s", Path.c_str());
		Script.open(Name);
		Levels = 1;
		Scan.open(Path);
	}

	void close() {
		Script.close();
		Levels = 0;
	}

	// Closes what is still open of TReadScriptFile (an error this reader
	// throws leaves it open; TReadScriptFile::error closes it).
	void closeScript() {
		while (Levels > 0) {
			Script.close();
			Levels--;
		}
	}

	[[noreturn]] void gameError(const char *Text) {
		Levels = 0;
		Script.error(const_cast<char *>(Text));
		throw "TReadScriptFile::error returned";
	}

	[[noreturn]] void refuse(int Line, const std::string &What) {
		closeScript();
		Refuse(FileName, Line, What);
	}

	// TReadScriptFile::nextToken, and the scanner's next token beside it.
	void nextToken() {
		try {
			Script.nextToken();
		} catch (const char *Text) {
			// TReadScriptFile::error closed every file; a failed open of an
			// include left the file before it open.
			Levels = strncmp(Text, "error in script-file", 20) == 0 ? 0 : 1;
			throw;
		}
		Current = Scan.next();
		Tokens.push_back(Current);
		if (Current.Include) {
			// TReadScriptFile opened the included file.
			Levels = 2;
			refuse(Current.Line, "an include, which the converted files cannot hold");
		}
		checkToken();
	}

	void checkToken() {
		bool Same = Script.Token == Current.Kind;
		if (Same) {
			switch (Current.Kind) {
				case IDENTIFIER:
					Same = Lower(Current.Text) == Script.getIdentifier();
					break;
				case NUMBER:
					Same = Script.getNumber() == Current.Number;
					break;
				case STRING:
					Same = Current.Text.substr(0, Current.Text.find('\0')) == Script.getString();
					break;
				case BYTES: {
					unsigned char *Bytes = Script.getBytesequence();
					for (size_t i = 0; i < Current.Bytes.size(); i++) {
						Same = Same && Bytes[i] == static_cast<unsigned char>(Current.Bytes[i]);
					}
					break;
				}
				case COORDINATE: {
					int x, y, z;
					Script.getCoordinate(x, y, z);
					Same = x == Current.X && y == Current.Y && z == Current.Z;
					break;
				}
				case SPECIAL:
					Same = Script.getSpecial() == Current.Special;
					break;
				case ENDOFFILE:
					break;
			}
		}
		if (!Same) {
			refuse(Current.Line,
					"patella-convert's scanner and the game's tokenizer disagree here (a fault of patella-convert)");
		}
		if (Current.Kind == STRING && Current.Text.find('\0') != std::string::npos) {
			refuse(Current.Line, "a string with a zero byte, where the game cut the string");
		}
	}

	bool isSpecial(char Special) {
		return Script.Token == SPECIAL && Script.getSpecial() == Special;
	}

	// TReadScriptFile::getIdentifier: the identifier in lower case.
	std::string identifier() {
		if (Script.Token != IDENTIFIER) {
			gameError("identifier expected");
		}
		return Script.getIdentifier();
	}

	std::string readIdentifier() {
		nextToken();
		if (Script.Token != IDENTIFIER) {
			gameError("identifier expected");
		}
		return Script.getIdentifier();
	}

	void readSymbol(char Symbol) {
		nextToken();
		if (Script.Token != SPECIAL) {
			gameError("special-char expected");
		}
		if (Script.getSpecial() != Symbol) {
			gameError("special-char expected");
		}
	}

	char readSpecial() {
		nextToken();
		if (Script.Token != SPECIAL) {
			gameError("special-char expected");
		}
		return Script.getSpecial();
	}

	int readNumber() {
		unsigned int Sign = 1;
		nextToken();
		if (isSpecial('-')) {
			Sign = static_cast<unsigned int>(-1);
			nextToken();
		}
		if (Script.Token != NUMBER) {
			gameError("number expected");
		}
		return static_cast<int>(Sign * static_cast<unsigned int>(Script.getNumber()));
	}

	// The string's bytes (the scanner's, which are the tokenizer's: a zero
	// byte is refused).
	std::string readString() {
		nextToken();
		if (Script.Token != STRING) {
			gameError("string expected");
		}
		return Current.Text;
	}

	void readCoordinate(int &x, int &y, int &z) {
		nextToken();
		if (Script.Token != COORDINATE) {
			gameError("coordinates expected");
		}
		Script.getCoordinate(x, y, z);
	}

	// TOutfit::load, with the refusals of what the game cut.
	void readOutfit(TOutfit &Outfit) {
		readSymbol('(');
		Outfit.OutfitID = readNumber();
		if (Outfit.OutfitID < 0 || Outfit.OutfitID > MAX_OUTFIT_ID) {
			refuse(Current.Line, "an outfit id outside 0 to 65535");
		}
		readSymbol(',');
		if (Outfit.OutfitID == 0) {
			int Type = readNumber();
			if (Type < 0 || Type > MAX_OUTFIT_OBJECT_TYPE) {
				refuse(Current.Line, "an object type outside 0 to 65535, which the game cut to 16 bits");
			}
			Outfit.ObjectType = static_cast<unsigned short>(Type);
		} else {
			nextToken();
			if (Script.Token != BYTES) {
				gameError("byte-sequence expected");
			}
			bool Valid = Current.Bytes.size() == static_cast<size_t>(NUMBER_OF_COLORS);
			for (int Byte : Current.Bytes) {
				Valid = Valid && Byte >= 0 && Byte <= MAX_COLOR;
			}
			if (!Valid) {
				refuse(Current.Line, "colors that are not four numbers of 0 to 255, which the game cut to four "
						"bytes");
			}
			for (int i = 0; i < NUMBER_OF_COLORS; i++) {
				Outfit.Colors[i] = static_cast<unsigned char>(Current.Bytes[static_cast<size_t>(i)]);
			}
		}
		readSymbol(')');
	}

	void note(std::vector<std::string> &Ignored, int Line, const std::string &What) {
		Ignored.push_back(FileName + ", line " + std::to_string(Line) + ": " + What);
	}
};

// LoadRace on one .mon file.
class TRaceReader : public TFileReader {
public:
	TRaceReader(const std::string &Path, const std::vector<TMonsterData> *Defined,
			std::vector<std::string> &Ignored) : TFileReader(Path), Defined(Defined), Ignored(Ignored) {}

	void read(TMonsterData &R);

private:
	const std::vector<TMonsterData> *Defined;
	std::vector<std::string> &Ignored;
	std::set<std::string> Given;

	// A key given twice is read as the game read it: a value replaces the
	// first, flags, skills, talk, inventory and spells are added to the
	// first's.
	void given(const std::string &Key, int Line) {
		if (!Given.insert(Key).second) {
			bool Added = Key == "flags" || Key == "skills" || Key == "talk" || Key == "inventory" || Key == "spells";
			note(Ignored, Line, Key + " given twice, of which the game took " + (Added ? "both" : "the last"));
		}
	}

	void readSpell(TMonsterData &R);
};

void TRaceReader::read(TMonsterData &R) {
	open();
	if (readIdentifier() != "racenumber") {
		gameError("race number expected");
	}
	readSymbol('=');
	int Race = readNumber();
	if (Race < 1 || Race >= MAX_RACES) {
		gameError("illegal race number");
	}
	if (Defined != nullptr) {
		for (const TMonsterData &Other : *Defined) {
			if (Other.Number == Race) {
				// Every race read before has a name (refused otherwise).
				gameError("race already defined");
			}
		}
	}

	// TRaceData's constructor, and LoadRace's outfit before the keys.
	R = TMonsterData();
	R.Number = Race;
	R.Name = "";
	R.Article = "";
	R.Outfit.OutfitID = Race;
	memset(R.Outfit.Colors, 0, sizeof(R.Outfit.Colors));
	R.Corpse = 0;
	R.FemaleCorpse = 0;
	R.Blood = "blood";
	R.Experience = R.SummonCost = R.FleeThreshold = R.Attack = R.Defend = R.Armor = R.Poison = R.LoseTarget = 0;
	R.Strategy[0] = 100;
	for (int i = 1; i < 4; i++) {
		R.Strategy[i] = 0;
	}
	R.Where = Where(Path, Current.Line);
	bool Flag[NUMBER_OF_FLAGS] = {};

	while (true) {
		nextToken();
		if (Script.Token == ENDOFFILE) {
			break;
		}
		if (Script.Token != IDENTIFIER) {
			gameError("identifier expected");
		}

		std::string Ident = identifier();
		int Line = Current.Line;
		readSymbol('=');
		const char *Known = nullptr;

		if (Ident == "name") {
			Known = "name";
			R.Name = readString();
			if (R.Name.size() > MAX_RACE_NAME_LENGTH) {
				refuse(Current.Line, "a name longer than 29 bytes, which overran the game's Name[30]");
			}
		}

		if (Ident == "article") {
			Known = "article";
			R.Article = readString();
			if (R.Article.size() > MAX_ARTICLE_LENGTH) {
				refuse(Current.Line, "an article longer than 2 bytes, which overran the game's Article[3]");
			}
		}

		if (Ident == "outfit") {
			Known = "outfit";
			readOutfit(R.Outfit);
		}

		if (Ident == "corpse") {
			Known = "corpse";
			R.Corpse = readNumber();
			R.FemaleCorpse = R.Corpse;
		}

		if (Ident == "corpses") {
			Known = "corpse";
			R.Corpse = readNumber();
			readSymbol(',');
			R.FemaleCorpse = readNumber();
		}

		if (Ident == "blood") {
			Known = "blood";
			std::string Value = readIdentifier();
			bool Found = false;
			for (const char *Blood : BLOOD_TYPES) {
				if (Value == Blood) {
					R.Blood = Blood;
					Found = true;
				}
			}
			if (!Found) {
				gameError("unknown blood type");
			}
		}

		const struct {
			const char *Identifier;
			int *Value;
		} NUMBERS[] = {
			{"experience", &R.Experience}, {"summoncost", &R.SummonCost}, {"fleethreshold", &R.FleeThreshold},
			{"attack", &R.Attack}, {"defend", &R.Defend}, {"armor", &R.Armor}, {"poison", &R.Poison},
			{"losetarget", &R.LoseTarget},
		};
		for (const auto &Entry : NUMBERS) {
			if (Ident == Entry.Identifier) {
				Known = Entry.Identifier;
				*Entry.Value = readNumber();
			}
		}

		if (Ident == "strategy") {
			Known = "strategy";
			readSymbol('(');
			for (int i = 0; i < 4; i++) {
				if (i != 0) {
					readSymbol(',');
				}
				R.Strategy[i] = readNumber();
			}
			readSymbol(')');
		}

		if (Ident == "flags") {
			Known = "flags";
			readSymbol('{');
			do {
				std::string Name = readIdentifier();
				bool Found = false;
				for (int i = 0; i < NUMBER_OF_FLAGS; i++) {
					if (Name == FLAGS[i].Identifier) {
						Flag[i] = true;
						Found = true;
					}
				}
				if (!Found) {
					note(Ignored, Current.Line, "unknown flag \"" + Name + "\", which the game ignored");
				}
			} while (readSpecial() != '}');
		}

		if (Ident == "skills") {
			Known = "skills";
			readSymbol('{');
			do {
				readSymbol('(');
				TMonsterSkillData S;
				S.Skill = readIdentifier();
				bool Found = false;
				for (const char *Name : SKILL_NAMES) {
					Found = Found || S.Skill == Name;
				}
				if (!Found) {
					gameError("unknown skill name");
				}
				S.Where = Where(Path, Current.Line);
				readSymbol(',');
				S.Actual = readNumber();
				readSymbol(',');
				S.Minimum = readNumber();
				readSymbol(',');
				S.Maximum = readNumber();
				readSymbol(',');
				S.NextLevel = readNumber();
				readSymbol(',');
				S.FactorPercent = readNumber();
				readSymbol(',');
				S.AddLevel = readNumber();
				readSymbol(')');
				R.Skills.push_back(S);
			} while (readSpecial() != '}');
		}

		if (Ident == "talk") {
			Known = "talk";
			readSymbol('{');
			do {
				std::string Talk = readString();
				if (Talk.empty()) {
					refuse(Current.Line, "an empty talk, which the game stored as no text (0)");
				}
				if (Talk[0] == '#' && Talk.size() < LOUD_TALK_PREFIX) {
					refuse(Current.Line, "a loud talk shorter than 3 bytes, which the game said from its fourth "
							"byte on");
				}
				R.Talk.push_back(Talk);
			} while (readSpecial() != '}');
		}

		if (Ident == "inventory") {
			Known = "inventory";
			readSymbol('{');
			do {
				readSymbol('(');
				TMonsterItemData I;
				I.Type = readNumber();
				readSymbol(',');
				I.Maximum = readNumber();
				readSymbol(',');
				I.Chance = readNumber();
				readSymbol(')');
				R.Inventory.push_back(I);
			} while (readSpecial() != '}');
		}

		if (Ident == "spells") {
			Known = "spells";
			readSymbol('{');
			do {
				readSpell(R);
			} while (readSpecial() != '}');
		}

		if (Known != nullptr) {
			given(Known, Line);
		} else {
			note(Ignored, Line, "unknown key \"" + Ident + "\", which the game ignored (it read its value as the "
					"next key)");
		}
	}

	close();

	for (int i = 0; i < NUMBER_OF_FLAGS; i++) {
		if (Flag[i]) {
			R.Flags.push_back(FLAGS[i].Name);
		}
	}
	if (R.Name.empty()) {
		Refuse(FileName, Current.Line, "a race without a name, which the game took as not defined");
	}
}

// One spell: Shape (p1, ...) -> Impact (q1, ...) : Delay, as the table of
// scripting.hh's "Monsters": shape and its keys, impact and its keys, one_in.
// A summon's race stays a number (ResolveRaceKeys).
void TRaceReader::readSpell(TMonsterData &R) {
	TScriptValue Spell = Table();
	std::string Shape = readIdentifier();
	int Line = Current.Line;
	const TSpellPart *ShapePart = nullptr;
	for (const TSpellPart &Part : SHAPES) {
		if (Shape == Part.Name) {
			ShapePart = &Part;
		}
	}
	if (ShapePart == nullptr) {
		gameError("unknown spell shape");
	}
	AddField(Spell, "shape", Text(Shape));
	readSymbol('(');
	for (size_t i = 0; i < ShapePart->Keys.size(); i++) {
		if (i != 0) {
			readSymbol(',');
		}
		AddField(Spell, ShapePart->Keys[i], Number(readNumber()));
	}
	readSymbol(')');

	readSymbol('I');

	std::string Impact = readIdentifier();
	const TSpellPart *ImpactPart = nullptr;
	for (const TSpellPart &Part : IMPACTS) {
		if (Impact == Part.Name) {
			ImpactPart = &Part;
		}
	}
	if (ImpactPart == nullptr) {
		gameError("unknown spell impact");
	}
	AddField(Spell, "impact", Text(Impact));
	readSymbol('(');
	for (size_t i = 0; i < ImpactPart->Keys.size(); i++) {
		if (i != 0) {
			readSymbol(',');
		}
		if (strcmp(ImpactPart->Keys[i], "outfit") == 0) {
			// For an outfit of an object type, LoadRace's HelpOutfit kept
			// two undefined bytes; the table holds only the type.
			TOutfit HelpOutfit;
			memset(&HelpOutfit, 0, sizeof(HelpOutfit));
			readOutfit(HelpOutfit);
			AddField(Spell, "outfit", OutfitValue(HelpOutfit));
		} else {
			AddField(Spell, ImpactPart->Keys[i], Number(readNumber()));
		}
	}
	readSymbol(')');

	readSymbol(':');
	int Delay = readNumber();
	if (Delay == 0) {
		gameError("zero spell delay");
	}
	if (Delay < 0) {
		refuse(Current.Line, "a negative spell delay (one_in must be at least 1)");
	}
	AddField(Spell, "one_in", Number(Delay));

	R.Spells.push_back(Spell);
	R.SpellWhere.push_back(Where(Path, Line));
}

// LoadMonsterRaid's first pass on one .evt file.
class TRaidReader : public TFileReader {
public:
	TRaidReader(const std::string &Path, const std::vector<TMonsterData> &Races, std::vector<std::string> &Ignored)
			: TFileReader(Path), Races(Races), Ignored(Ignored) {}

	void read(TRaidFile &File);

private:
	const std::vector<TMonsterData> &Races;
	std::vector<std::string> &Ignored;

	TRaidKey key(const char *Name, int Line) {
		TRaidKey Key;
		Key.Name = Name;
		Key.Line = Line;
		Key.EndLine = Current.EndLine;
		return Key;
	}
};

void TRaidReader::read(TRaidFile &File) {
	TRaidData &Raid = File.Raid;
	TRaidLayout &Layout = File.Layout;
	Raid = TRaidData();
	Raid.Big = false;
	Raid.Interval = 0;
	Raid.Date = 0;
	open();

	nextToken();
	Raid.Where = Where(Path, Current.Line);
	if (identifier() == "description") {
		int Line = Current.Line;
		readSymbol('=');
		Raid.Description = readString();
		Layout.RaidKeys.push_back(key("description", Line));
		note(Ignored, Line, "Description, which the game did not use (kept as description)");
		nextToken();
	}

	if (identifier() != "type") {
		gameError("type expected");
	}
	int Line = Current.Line;
	readSymbol('=');
	std::string Type = readIdentifier();
	Raid.Big = Type == "bigraid";
	if (Type != "bigraid" && Type != "smallraid") {
		note(Ignored, Current.Line, "raid type \"" + Type + "\", which the game took as a small raid");
	}
	Layout.RaidKeys.push_back(key("type", Line));

	std::string Ident = readIdentifier();
	Line = Current.Line;
	if (Ident == "date") {
		readSymbol('=');
		Raid.Date = readNumber();
		Layout.RaidKeys.push_back(key("date", Line));
	} else if (Ident == "interval") {
		readSymbol('=');
		Raid.Interval = readNumber();
		Layout.RaidKeys.push_back(key("interval", Line));
	} else {
		gameError("date or interval expected");
	}

	if (readIdentifier() != "delay") {
		gameError("delay expected");
	}

	while (Script.Token == IDENTIFIER) {
		std::vector<TRaidKey> Keys;
		std::set<std::string> Given;
		int DelayLine = Current.Line;
		readSymbol('=');
		TRaidWaveData Wave;
		Wave.Delay = readNumber();
		Keys.push_back(key("delay", DelayLine));
		Wave.x = Wave.y = Wave.z = 0;
		Wave.Spread = 0;
		int Race = -1;
		Wave.MinCount = 0;
		Wave.MaxCount = 0;
		Wave.Radius = INT_MAX;
		Wave.Lifetime = 0;
		Wave.Where = Where(Path, DelayLine);

		while (true) {
			nextToken();
			if (Script.Token == ENDOFFILE) {
				break;
			}
			if (Script.Token != IDENTIFIER) {
				gameError("identifier expected");
			}
			if (identifier() == "delay") {
				break;
			}

			std::string Name = identifier();
			int KeyLine = Current.Line;
			readSymbol('=');
			const char *Known = nullptr;

			if (Name == "location") {
				Known = "location";
				Wave.Location = readString();
				note(Ignored, KeyLine, "Location, which the game read and did not use (kept as location)");
			}

			if (Name == "position") {
				Known = "position";
				readCoordinate(Wave.x, Wave.y, Wave.z);
			}

			if (Name == "spread") {
				Known = "spread";
				Wave.Spread = readNumber();
			}

			if (Name == "race") {
				Known = "race";
				Race = readNumber();
				if (!(Race >= 1 && Race <= MAX_RACES - 1)) {
					gameError("illegal race number");
				}
			}

			if (Name == "count") {
				Known = "count";
				readSymbol('(');
				Wave.MinCount = readNumber();
				readSymbol(',');
				Wave.MaxCount = readNumber();
				readSymbol(')');
				if (Wave.MinCount > Wave.MaxCount) {
					gameError("mincount greater than maxcount");
				}
				if (Wave.MinCount < 0 || Wave.MaxCount <= 0) {
					gameError("illegal number of monsters");
				}
			}

			if (Name == "radius") {
				Known = "radius";
				Wave.Radius = readNumber();
			}

			if (Name == "lifetime") {
				Known = "lifetime";
				Wave.Lifetime = readNumber();
			}

			if (Name == "message") {
				Known = "message";
				Wave.Message = readString();
			}

			TRaidKey Key;
			if (Name == "inventory") {
				Known = "inventory";
				readSymbol('{');
				do {
					readSymbol('(');
					Key.ItemLines.push_back(Current.Line);
					TMonsterItemData I;
					I.Type = readNumber();
					readSymbol(',');
					I.Maximum = readNumber();
					readSymbol(',');
					I.Chance = readNumber();
					readSymbol(')');
					Wave.Inventory.push_back(I);
				} while (readSpecial() != '}');
			}

			if (Known == nullptr) {
				note(Ignored, KeyLine, "unknown key \"" + Name + "\", which the game ignored (it read its value as "
						"the next key)");
			} else if (!Given.insert(Known).second) {
				// The game took the last value (an inventory: added to the
				// first); the key stays where it was first given.
				note(Ignored, KeyLine, std::string(Known) + " given twice in a wave, of which the game took the "
						+ (strcmp(Known, "inventory") == 0 ? "objects of both" : "last"));
				for (TRaidKey &Earlier : Keys) {
					if (Earlier.Name == Known) {
						Earlier.EndLine = std::max(Earlier.EndLine, Current.EndLine);
						Earlier.ItemLines.insert(Earlier.ItemLines.end(), Key.ItemLines.begin(), Key.ItemLines.end());
					}
				}
			} else {
				std::vector<int> ItemLines = Key.ItemLines;
				Key = key(Known, KeyLine);
				Key.ItemLines = ItemLines;
				Keys.push_back(Key);
			}
		}

		if (Wave.x == 0) {
			gameError("position expected");
		}
		if (Race == -1) {
			gameError("race expected");
		}
		if (Wave.MaxCount == 0) {
			gameError("count expected");
		}

		const TMonsterData *Data = RaceByNumber(Races, Race);
		if (Data == nullptr) {
			refuse(DelayLine, "a wave of race " + std::to_string(Race) + ", of which there is no .mon");
		}
		Wave.Race = Data->Key;
		Raid.Waves.push_back(Wave);
		Layout.WaveKeys.push_back(Keys);
	}

	close();

	for (const TScanComment &Comment : Scan.Comments) {
		for (char c : Comment.Text) {
			unsigned char Byte = static_cast<unsigned char>(c);
			if ((Byte < 0x20 && Byte != '\t') || Byte == 0x7F) {
				Refuse(FileName, Comment.Line, "a comment with a control character, which a TOML comment cannot "
						"hold");
			}
		}
		TRaidComment Kept;
		Kept.Text = Comment.Text;
		Kept.Line = Comment.Line;
		Kept.Trailing = Comment.Before > 0
				&& Tokens[static_cast<size_t>(Comment.Before - 1)].EndLine == Comment.Line;
		Layout.Comments.push_back(Kept);
	}
	Layout.BlankLines = Scan.blankLines();
}

// LoadMonsterhomes's reading of monster.db.
class THomesReader : public TFileReader {
public:
	THomesReader(const std::string &Path, const std::vector<TMonsterData> &Races, std::vector<std::string> &Ignored)
			: TFileReader(Path), Races(Races), Ignored(Ignored) {}

	void read(std::vector<TMonsterHomeData> &Homes) {
		Homes.clear();
		open();
		while (true) {
			int Race = readNumber();
			int Line = Current.Line;
			if (Race == 0) {
				break;
			}
			TMonsterHomeData Home;
			Home.x = readNumber();
			Home.y = readNumber();
			Home.z = readNumber();
			Home.Radius = readNumber();
			Home.Amount = readNumber();
			Home.Regeneration = readNumber();
			Home.Where = Where(Path, Line);
			const TMonsterData *Data = RaceByNumber(Races, Race);
			if (Data == nullptr) {
				refuse(Line, "a home of race " + std::to_string(Race) + ", of which there is no .mon");
			}
			Home.Race = Data->Key;
			Homes.push_back(Home);
		}
		close();

		// The game read no further; the scanner looks whether anything follows.
		TScanToken Next;
		try {
			Next = Scan.next();
		} catch (const char *) {
			Next.Kind = SPECIAL;
			Next.Line = Current.EndLine;
		}
		if (Next.Kind != ENDOFFILE) {
			note(Ignored, Next.Line, "more after the race 0 that ends the homes, which the game did not read");
		}
	}

private:
	const std::vector<TMonsterData> &Races;
	std::vector<std::string> &Ignored;
};

void ReadRace(const std::string &Path, const std::vector<TMonsterData> *Defined, TMonsterData &Race,
		std::vector<std::string> &Ignored) {
	try {
		std::string Name = BaseName(Path);
		Race.Key = Name.size() > 4 ? Name.substr(0, Name.size() - 4) : Name;
		if (!EndsWith(Name, ".mon") || !ValidKey(Race.Key)) {
			Refuse(Name, 1, "a file name that cannot be a race's key (1 to 30 characters of a-z, 0-9 and '_' "
					"before \".mon\")");
		}
		std::string Key = Race.Key;
		TRaceReader Reader(Path, Defined, Ignored);
		Reader.read(Race);
		Race.Key = Key;
	} catch (const char *Text) {
		ThrowFor(Path, Text);
	}
}

} // namespace

void ReadRaceFile(const std::string &Path, TMonsterData &Race, std::vector<std::string> &Ignored) {
	ReadRace(Path, nullptr, Race, Ignored);
}

void ResolveRaceKeys(std::vector<TMonsterData> &Races) {
	for (TMonsterData &Race : Races) {
		for (size_t i = 0; i < Race.Spells.size(); i++) {
			TScriptValue &Spell = Race.Spells[i];
			bool Summon = false;
			for (const auto &Field : Spell.Fields) {
				Summon = Summon || (Field.first == "impact" && Field.second.Text == "summon");
			}
			for (auto &Field : Spell.Fields) {
				if (!Summon || Field.first != "race" || Field.second.Kind != TScriptValue::NUMBER) {
					continue;
				}
				const TMonsterData *Data = RaceByNumber(Races, Field.second.Number);
				if (Data == nullptr) {
					const std::string &At = Race.SpellWhere[i];
					size_t Colon = At.rfind(':');
					std::string Path = At.substr(0, Colon);
					try {
						Refuse(BaseName(Path), atoi(At.c_str() + Colon + 1), "a summon of race "
								+ std::to_string(Field.second.Number) + ", of which there is no .mon");
					} catch (const char *Text) {
						ThrowFor(Path, Text);
					}
				}
				Field.second = Text(Data->Key);
			}
		}
	}
}

TRaidFile ReadRaidFile(const std::string &Path, const std::vector<TMonsterData> &Races,
		std::vector<std::string> &Ignored) {
	TRaidFile File;
	try {
		std::string Name = BaseName(Path);
		std::string Key = Name.size() > 4 ? Name.substr(0, Name.size() - 4) : Name;
		if (!EndsWith(Name, ".evt") || !ValidKey(Key)) {
			Refuse(Name, 1, "a file name that cannot be a raid's key (1 to 30 characters of a-z, 0-9 and '_' "
					"before \".evt\")");
		}
		TRaidReader Reader(Path, Races, Ignored);
		Reader.read(File);
		File.Raid.Key = Key;
	} catch (const char *Text) {
		ThrowFor(Path, Text);
	}
	return File;
}

void ReadMonsterHomes(const char *FileName, const std::vector<TMonsterData> &Races,
		std::vector<TMonsterHomeData> &Homes, std::vector<std::string> &Ignored) {
	try {
		THomesReader Reader(FileName, Races, Ignored);
		Reader.read(Homes);
	} catch (const char *Text) {
		ThrowFor(FileName, Text);
	}
}

void ReadMonsterFolder(const char *Folder, TMonsterFolder &Result) {
	Result = TMonsterFolder();
	std::vector<std::string> RaceFiles, RaidFiles;
	DIR *WorkDir = opendir(Folder);
	if (WorkDir == nullptr) {
		ThrowFor(Folder, "the folder cannot be opened");
	}
	dirent *DirEntry;
	while ((DirEntry = readdir(WorkDir)) != nullptr) {
		std::string Name = DirEntry->d_name;
		if (Name.size() > 4 && EndsWith(Name, ".mon")) {
			RaceFiles.push_back(Name);
		}
		if (Name.size() > 4 && EndsWith(Name, ".evt")) {
			RaidFiles.push_back(Name);
		}
	}
	closedir(WorkDir);

	std::string Prefix = std::string(Folder) + "/";
	for (const std::string &Name : RaceFiles) {
		TMonsterData Race;
		ReadRace(Prefix + Name, &Result.Races, Race, Result.Ignored);
		Result.Races.push_back(Race);
	}
	std::stable_sort(Result.Races.begin(), Result.Races.end(),
			[](const TMonsterData &a, const TMonsterData &b) { return a.Number < b.Number; });
	ResolveRaceKeys(Result.Races);

	for (const std::string &Name : RaidFiles) {
		Result.Raids.push_back(ReadRaidFile(Prefix + Name, Result.Races, Result.Ignored));
	}
}
