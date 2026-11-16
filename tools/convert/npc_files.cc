// npc_files.cc: the reader of the original NPC files (npc_files.hh). This is
// the game's reading of them before npcs.toml and the Lua scripts: InitNPCs's
// loop over the folder, TNPC::TNPC's header keys, TOutfit::load, and
// TBehaviourDatabase's parser (its constructor, readTerm, readFactor and
// readValue, with TCondition::set's properties and TAction's kinds), its logic
// reproduced on TReadScriptFile (script.cc, unchanged) with its error texts.
// Instead of the game's tables it makes the rules as elements of lib/npc.lua
// (npc_files.hh). The file name is given instead of made from NPCPATH, and
// the log line "Loading NPC data from file ..." is left out.
//
// Beside TReadScriptFile, a scanner of its own (TScanner) reads the same
// bytes in step with it, token for token, and each token is checked to be the
// same. It gives what the tokenizer does not tell: the file and line of each
// token, the comments the tokenizer skips, its includes (@"<file>"), the whole
// bytes of a string (the tokenizer's end at a zero byte) and the numbers of a
// byte sequence. TReadScriptFile's own read functions (readSymbol,
// readNumber, ...) are reproduced on the reader's step (they only call
// nextToken and check the token, with the same texts).
//
// What it refuses besides what the game refused, each in the form of
// TReadScriptFile's errors, with the file and line:
// - what the new form cannot hold: an include that is not at a rule boundary
//   (the tokens of the included file must be whole rules) or outside the
//   behaviour block; an include inside an .ndb; an include of a name that is
//   not a plain <name>.ndb of the same folder; a string with a zero byte; a
//   name longer than 30 bytes or with a line break; a comment with a CR
//   inside or that would start a Lua long comment ("#[[", "#[=["); a closing
//   brace where a rule of an .ndb begins; an .npc or .ndb file whose name
//   cannot name its script (a-z, 0-9, '_', '-' and '.' for an .npc; parts of
//   a-z and 0-9 joined by single '-', the first starting with a letter, for
//   an .ndb, whose name is a module's and a local's);
// - what the game handled only by accident: an empty text action
//   (AddDynamicString("") gives 0, on which react would crash) or an empty
//   word (the same); an .npc without one of the seven header keys (name,
//   sex, race, outfit, home, radius, gostrength); race after gostrength or
//   given twice; any other key given twice (the game took the last); a key
//   the game did not know (it went on to read the value as the next key); a
//   race outside 1 to 511; a radius or gostrength outside 0 to 65535; an
//   outfit id outside 0 to 65535; an object type outside 0 to 65535 (the
//   game cut it to 16 bits); colors that are not four numbers of 0 to 255
//   (TOutfit::load took the first four bytes of the sequence, each cut to 8
//   bits); a home outside x and y 0 to 65535 and z 0 to 15 (npcs.toml's
//   ranges; the game left such an NPC off the map).

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include <dirent.h>

#include "script.hh"
#include "mudinc.hh"
#include "utils.hh"
#include "cr.hh"
#include "crmain.hh"
#include "npcdata.hh"

#include "npc_files.hh"

namespace {

// The number of an outfit's colors, and the numbers a rule's "%N" condition
// may take (MAX_NUMBER_PARAMETERS of crnonpl.cc). The ranges of the header
// values are npcs.toml's (npcdata.hh; the race 1 to MAX_RACES - 1).
const int NUMBER_OF_COLORS = 4;
const int MAX_NUMBER_PARAMETERS = 3;

// The posz TNPC::TNPC started with: still this after the file, no home was
// given.
const int NO_HOME_Z = 255;

// The longest script name LoadScript takes.
const int MAX_SCRIPT_NAME_LENGTH = 256;

// The texts of the errors this reader throws itself; they stay valid until
// the next one.
std::string RefusalText;
std::string ErrorText;

// One token as the scanner read it.
struct TScanToken {
	TOKEN Kind = ENDOFFILE;
	std::string Text;        // IDENTIFIER: as written; STRING: all its bytes
	int Number = 0;          // NUMBER
	std::vector<int> Bytes;  // BYTES: each number as the tokenizer computed it
	int X = 0, Y = 0, Z = 0; // COORDINATE
	char Special = 0;        // SPECIAL
	int File = 0;            // the occurrence of the file it is in
	int Depth = 0;           // that file's depth (0: the file opened first)
	int Line = 0;            // where it starts
	int EndLine = 0;         // where it ends
};

// What the scanner met besides tokens, in the order it met them: a comment,
// an include and the end of an included file. Before is the index of the
// token being scanned then: the event lies between the tokens Before - 1 and
// Before.
struct TScanEvent {
	enum TKind {COMMENT, INCLUDE, INCLUDE_END};
	TKind Kind = COMMENT;
	int File = 0;       // COMMENT, INCLUDE: the file it is in; INCLUDE_END: the file that ended
	int Depth = 0;      // that file's depth
	int Line = 0;
	std::string Text;   // COMMENT: after the '#', a CR at the end dropped; INCLUDE: the name
	int Before = 0;
	int Included = -1;  // INCLUDE: the occurrence of the included file
};

// An open file of the scanner.
struct TScanFile {
	std::string Path;   // as TReadScriptFile's Filename has it
	std::string Data;
	size_t Position = 0;
	int Line = 1;
	int Id = 0;
};

// The scanner: TReadScriptFile::nextToken's states on the files' bytes, with
// what it skips noted. It assumes the tokenizer accepted the token (the
// reader runs the tokenizer first); where it would have refused, the scanner
// throws.
class TScanner {
public:
	std::vector<std::string> FileNames;   // by occurrence: the name the errors give
	std::vector<TScanEvent> Events;
	int TokenCount = 0;

	int depth() const {
		return static_cast<int>(Stack.size()) - 1;
	}

	void open(const std::string &Path) {
		TScanFile File;
		File.Path = Path;
		File.Id = static_cast<int>(FileNames.size());
		FILE *f = fopen(Path.c_str(), "rb");
		if (f == nullptr) {
			throw "patella-convert's scanner cannot open a file the tokenizer opened";
		}
		char Buffer[65536];
		size_t n;
		while ((n = fread(Buffer, 1, sizeof(Buffer), f)) > 0) {
			File.Data.append(Buffer, n);
		}
		fclose(f);
		size_t Slash = Path.rfind('/');
		FileNames.push_back(Slash == std::string::npos ? Path : Path.substr(Slash + 1));
		Stack.push_back(File);
	}

	TScanToken next() {
		TScanToken Token;
		int State;
		int c;
		unsigned int Number;
		unsigned int Sign;
		std::string Text;
		TScanEvent Comment;
	again:
		State = 0;
		Number = 0;
		Sign = 1;
		Text.clear();
		while (true) {
			TScanFile &f = Stack.back();
			switch (State) {
			case 0:
				c = get(f);
				if (c == EOF) {
					if (depth() <= 0) {
						return finish(Token, ENDOFFILE, f);
					}
					closeIncluded();
					goto again;
				}
				if (c == '\n') {
					f.Line++;
				} else if (!isSpace(c)) {
					Token.File = f.Id;
					Token.Depth = depth();
					Token.Line = f.Line;
					if (c == '#') {
						State = 1;
						Comment = TScanEvent();
						Comment.Kind = TScanEvent::COMMENT;
						Comment.File = f.Id;
						Comment.Depth = depth();
						Comment.Line = f.Line;
						Comment.Before = TokenCount;
					} else if (c == '@') {
						State = 30;
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
				c = get(f);
				if (c == EOF || c == '\n') {
					if (!Comment.Text.empty() && Comment.Text.back() == '\r') {
						Comment.Text.pop_back();
					}
					Events.push_back(Comment);
				}
				if (c == EOF) {
					if (depth() <= 0) {
						return finish(Token, ENDOFFILE, f);
					}
					closeIncluded();
					goto again;
				}
				if (c == '\n') {
					f.Line++;
					State = 0;
				} else {
					Comment.Text += static_cast<char>(c);
				}
				break;

			case 2:
				c = get(f);
				if (c != EOF && (isAlpha(c) || isDigit(c) || c == '_')) {
					Text += static_cast<char>(c);
				} else {
					if (c != EOF) {
						unget(f);
					}
					Token.Text = Text;
					return finish(Token, IDENTIFIER, f);
				}
				break;

			case 3:
				c = get(f);
				if (c != EOF && isDigit(c)) {
					Number = Number * 10 + static_cast<unsigned int>(c - '0');
				} else if (c == '-') {
					Token.Bytes.push_back(static_cast<int>(Number));
					State = 4;
				} else {
					if (c != EOF) {
						unget(f);
					}
					Token.Number = static_cast<int>(Number);
					return finish(Token, NUMBER, f);
				}
				break;

			case 4:
				c = get(f);
				if (c == EOF || !isDigit(c)) {
					throw "patella-convert's scanner met a byte sequence the tokenizer accepted";
				}
				Number = static_cast<unsigned int>(c - '0');
				State = 5;
				break;

			case 5:
				c = get(f);
				if (c != EOF && isDigit(c)) {
					Number = Number * 10 + static_cast<unsigned int>(c - '0');
				} else if (c == '-') {
					Token.Bytes.push_back(static_cast<int>(Number));
					State = 4;
				} else {
					if (c != EOF) {
						unget(f);
					}
					Token.Bytes.push_back(static_cast<int>(Number));
					return finish(Token, BYTES, f);
				}
				break;

			case 6:
				c = get(f);
				if (c == EOF) {
					throw "patella-convert's scanner met a string the tokenizer accepted";
				}
				if (c == '"') {
					Token.Text = Text;
					return finish(Token, STRING, f);
				} else if (c == '\\') {
					State = 7;
				} else {
					if (c == '\n') {
						f.Line++;
					}
					Text += static_cast<char>(c);
				}
				break;

			case 7:
				// Quirk of the tokenizer: a line break after a backslash is
				// kept in the string but not counted as a line.
				c = get(f);
				if (c == EOF) {
					throw "patella-convert's scanner met a string the tokenizer accepted";
				}
				Text += (c == 'n' ? '\n' : static_cast<char>(c));
				State = 6;
				break;

			case 10:
				return finish(Token, SPECIAL, f);

			case 11:
				c = get(f);
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
						unget(f);
					}
					return finish(Token, SPECIAL, f);
				}
				break;

			case 12:
			case 14:
			case 16:
				c = get(f);
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
					return finish(Token, COORDINATE, f);
				} else {
					throw "patella-convert's scanner met coordinates the tokenizer accepted";
				}
				break;

			case 13:
			case 15:
				c = get(f);
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
				c = get(f);
				Token.Special = '<';
				if (c == '=') {
					Token.Special = 'L';
				} else if (c == '>') {
					Token.Special = 'N';
				} else if (c != EOF) {
					unget(f);
				}
				return finish(Token, SPECIAL, f);

			case 25:
				c = get(f);
				Token.Special = '>';
				if (c == '=') {
					Token.Special = 'G';
				} else if (c != EOF) {
					unget(f);
				}
				return finish(Token, SPECIAL, f);

			case 27:
				c = get(f);
				Token.Special = '-';
				if (c == '>') {
					Token.Special = 'I';
				} else if (c != EOF) {
					unget(f);
				}
				return finish(Token, SPECIAL, f);

			case 30:
				c = get(f);
				if (c != '"') {
					throw "patella-convert's scanner met an include the tokenizer accepted";
				}
				State = 31;
				break;

			case 31:
				// The tokenizer neither counts a line break here nor
				// escapes anything.
				c = get(f);
				if (c == EOF) {
					throw "patella-convert's scanner met an include the tokenizer accepted";
				}
				if (c == '"') {
					include(Text, Token.Line);
					goto again;
				}
				Text += static_cast<char>(c);
				break;

			default:
				throw "patella-convert's scanner reached a state it does not have";
			}
		}
	}

private:
	std::vector<TScanFile> Stack;

	static int get(TScanFile &f) {
		if (f.Position < f.Data.size()) {
			return static_cast<unsigned char>(f.Data[f.Position++]);
		}
		return EOF;
	}

	static void unget(TScanFile &f) {
		f.Position--;
	}

	TScanToken &finish(TScanToken &Token, TOKEN Kind, const TScanFile &f) {
		Token.Kind = Kind;
		if (Kind == ENDOFFILE) {
			Token.File = f.Id;
			Token.Depth = depth();
			Token.Line = f.Line;
		}
		Token.EndLine = f.Line;
		TokenCount++;
		return Token;
	}

	// TReadScriptFile::open for an include: a name not starting with '/' is
	// taken relative to the including file's folder. The name ends at a zero
	// byte, as the tokenizer's C string does.
	void include(const std::string &Name, int Line) {
		TScanEvent Event;
		Event.Kind = TScanEvent::INCLUDE;
		Event.File = Stack.back().Id;
		Event.Depth = depth();
		Event.Line = Line;
		Event.Text = Name;
		Event.Before = TokenCount;
		std::string FileName = Name.substr(0, Name.find('\0'));
		std::string Path = FileName;
		if (FileName.empty() || FileName[0] != '/') {
			const std::string &Including = Stack.back().Path;
			size_t Slash = Including.rfind('/');
			if (Slash != std::string::npos) {
				Path = Including.substr(0, Slash + 1) + FileName;
			}
		}
		open(Path);
		Event.Included = Stack.back().Id;
		Events.push_back(Event);
	}

	void closeIncluded() {
		TScanEvent Event;
		Event.Kind = TScanEvent::INCLUDE_END;
		Event.File = Stack.back().Id;
		Event.Depth = depth();
		Event.Line = Stack.back().Line;
		Event.Before = TokenCount;
		Events.push_back(Event);
		Stack.pop_back();
	}
};

// The properties of a condition (TCondition::set's names) and the actions
// (TBehaviourDatabase's identifiers), with their names in lib/npc.lua.
struct TProperty {
	const char *Identifier;
	const char *Name;
};

const TProperty PROPERTIES[] = {
	{"address", "ADDRESS"}, {"busy", "BUSY"}, {"vanish", "VANISH"}, {"male", "MALE"}, {"female", "FEMALE"},
	{"knight", "KNIGHT"}, {"paladin", "PALADIN"}, {"sorcerer", "SORCERER"}, {"druid", "DRUID"},
	{"premium", "PREMIUM"}, {"promoted", "PROMOTED"}, {"pzblock", "PZBLOCK"}, {"nonpvp", "NONPVP"},
	{"pvpenforced", "PVPENFORCED"},
};

// The kinds of action by how the parser read them: "nop", "<name> = term",
// "<name>(term, term)", "<name>(term)", "<name>" and "<name>(term, term,
// term)".
enum TActionForm {NOP, SET, TWO, ONE, NONE, THREE};

struct TActionName {
	const char *Identifier;
	TActionForm Form;
	const char *Name;
};

const TActionName ACTIONS[] = {
	{"topic", SET, "setTopic"}, {"price", SET, "setPrice"}, {"amount", SET, "setAmount"},
	{"type", SET, "setType"}, {"data", SET, "setData"}, {"hp", SET, "setHp"},
	{"burning", TWO, "burning"}, {"poison", TWO, "poison"}, {"setquestvalue", TWO, "setQuestValue"},
	{"effectme", ONE, "effectMe"}, {"effectopp", ONE, "effectOpp"}, {"profession", ONE, "profession"},
	{"teachspell", ONE, "teachSpell"}, {"summon", ONE, "summon"}, {"create", ONE, "create"},
	{"delete", ONE, "delete"}, {"createmoney", NONE, "createMoney"}, {"deletemoney", NONE, "deleteMoney"},
	{"queue", NONE, "queue"}, {"teleport", THREE, "teleport"}, {"startposition", THREE, "startPosition"},
	{"idle", NONE, "idle"}, {"nop", NOP, "nop"},
};

// The header keys of an .npc, in the order of the "no ... specified" checks
// the reader adds after the game's.
const char *const KEYS[] = {"name", "sex", "race", "outfit", "home", "radius", "gostrength", "behaviour"};
const int NUMBER_OF_KEYS = 8;

const char *const MISSING_KEY[] = {
	"no name specified for NPC", "no sex specified for NPC", "no race specified for NPC",
	"no outfit specified for NPC", "no startpoint specified for NPC", "no radius specified for NPC",
	"no gostrength specified for NPC", "no behaviour database specified for NPC",
};

TNpcElement Word(const std::string &Text) {
	TNpcElement Element;
	Element.Kind = TNpcElement::STRING;
	Element.Text = Text;
	return Element;
}

TNpcElement Number(int Value) {
	TNpcElement Element;
	Element.Kind = TNpcElement::NUMBER;
	Element.Number = Value;
	return Element;
}

TNpcElement Name(const char *Text) {
	TNpcElement Element;
	Element.Kind = TNpcElement::NAME;
	Element.Text = Text;
	return Element;
}

TNpcElement Call(const char *Text, std::vector<TNpcElement> Arguments) {
	TNpcElement Element;
	Element.Kind = TNpcElement::CALL;
	Element.Text = Text;
	Element.Arguments = std::move(Arguments);
	return Element;
}

// A number parameter, %N: P1 to P3, param(n) for any other N.
TNpcElement Parameter(int N) {
	static const char *const NAMES[] = {"P1", "P2", "P3"};
	if (N >= 1 && N <= MAX_NUMBER_PARAMETERS) {
		return Name(NAMES[N - 1]);
	}
	return Call("param", {Number(N)});
}

std::string Lower(const std::string &Text) {
	std::string Result = Text;
	for (char &c : Result) {
		c = static_cast<char>(toLower(c));
	}
	return Result;
}

// Whether Name is a block's name the scripts can use: parts of a-z and 0-9
// joined by single '-', the first part starting with a letter.
bool ValidBlockName(const std::string &Name) {
	if (Name.empty() || !(Name[0] >= 'a' && Name[0] <= 'z')) {
		return false;
	}
	for (size_t i = 0; i < Name.size(); i++) {
		char c = Name[i];
		if (c == '-') {
			if (i + 1 == Name.size() || Name[i + 1] == '-') {
				return false;
			}
		} else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) {
			return false;
		}
	}
	return true;
}

// The names a block's local may not take: Lua's keywords and the names of a
// converted script (behaviour, rules, require, the handlers, the
// constructors).
bool ReservedName(const std::string &Name) {
	static const std::set<std::string> RESERVED = {
		"and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in", "local",
		"nil", "not", "or", "repeat", "return", "then", "true", "until", "while", "behaviour", "rules",
		"require", "onCreatureSay", "onCreatureDisappear", "eq", "ne", "lt", "gt", "le", "ge", "param", "count",
		"spellKnown", "spellLevel", "questValue", "random", "add", "sub", "mul", "setTopic", "setPrice",
		"setAmount", "setType", "setData", "setHp", "burning", "poison", "setQuestValue", "effectMe",
		"effectOpp", "profession", "teachSpell", "summon", "create", "delete", "createMoney", "deleteMoney",
		"queue", "idle", "nop", "teleport", "startPosition", "previous",
	};
	return RESERVED.count(Name) != 0;
}

// Whether FileName (without ".npc") can name the script npcs/<FileName>.lua.
bool ValidNpcFileName(const std::string &FileName) {
	if (FileName.empty() || FileName == "." || FileName == ".."
			|| FileName.size() + strlen("npcs/.lua") > static_cast<size_t>(MAX_SCRIPT_NAME_LENGTH)) {
		return false;
	}
	for (char c : FileName) {
		if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
			return false;
		}
	}
	return true;
}

std::string BaseName(const std::string &Path) {
	size_t Slash = Path.rfind('/');
	return Slash == std::string::npos ? Path : Path.substr(Slash + 1);
}

bool EndsWith(const std::string &Text, const char *Ending) {
	size_t Length = strlen(Ending);
	return Text.size() >= Length && Text.compare(Text.size() - Length, Length, Ending) == 0;
}

[[noreturn]] void ThrowFor(const std::string &Path, const char *Text) {
	ErrorText = Path + ": " + Text;
	throw ErrorText.c_str();
}

// One file's reading: TReadScriptFile and the scanner in step, and the
// parser on them.
class TNpcReader {
public:
	explicit TNpcReader(const std::string &Path) : Path(Path) {}

	~TNpcReader() {
		closeScript();
	}

	void readNpc(TNpcFile &Npc);
	void readBlock(TNpcBlock &Block);

	// Closes what is still open of TReadScriptFile (an error the reader
	// throws leaves it open; TReadScriptFile::error closes it).
	void closeScript() {
		while (Levels > 0) {
			Script.close();
			Levels--;
		}
	}

private:
	std::string Path;
	TReadScriptFile Script;
	TScanner Scan;
	int Levels = 0;             // the files TReadScriptFile has open
	size_t CheckedEvents = 0;
	std::vector<TScanToken> Tokens;
	TScanToken Current;
	bool InBlock = false;       // reading an .ndb on its own
	std::vector<TNpcRule> Rules;
	std::vector<int> RuleFirst, RuleLast;   // each rule's first and last token
	int OpenIndex = -1;         // an .npc's '{' and '}'
	int CloseIndex = -1;

	int currentIndex() const {
		return static_cast<int>(Tokens.size()) - 1;
	}

	void open() {
		char FileName[4096];
		snprintf(FileName, sizeof(FileName), "%s", Path.c_str());
		Script.open(FileName);
		Levels = 1;
		Scan.open(Path);
	}

	void closeAfterEnd() {
		Script.close();
		Levels = 0;
	}

	[[noreturn]] void gameError(const char *Text) {
		Levels = 0;
		Script.error(const_cast<char *>(Text));
		throw "TReadScriptFile::error returned";
	}

	// A thrown text of the game's (with the files still open, which the game
	// left so).
	[[noreturn]] void gameThrow(const char *Text) {
		closeScript();
		throw Text;
	}

	[[noreturn]] void refuse(int File, int Line, const std::string &What) {
		closeScript();
		RefusalText = "error in script-file \"" + Scan.FileNames[static_cast<size_t>(File)] + "\", line "
				+ std::to_string(Line) + ": " + What;
		throw RefusalText.c_str();
	}

	[[noreturn]] void refuse(const TScanToken &At, const std::string &What) {
		refuse(At.File, At.Line, What);
	}

	void nextToken();
	void checkToken();
	void checkEvents();
	void readSymbol(char Symbol);
	int readNumber();
	void readString();
	void readIdentifier();

	bool isSpecial(char Special) {
		return Script.Token == SPECIAL && Script.getSpecial() == Special;
	}

	void readOutfit(TOutfit &Outfit);
	void readBehaviour();
	void readRule();
	TNpcElement readTerm();
	TNpcElement readFactor();
	TNpcElement readValue();
	void checkIncludes(TNpcFile &Npc);
	void buildList(TNpcList &List, int HeaderBefore, int FirstToken, int EndToken);
};

// TReadScriptFile::nextToken, and the scanner's next token beside it.
void TNpcReader::nextToken() {
	try {
		Script.nextToken();
	} catch (const char *Text) {
		// TReadScriptFile::error closed every file; a failed open of an
		// include left the files before it open.
		Levels = strncmp(Text, "error in script-file", 20) == 0 ? 0 : Scan.depth() + 1;
		throw;
	}
	Current = Scan.next();
	Tokens.push_back(Current);
	Levels = Scan.depth() + 1;
	checkToken();
	checkEvents();
}

void TNpcReader::checkToken() {
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
		refuse(Current,
				"patella-convert's scanner and the game's tokenizer disagree here (a fault of patella-convert)");
	}
	if (Current.Kind == STRING && Current.Text.find('\0') != std::string::npos) {
		refuse(Current, "a string with a zero byte, where the game cut the string");
	}
}

// The includes and comments the scanner met with this token.
void TNpcReader::checkEvents() {
	for (; CheckedEvents < Scan.Events.size(); CheckedEvents++) {
		const TScanEvent &Event = Scan.Events[CheckedEvents];
		if (Event.Kind == TScanEvent::INCLUDE) {
			if (InBlock || Event.Depth > 0) {
				refuse(Event.File, Event.Line, "an include inside an .ndb, which a rule block's module cannot hold");
			}
			std::string Name = Event.Text;
			if (!EndsWith(Name, ".ndb") || Name.find('\0') != std::string::npos
					|| !ValidBlockName(Name.substr(0, Name.size() - 4))) {
				refuse(Event.File, Event.Line, "an include of \"" + Name + "\", not a plain <name>.ndb of the same "
						"folder whose name is parts of a-z and 0-9 joined by single '-', starting with a letter");
			}
		} else if (Event.Kind == TScanEvent::COMMENT) {
			const std::string &Text = Event.Text;
			if (Text.find('\r') != std::string::npos) {
				refuse(Event.File, Event.Line, "a comment with a CR inside, which ends a Lua comment");
			}
			if (!Text.empty() && Text[0] == '[') {
				size_t i = 1;
				while (i < Text.size() && Text[i] == '=') {
					i++;
				}
				if (i < Text.size() && Text[i] == '[') {
					refuse(Event.File, Event.Line, "a comment that would start a Lua long comment");
				}
			}
		}
	}
}

// TReadScriptFile's read functions on the reader's step, with their texts.
void TNpcReader::readSymbol(char Symbol) {
	nextToken();
	if (Script.Token != SPECIAL) {
		gameError("special-char expected");
	}
	if (Script.getSpecial() != Symbol) {
		gameError("special-char expected");
	}
}

int TNpcReader::readNumber() {
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

void TNpcReader::readString() {
	nextToken();
	if (Script.Token != STRING) {
		gameError("string expected");
	}
}

void TNpcReader::readIdentifier() {
	nextToken();
	if (Script.Token != IDENTIFIER) {
		gameError("identifier expected");
	}
}

// TOutfit::load.
void TNpcReader::readOutfit(TOutfit &Outfit) {
	readSymbol('(');
	Outfit.OutfitID = readNumber();
	if (Outfit.OutfitID < 0 || Outfit.OutfitID > MAX_NPC_OUTFIT) {
		refuse(Current, "an outfit id outside 0 to 65535");
	}
	readSymbol(',');
	if (Outfit.OutfitID == 0) {
		int Type = readNumber();
		if (Type < 0 || Type > MAX_NPC_OUTFIT_OBJECT_TYPE) {
			refuse(Current, "an object type outside 0 to 65535, which the game cut to 16 bits");
		}
		Outfit.ObjectType = static_cast<unsigned short>(Type);
	} else {
		nextToken();
		if (Script.Token != BYTES) {
			gameError("byte-sequence expected");
		}
		memcpy(Outfit.Colors, Script.getBytesequence(), sizeof(Outfit.Colors));
		bool Valid = Current.Bytes.size() == static_cast<size_t>(NUMBER_OF_COLORS);
		for (int Color : Current.Bytes) {
			Valid = Valid && Color >= 0 && Color <= MAX_NPC_OUTFIT_COLOR;
		}
		if (!Valid) {
			refuse(Current, "colors that are not four numbers of 0 to 255, of which the game took the first four "
					"bytes");
		}
	}
	readSymbol(')');
}

// TNPC::TNPC's reading of the file.
void TNpcReader::readNpc(TNpcFile &Npc) {
	Npc.Sex = SEX_MALE;
	Npc.Race = 0;
	memset(&Npc.Outfit, 0, sizeof(Npc.Outfit));
	Npc.HomeZ = NO_HOME_Z;
	Npc.Name.clear();
	bool Seen[NUMBER_OF_KEYS] = {};
	TScanToken Home;

	open();
	while (true) {
		nextToken();
		if (Script.Token == ENDOFFILE) {
			break;
		}
		if (Script.Token != IDENTIFIER) {
			gameError("identifier expected");
		}
		std::string Ident = Script.getIdentifier();
		TScanToken Key = Current;
		readSymbol('=');

		int Index = 0;
		while (Index < NUMBER_OF_KEYS && Ident != KEYS[Index]) {
			Index++;
		}
		if (Index == NUMBER_OF_KEYS) {
			refuse(Key, "a key \"" + Ident + "\" the game did not know (it read its value as the next key)");
		}
		if (Ident == "race") {
			if (Seen[Index] || Seen[6]) {
				refuse(Key, "race after gostrength or given twice (it set the skills again)");
			}
		} else if (Ident != "behaviour" && Seen[Index]) {
			refuse(Key, "\"" + Ident + "\" given twice (the game took the last)");
		}

		if (Ident == "name") {
			readString();
			if (Current.Text.size() > static_cast<size_t>(MAX_NPC_NAME_LENGTH)) {
				refuse(Current, "a name longer than 30 bytes, which TCreature's Name cannot hold");
			}
			if (Current.Text.find_first_of("\r\n") != std::string::npos) {
				refuse(Current, "a name with a line break, which the script's header comment cannot hold");
			}
			Npc.Name = Script.getString();
		}

		if (Ident == "sex") {
			readIdentifier();
			std::string Value = Script.getIdentifier();
			int Sex = 255;
			if (Value == "male") {
				Sex = SEX_MALE;
			}
			if (Value == "female") {
				Sex = SEX_FEMALE;
			}
			if (Sex == 255) {
				gameError("unknown constant");
			}
			Npc.Sex = Sex;
		}

		if (Ident == "race") {
			Npc.Race = readNumber();
			if (Npc.Race < 1 || Npc.Race >= MAX_RACES) {
				refuse(Current, "a race outside 1 to 511");
			}
		}

		if (Ident == "outfit") {
			readOutfit(Npc.Outfit);
		}

		if (Ident == "home") {
			nextToken();
			if (Script.Token != COORDINATE) {
				gameError("coordinates expected");
			}
			Script.getCoordinate(Npc.HomeX, Npc.HomeY, Npc.HomeZ);
			Home = Current;
		}

		if (Ident == "radius") {
			Npc.Radius = readNumber();
			if (Npc.Radius < 0 || Npc.Radius > MAX_NPC_RADIUS) {
				refuse(Current, "a radius outside 0 to 65535");
			}
		}

		if (Ident == "gostrength") {
			if (Npc.Race == 0) {
				gameThrow("gostrength before race in npc-script-file");
			}
			Npc.GoStrength = readNumber();
			if (Npc.GoStrength < 0 || Npc.GoStrength > MAX_NPC_GOSTRENGTH) {
				refuse(Current, "a gostrength outside 0 to 65535");
			}
		}

		if (Ident == "behaviour") {
			if (Seen[Index]) {
				gameThrow("behaviour database specified twice for NPC");
			}
			readBehaviour();
		}
		Seen[Index] = true;
	}
	closeAfterEnd();

	if (Npc.Name.empty()) {
		throw MISSING_KEY[0];
	}
	if (Npc.HomeZ == NO_HOME_Z) {
		throw MISSING_KEY[4];
	}
	if (!Seen[7]) {
		throw MISSING_KEY[7];
	}
	for (int Index = 0; Index < NUMBER_OF_KEYS; Index++) {
		if (!Seen[Index]) {
			throw MISSING_KEY[Index];
		}
	}
	if (Npc.HomeX < 0 || Npc.HomeX > MAX_NPC_HOME_XY || Npc.HomeY < 0 || Npc.HomeY > MAX_NPC_HOME_XY || Npc.HomeZ < 0
			|| Npc.HomeZ > MAX_NPC_HOME_Z) {
		refuse(Home, "a home outside x and y 0 to 65535 and z 0 to 15");
	}

	checkIncludes(Npc);
	Npc.Rules = Rules;
	buildList(Npc.List, OpenIndex, OpenIndex + 1, CloseIndex);
	for (const TNpcListItem &Item : Npc.List.Items) {
		if (Item.Kind == TNpcListItem::RULE) {
			Npc.OwnRules++;
		}
	}
}

// An .ndb on its own: its rules up to the end of the file.
void TNpcReader::readBlock(TNpcBlock &Block) {
	InBlock = true;
	open();
	nextToken();
	while (Script.Token != ENDOFFILE) {
		if (isSpecial('}')) {
			refuse(Current, "a closing brace where a rule begins, which would end the behaviour block of an NPC "
					"that includes the block");
		}
		readRule();
	}
	int EndIndex = currentIndex();
	closeAfterEnd();
	Block.Rules = Rules;
	int HeaderBefore = Rules.empty() ? EndIndex : RuleFirst[0];
	buildList(Block.List, HeaderBefore, HeaderBefore, EndIndex);
}

// TBehaviourDatabase::TBehaviourDatabase.
void TNpcReader::readBehaviour() {
	readSymbol('{');
	OpenIndex = currentIndex();
	nextToken();
	while (true) {
		if (isSpecial('}')) {
			CloseIndex = currentIndex();
			break;
		}
		readRule();
	}
}

// One rule of TBehaviourDatabase's loop: its conditions (unless it starts
// with "->") and its actions.
void TNpcReader::readRule() {
	TNpcRule Rule;
	int First = currentIndex();
	if (!isSpecial('I')) {
		while (true) {
			bool Success = false;
			if (Script.Token == STRING) {
				if (Current.Text.empty()) {
					refuse(Current, "an empty word, which the game made no string of (react would crash on it)");
				}
				Rule.Conditions.push_back(Word(Script.getString()));
				Success = true;
			}
			if (Script.Token == IDENTIFIER) {
				std::string Ident = Script.getIdentifier();
				for (const TProperty &Property : PROPERTIES) {
					if (Ident == Property.Identifier) {
						Rule.Conditions.push_back(Name(Property.Name));
						Success = true;
					}
				}
			}
			if (isSpecial('!')) {
				Rule.Conditions.push_back(Name("FORCE"));
				Success = true;
			}
			if (isSpecial('%')) {
				int Nr = readNumber();
				if (Nr < 1 || Nr > MAX_NUMBER_PARAMETERS) {
					gameError("illegal ordinal number");
				}
				Rule.Conditions.push_back(Parameter(Nr));
				Success = true;
			}

			if (!Success) {
				TNpcElement Term1 = readTerm();
				if (Script.Token != SPECIAL) {
					gameError("relational operator expected");
				}
				const char *Operator = nullptr;
				switch (Script.getSpecial()) {
					case '<': Operator = "lt"; break;
					case '>': Operator = "gt"; break;
					case '=': Operator = "eq"; break;
					case 'N': Operator = "ne"; break;
					case 'L': Operator = "le"; break;
					case 'G': Operator = "ge"; break;
					default:
						gameError("relational operator expected");
				}
				nextToken();
				TNpcElement Term2 = readTerm();
				Rule.Conditions.push_back(Call(Operator, {Term1, Term2}));
			} else {
				nextToken();
			}

			if (!isSpecial(',')) {
				break;
			}
			nextToken();
		}

		if (!isSpecial('I')) {
			gameError("'->' expected");
		}
	}

	while (true) {
		nextToken();
		if (Script.Token == STRING) {
			if (Current.Text.empty()) {
				refuse(Current, "an empty text action, which the game made no string of (react would crash on it)");
			}
			Rule.Actions.push_back(Word(Script.getString()));
			nextToken();
		} else if (Script.Token == IDENTIFIER) {
			std::string Ident = Script.getIdentifier();
			const TActionName *Action = nullptr;
			for (const TActionName &Candidate : ACTIONS) {
				if (Ident == Candidate.Identifier) {
					Action = &Candidate;
				}
			}
			if (Action == nullptr) {
				gameError("unknown identifier");
			}
			switch (Action->Form) {
				case NOP:
					Rule.Actions.push_back(Name(Action->Name));
					nextToken();
					break;
				case SET:
					readSymbol('=');
					nextToken();
					Rule.Actions.push_back(Call(Action->Name, {readTerm()}));
					break;
				case TWO: {
					readSymbol('(');
					nextToken();
					TNpcElement Param1 = readTerm();
					if (!isSpecial(',')) {
						gameError(", expected");
					}
					nextToken();
					TNpcElement Param2 = readTerm();
					if (!isSpecial(')')) {
						gameError(") expected");
					}
					Rule.Actions.push_back(Call(Action->Name, {Param1, Param2}));
					nextToken();
					break;
				}
				case ONE:
					readSymbol('(');
					nextToken();
					Rule.Actions.push_back(Call(Action->Name, {readTerm()}));
					if (!isSpecial(')')) {
						gameError(") expected");
					}
					nextToken();
					break;
				case NONE:
					Rule.Actions.push_back(Name(Action->Name));
					nextToken();
					break;
				case THREE: {
					readSymbol('(');
					nextToken();
					TNpcElement Param1 = readTerm();
					if (!isSpecial(',')) {
						gameError(", expected");
					}
					nextToken();
					TNpcElement Param2 = readTerm();
					if (!isSpecial(',')) {
						gameError(", expected");
					}
					nextToken();
					TNpcElement Param3 = readTerm();
					if (!isSpecial(')')) {
						gameError(") expected");
					}
					Rule.Actions.push_back(Call(Action->Name, {Param1, Param2, Param3}));
					nextToken();
					break;
				}
			}
		} else if (isSpecial('*')) {
			// An .ndb read on its own may start with it: the NPC that
			// includes the block has its rules before.
			if (Rules.empty() && !InBlock) {
				gameError("no previous pattern");
			}
			Rule.Actions.push_back(Name("previous"));
			nextToken();
		} else {
			gameError("illegal action");
		}

		if (!isSpecial(',')) {
			break;
		}
	}

	Rules.push_back(Rule);
	RuleFirst.push_back(First);
	RuleLast.push_back(currentIndex() - 1);
}

// TBehaviourDatabase::readValue.
TNpcElement TNpcReader::readValue() {
	TNpcElement Node;
	bool Found = false;
	if (Script.Token == NUMBER) {
		Node = Number(Script.getNumber());
		Found = true;
	} else if (Script.Token == IDENTIFIER) {
		static const TProperty VALUES[] = {
			{"topic", "TOPIC"}, {"price", "PRICE"}, {"amount", "AMOUNT"}, {"level", "LEVEL"},
			{"maglevel", "MAGLEVEL"}, {"hp", "HP"}, {"burning", "BURNING"}, {"poison", "POISON"},
			{"countmoney", "COUNTMONEY"}, {"type", "TYPE"}, {"data", "DATA"},
		};
		static const TProperty FUNCTIONS[] = {
			{"count", "count"}, {"spellknown", "spellKnown"}, {"spelllevel", "spellLevel"},
			{"questvalue", "questValue"},
		};
		std::string Ident = Script.getIdentifier();
		for (const TProperty &Value : VALUES) {
			if (Ident == Value.Identifier) {
				Node = Name(Value.Name);
				Found = true;
			}
		}
		for (const TProperty &Function : FUNCTIONS) {
			if (Ident == Function.Identifier) {
				readSymbol('(');
				nextToken();
				Node = Call(Function.Name, {readTerm()});
				Found = true;
				if (!isSpecial(')')) {
					gameError(") expected");
				}
			}
		}
		if (Ident == "random") {
			readSymbol('(');
			nextToken();
			TNpcElement First = readTerm();
			if (!isSpecial(',')) {
				gameError(", expected");
			}
			nextToken();
			TNpcElement Second = readTerm();
			if (!isSpecial(')')) {
				gameError(") expected");
			}
			Node = Call("random", {First, Second});
			Found = true;
		}
	} else if (Script.Token == SPECIAL) {
		if (Script.getSpecial() != '%') {
			gameError("illegal character");
		}
		Node = Parameter(readNumber());
		Found = true;
	} else {
		gameError("illegal value");
	}

	if (!Found) {
		gameError("unknown value");
	}

	nextToken();
	return Node;
}

// TBehaviourDatabase::readFactor.
TNpcElement TNpcReader::readFactor() {
	TNpcElement Left = readValue();
	while (isSpecial('*')) {
		nextToken();
		TNpcElement Right = readValue();
		Left = Call("mul", {Left, Right});
	}
	return Left;
}

// TBehaviourDatabase::readTerm.
TNpcElement TNpcReader::readTerm() {
	TNpcElement Left = readFactor();
	while (isSpecial('+') || isSpecial('-')) {
		const char *Operator = isSpecial('+') ? "add" : "sub";
		nextToken();
		TNpcElement Right = readFactor();
		Left = Call(Operator, {Left, Right});
	}
	return Left;
}

// The includes of an .npc: each inside its behaviour block and at a rule
// boundary, so that the included file's tokens are whole rules.
void TNpcReader::checkIncludes(TNpcFile &Npc) {
	std::set<int> Boundaries(RuleFirst.begin(), RuleFirst.end());
	Boundaries.insert(CloseIndex);
	for (size_t i = 0; i < Scan.Events.size(); i++) {
		const TScanEvent &Event = Scan.Events[i];
		if (Event.Kind != TScanEvent::INCLUDE) {
			continue;
		}
		int Begin = Event.Before;
		int End = -1;
		for (size_t j = i + 1; j < Scan.Events.size() && End < 0; j++) {
			if (Scan.Events[j].Kind == TScanEvent::INCLUDE_END && Scan.Events[j].File == Event.Included) {
				End = Scan.Events[j].Before;
			}
		}
		if (End < 0 || Begin <= OpenIndex || End > CloseIndex) {
			refuse(Event.File, Event.Line, "an include outside the behaviour block");
		}
		if (Boundaries.count(Begin) == 0 || Boundaries.count(End) == 0) {
			refuse(Event.File, Event.Line, "an include that is not at a rule boundary (the included file must hold "
					"whole rules)");
		}
		std::string Block = Event.Text.substr(0, Event.Text.size() - 4);
		if (std::find(Npc.Blocks.begin(), Npc.Blocks.end(), Block) == Npc.Blocks.end()) {
			Npc.Blocks.push_back(Block);
		}
	}
}

// The written list of the file opened first: its comments up to the token
// HeaderBefore for the header; its rules, includes and comments from the
// token FirstToken to the token EndToken (an .npc's '}', an .ndb's end); the
// comments after it for the trailer. A comment inside a rule goes before it,
// one on the line where a rule or include ends after it.
void TNpcReader::buildList(TNpcList &List, int HeaderBefore, int FirstToken, int EndToken) {
	std::vector<int> TokenRule(Tokens.size(), -1);
	for (size_t r = 0; r < Rules.size(); r++) {
		for (int t = RuleFirst[r]; t <= RuleLast[r]; t++) {
			TokenRule[static_cast<size_t>(t)] = static_cast<int>(r);
		}
	}

	struct TPlaced {
		TNpcListItem Item;
		int FirstLine;
		int LastLine;
	};
	std::vector<TPlaced> Placed;
	bool InRule = false;
	size_t e = 0;
	const std::vector<TScanEvent> &Events = Scan.Events;

	for (; e < Events.size() && Events[e].Before <= HeaderBefore; e++) {
		if (Events[e].Kind == TScanEvent::COMMENT && Events[e].Depth == 0) {
			List.HeaderComments.push_back(Events[e].Text);
		}
	}

	auto Rule = [&](int Index) -> int {
		for (size_t i = 0; i < Placed.size(); i++) {
			if (Placed[i].Item.Kind == TNpcListItem::RULE && Placed[i].Item.Rule == Index) {
				return static_cast<int>(i);
			}
		}
		return -1;
	};

	for (int t = FirstToken;; t++) {
		for (; e < Events.size() && Events[e].Before <= t && Events[e].Before <= EndToken; e++) {
			const TScanEvent &Event = Events[e];
			if (Event.Depth != 0) {
				continue;
			}
			if (Event.Kind == TScanEvent::COMMENT) {
				if (InRule) {
					Placed.back().Item.CommentsBefore.push_back(Event.Text);
				} else if (!Placed.empty() && Placed.back().Item.Kind != TNpcListItem::COMMENT
						&& !Placed.back().Item.HasComment && Placed.back().LastLine == Event.Line) {
					Placed.back().Item.Comment = Event.Text;
					Placed.back().Item.HasComment = true;
				} else {
					TPlaced Comment;
					Comment.Item.Kind = TNpcListItem::COMMENT;
					Comment.Item.Comment = Event.Text;
					Comment.FirstLine = Comment.LastLine = Event.Line;
					Placed.push_back(Comment);
				}
			} else if (Event.Kind == TScanEvent::INCLUDE) {
				TPlaced Block;
				Block.Item.Kind = TNpcListItem::BLOCK;
				Block.Item.Block = Event.Text.substr(0, Event.Text.size() - 4);
				Block.FirstLine = Block.LastLine = Event.Line;
				// Its rules: those whose first token is the include's first
				// one up to the include's end.
				int End = -1;
				for (size_t j = e + 1; j < Events.size() && End < 0; j++) {
					if (Events[j].Kind == TScanEvent::INCLUDE_END && Events[j].File == Event.Included) {
						End = Events[j].Before;
					}
				}
				Block.Item.Rule = static_cast<int>(Rules.size());
				for (size_t r = 0; r < Rules.size(); r++) {
					if (RuleFirst[r] >= Event.Before && RuleFirst[r] < End) {
						Block.Item.Rule = std::min(Block.Item.Rule, static_cast<int>(r));
						Block.Item.RuleCount++;
					}
				}
				Placed.push_back(Block);
			}
		}
		if (t >= EndToken) {
			break;
		}
		const TScanToken &Token = Tokens[static_cast<size_t>(t)];
		int r = TokenRule[static_cast<size_t>(t)];
		if (Token.Depth != 0 || r < 0) {
			continue;
		}
		if (t == RuleFirst[static_cast<size_t>(r)]) {
			TPlaced Item;
			Item.Item.Kind = TNpcListItem::RULE;
			Item.Item.Rule = r;
			Item.FirstLine = Token.Line;
			Item.LastLine = Token.EndLine;
			Placed.push_back(Item);
			InRule = true;
		}
		if (t == RuleLast[static_cast<size_t>(r)]) {
			int i = Rule(r);
			Placed[static_cast<size_t>(i)].LastLine = Token.EndLine;
			InRule = false;
		}
	}

	for (; e < Events.size(); e++) {
		if (Events[e].Kind == TScanEvent::COMMENT && Events[e].Depth == 0) {
			List.TrailerComments.push_back(Events[e].Text);
		}
	}

	for (size_t i = 0; i < Placed.size(); i++) {
		if (i > 0 && Placed[i].FirstLine > Placed[i - 1].LastLine + 1) {
			TNpcListItem Blank;
			Blank.Kind = TNpcListItem::BLANK;
			List.Items.push_back(Blank);
		}
		List.Items.push_back(Placed[i].Item);
	}
}

bool SameElement(const TNpcElement &A, const TNpcElement &B) {
	if (A.Kind != B.Kind || A.Text != B.Text || A.Number != B.Number || A.Arguments.size() != B.Arguments.size()) {
		return false;
	}
	for (size_t i = 0; i < A.Arguments.size(); i++) {
		if (!SameElement(A.Arguments[i], B.Arguments[i])) {
			return false;
		}
	}
	return true;
}

bool SameElements(const std::vector<TNpcElement> &A, const std::vector<TNpcElement> &B) {
	if (A.size() != B.size()) {
		return false;
	}
	for (size_t i = 0; i < A.size(); i++) {
		if (!SameElement(A[i], B[i])) {
			return false;
		}
	}
	return true;
}

} // namespace

std::string BlockLocalName(const std::string &Block) {
	std::string Local;
	bool Upper = false;
	for (char c : Block) {
		if (c == '-') {
			Upper = true;
		} else {
			Local += Upper && c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
			Upper = false;
		}
	}
	return Local;
}

TNpcFile ReadNpcFile(const std::string &Path) {
	TNpcFile Npc;
	std::string File = BaseName(Path);
	if (!EndsWith(File, ".npc") || !ValidNpcFileName(File.substr(0, File.size() - 4))) {
		ThrowFor(Path, "an .npc file whose name cannot name its script npcs/<name>.lua (a-z, 0-9, '_', '-' and '.')");
	}
	Npc.FileName = File.substr(0, File.size() - 4);
	TNpcReader Reader(Path);
	try {
		Reader.readNpc(Npc);
	} catch (const char *Text) {
		Reader.closeScript();
		ThrowFor(Path, Text);
	}
	return Npc;
}

TNpcBlock ReadNpcBlock(const std::string &Path) {
	TNpcBlock Block;
	std::string File = BaseName(Path);
	if (!EndsWith(File, ".ndb") || !ValidBlockName(File.substr(0, File.size() - 4))
			|| ReservedName(BlockLocalName(File.substr(0, File.size() - 4)))) {
		ThrowFor(Path, "an .ndb file whose name cannot name its module lib/npcs/<name>.lua and its local (parts of "
				"a-z and 0-9 joined by single '-', starting with a letter; not a name the scripts use)");
	}
	Block.Name = File.substr(0, File.size() - 4);
	TNpcReader Reader(Path);
	try {
		Reader.readBlock(Block);
	} catch (const char *Text) {
		Reader.closeScript();
		ThrowFor(Path, Text);
	}
	return Block;
}

void ReadNpcFolder(const char *Folder, TNpcFolder &Result) {
	Result.Npcs.clear();
	Result.Blocks.clear();
	std::vector<std::string> NpcFiles, BlockFiles;
	DIR *WorkDir = opendir(Folder);
	if (WorkDir == nullptr) {
		error("InitNPCs: Subdirectory %s not found\n", Folder);
		throw "Cannot init NPCs";
	}
	while (true) {
		struct dirent *DirEntry = readdir(WorkDir);
		if (DirEntry == nullptr) {
			break;
		}
		std::string s = DirEntry->d_name;
		if (s.size() > 4 && EndsWith(s, ".npc")) {
			NpcFiles.push_back(s);
		}
		if (s.size() > 4 && EndsWith(s, ".ndb")) {
			BlockFiles.push_back(s);
		}
	}
	closedir(WorkDir);
	std::sort(BlockFiles.begin(), BlockFiles.end());

	std::set<std::string> Locals;
	for (const std::string &File : BlockFiles) {
		std::string Path = std::string(Folder) + "/" + File;
		Result.Blocks.push_back(ReadNpcBlock(Path));
		if (!Locals.insert(BlockLocalName(Result.Blocks.back().Name)).second) {
			ThrowFor(Path, "an .ndb file whose local name another block of the folder has too");
		}
	}

	for (const std::string &File : NpcFiles) {
		std::string Path = std::string(Folder) + "/" + File;
		Result.Npcs.push_back(ReadNpcFile(Path));
		const TNpcFile &Npc = Result.Npcs.back();
		for (const TNpcListItem &Item : Npc.List.Items) {
			if (Item.Kind != TNpcListItem::BLOCK) {
				continue;
			}
			const TNpcBlock *Block = nullptr;
			for (const TNpcBlock &Candidate : Result.Blocks) {
				if (Candidate.Name == Item.Block) {
					Block = &Candidate;
				}
			}
			bool Same = Block != nullptr && Block->Rules.size() == static_cast<size_t>(Item.RuleCount);
			for (int r = 0; Same && r < Item.RuleCount; r++) {
				const TNpcRule &A = Npc.Rules[static_cast<size_t>(Item.Rule + r)];
				const TNpcRule &B = Block->Rules[static_cast<size_t>(r)];
				Same = SameElements(A.Conditions, B.Conditions) && SameElements(A.Actions, B.Actions);
			}
			if (!Same) {
				ThrowFor(Path, ("the rules of " + Item.Block + ".ndb as the NPC read them are not the block's own "
						"(a fault of patella-convert)").c_str());
			}
		}
	}
}
