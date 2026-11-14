// moveuse_writer.cc: writes the game's move and use rules as moveuse.toml
// (moveuse_writer.hh).

#include "moveuse_writer.hh"

#include <cstdint>
#include <string>
#include <vector>

#include "crskill.hh"
#include "moveusedata.hh"
#include "objects.hh"
#include "strings.hh"
#include "tomlfile.hh"
#include "tomlwrite.hh"
#include "utils.hh"

// The names of the parameter types in the header's lists, by ParameterType.
static const char *const ParameterNames[] = {
	"object",
	"type",
	"flag",
	"type_attribute",
	"instance_attribute",
	"coordinate",
	"vector",
	"right",
	"skill",
	"number",
	"text",
	"comparison",
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

// The signatures as "name(type, type)" words.
static std::vector<std::string> Signatures(const TMoveUseSignature *List, int Count) {
	std::vector<std::string> Words;
	for (int i = 0; i < Count; i++) {
		std::string Word = std::string(List[i].Key) + "(";
		for (int j = 0; j < List[i].NumberOfParameters; j++) {
			Word += std::string(j > 0 ? ", " : "") + ParameterNames[List[i].Parameters[j]];
		}
		Words.push_back(Word + ")");
	}
	return Words;
}

static std::string Header() {
	std::string Text;
	Text += "# moveuse.toml: the move and use rules of the game server, converted from the\n";
	Text += "# original moveuse.dat by patella-convert. The game reads moveuse.toml in its\n";
	Text += "# data folder ([paths] data of config.toml). An unknown key or name, a value\n";
	Text += "# of the wrong type or out of range, or an object type that does not exist,\n";
	Text += "# is an error.\n";
	Text += "#\n";
	Text += "# When something happens to an object, the game tries the rules of that event\n";
	Text += "# in the order of this file and runs the actions of the first rule whose\n";
	Text += "# conditions all hold; the order of the rules matters. Each [[rule]] is one\n";
	Text += "# rule:\n";
	Text += "#   section     The sections the rule is in, outermost first (BEGIN and END\n";
	Text += "#               of moveuse.dat), for the reader. Left out, none.\n";
	Text += "#   event       What sets it off, and what obj1 and obj2 are then:\n";
	Text += "#               use         The user uses obj1.\n";
	Text += "#               multi_use   The user uses obj1 on obj2.\n";
	Text += "#               movement    obj1 is moved.\n";
	Text += "#               collision   obj1 and obj2 come to lie on one field, obj1\n";
	Text += "#                           being the object with the collision_event flag.\n";
	Text += "#               separation  obj1 and obj2 come apart, obj1 being the object\n";
	Text += "#                           with the separation_event flag.\n";
	Text += "#   note        A note on the rule, for the reader. Left out, none.\n";
	Text += "#   conditions  What must hold, in this order: at least one\n";
	Text += "#               { name = \"...\", args = [...] }. negate = true inverts one\n";
	Text += "#               (\"!\" in moveuse.dat); ignore_result = true makes one hold\n";
	Text += "#               whatever it gives (\"~\"; it is still checked and can still\n";
	Text += "#               set temp).\n";
	Text += "#   actions     What is done, in this order: at least one\n";
	Text += "#               { name = \"...\", args = [...] }; args is left out when there\n";
	Text += "#               are none.\n";
	Text += "#\n";
	Text += "# The arguments, in the order the lists below give them:\n";
	Text += "#   object              \"obj1\", \"obj2\", \"user\" (the creature that uses\n";
	Text += "#                       an object; none for movement, collision and\n";
	Text += "#                       separation), \"temp\" (the object the rule's last\n";
	Text += "#                       is_object_there, is_creature_there, is_player_there,\n";
	Text += "#                       is_object_in_inventory, create_on_map, create,\n";
	Text += "#                       change_on_map or change_rel found or made) or \"null\"\n";
	Text += "#                       (none).\n";
	Text += "#   type                An object type number (items.toml); the comment after\n";
	Text += "#                       the entry gives its name.\n";
	Text += "#   flag                A flag of the object types, as items.toml names it.\n";
	Text += "#   type_attribute      An attribute of the object types, as items.toml names\n";
	Text += "#                       it.\n";
	Text += "#   instance_attribute  One of:\n";
	std::vector<std::string> Words(InstanceAttributeKeys, InstanceAttributeKeys + INSTANCEATTRIBUTES);
	Text += CommentList(Words, "#                         ", ", ");
	Text += "#   coordinate          A field [x, y, z]: x and y 24576 to 40959, z 0 to 15.\n";
	Text += "#   vector              An offset [x, y, z] from an object's field: x and y\n";
	Text += "#                       -8192 to 8191, z -8 to 7.\n";
	Text += "#   right               \"premium_account\" or \"special_moveuse\".\n";
	Text += "#   skill               One of:\n";
	Words.assign(SkillKeys, SkillKeys + NUMBER_OF_SKILL_KEYS);
	Text += CommentList(Words, "#                         ", ", ");
	Text += "#   number              A 32-bit number.\n";
	Text += "#   text                A text.\n";
	Text += "#   comparison          \"<\", \"<=\", \"=\", \">=\", \">\" or \"<>\".\n";
	Text += "#\n";
	Text += "# The conditions:\n";
	Text += CommentList(Signatures(ConditionSignatures, NUMBER_OF_CONDITION_TYPES), "#   ", ", ");
	Text += "#\n";
	Text += "# The actions:\n";
	Text += CommentList(Signatures(ActionSignatures, NUMBER_OF_ACTION_TYPES), "#   ", ", ");
	return Text;
}

std::vector<bool> DefinedTypes(const char *ItemsToml) {
	std::vector<bool> Defined;
	toml::table Items = TomlParseFile(ItemsToml);
	if (const toml::array *Entries = TomlGetArray(Items, "item")) {
		for (const toml::node &Node : *Entries) {
			const toml::table *Entry = Node.as_table();
			const toml::node *Id = Entry != nullptr ? Entry->get("id") : nullptr;
			if (Id == nullptr || !Id->is_integer() || Id->as_integer()->get() < 0
					|| Id->as_integer()->get() > 65535) {
				throw "an [[item]] of items.toml without an id of 0 to 65535";
			}
			size_t TypeID = static_cast<size_t>(Id->as_integer()->get());
			if (Defined.size() <= TypeID) {
				Defined.resize(TypeID + 1, false);
			}
			Defined[TypeID] = true;
		}
	}
	return Defined;
}

// "<type>: <name>" for the comment after an entry.
static std::string TypeComment(int Type, const std::vector<bool> &Defined) {
	if (Type < 0 || static_cast<size_t>(Type) >= Defined.size() || !Defined[static_cast<size_t>(Type)]) {
		return std::to_string(Type) + ": (not in items.toml)";
	}
	const char *Name = ObjectTypes(Type).Name;
	std::string Text;
	if (Name == nullptr || Name[0] == 0) {
		Text = "(no name)";
	} else {
		for (const char *p = Name; *p != 0; p++) {
			unsigned char c = static_cast<unsigned char>(*p);
			Text += c < 0x20 || c == 0x7F ? '?' : *p;
		}
	}
	return std::to_string(Type) + ": " + Latin1ToUtf8(Text);
}

static std::string Triple(int x, int y, int z) {
	return "[" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + "]";
}

// A TMoveUseKey's key for Value, or nullptr.
static const char *KeyOf(const TMoveUseKey *Keys, int Count, int Value) {
	for (int i = 0; i < Count; i++) {
		if (Keys[i].Value == Value) {
			return Keys[i].Key;
		}
	}
	return nullptr;
}

// A key of Keys (Count of them) by its index Value as a TOML string.
static std::string Key(const char *const *Keys, int Count, int Value, const char *Refusal) {
	if (Value < 0 || Value >= Count) {
		throw Refusal;
	}
	return std::string("\"") + Keys[Value] + "\"";
}

// The args of a condition or action of Signature, and the type comments in
// Comments.
static std::string Arguments(const TMoveUseSignature &Signature, const int *Parameters,
		const std::vector<bool> &Defined, std::vector<std::string> &Comments) {
	std::string Text;
	for (int i = 0; i < Signature.NumberOfParameters; i++) {
		int Value = Parameters[i];
		int x, y, z;
		const char *Name;
		std::string Argument;
		switch (Signature.Parameters[i]) {
			case PARAMETER_OBJECT:
				Argument = Key(EventObjectKeys, NUMBER_OF_EVENTOBJECTS, Value, "an object the game does not know");
				break;
			case PARAMETER_TYPE:
				Argument = std::to_string(Value);
				Comments.push_back(TypeComment(Value, Defined));
				break;
			case PARAMETER_FLAG:
				Argument = Key(FlagKeys, FLAGS, Value, "a flag the game does not know");
				break;
			case PARAMETER_TYPEATTRIBUTE:
				Argument = Key(TypeAttributeKeys, TYPEATTRIBUTES, Value, "a type attribute the game does not know");
				break;
			case PARAMETER_INSTANCEATTRIBUTE:
				Argument = Key(InstanceAttributeKeys, INSTANCEATTRIBUTES, Value,
						"an instance attribute the game does not know");
				break;
			case PARAMETER_COORDINATE:
				UnpackAbsoluteCoordinate(static_cast<uint32_t>(Value), x, y, z);
				Argument = Triple(x, y, z);
				break;
			case PARAMETER_VECTOR:
				UnpackRelativeCoordinate(static_cast<uint32_t>(Value), x, y, z);
				Argument = Triple(x, y, z);
				break;
			case PARAMETER_RIGHT:
				Name = KeyOf(RightKeys, 2, Value);
				if (Name == nullptr) {
					throw "a right the game does not know";
				}
				Argument = std::string("\"") + Name + "\"";
				break;
			case PARAMETER_SKILL:
				Argument = Key(SkillKeys, NUMBER_OF_SKILL_KEYS, Value, "a skill the game does not know");
				break;
			case PARAMETER_NUMBER:
				Argument = std::to_string(Value);
				break;
			case PARAMETER_TEXT: {
				const char *String = GetDynamicString(static_cast<uint32_t>(Value));
				Argument = TomlQuote(String != nullptr ? String : "");
				break;
			}
			case PARAMETER_COMPARISON:
				Name = KeyOf(ComparisonKeys, 6, Value);
				if (Name == nullptr) {
					throw "a comparison other than <, <=, =, >=, > and <>, which moveuse.toml cannot hold";
				}
				Argument = std::string("\"") + Name + "\"";
				break;
		}
		Text += (i > 0 ? ", " : "") + Argument;
	}
	return Text;
}

// One line of a conditions or actions list.
static std::string EntryLine(const std::string &Name, const std::string &Modifier, const std::string &Args,
		const std::vector<std::string> &Comments) {
	std::string Text = "\t{ name = \"" + Name + "\"" + Modifier;
	if (!Args.empty()) {
		Text += ", args = [" + Args + "]";
	}
	Text += " },";
	for (size_t i = 0; i < Comments.size(); i++) {
		Text += (i == 0 ? "  # " : "; ") + Comments[i];
	}
	return Text + "\n";
}

static std::string RuleText(const TMoveUseDatEntry &Entry, const std::vector<std::string> &Section,
		const std::vector<bool> &Defined) {
	const TMoveUseRule &Rule = MoveUseDatabases[Entry.Event].Rules(Entry.Rule);
	std::string Text = "\n[[rule]]\n";
	if (!Section.empty()) {
		Text += "section = [";
		for (size_t i = 0; i < Section.size(); i++) {
			Text += (i > 0 ? ", " : "") + TomlQuote(Section[i]);
		}
		Text += "]\n";
	}
	Text += std::string("event = \"") + EventKeys[Entry.Event] + "\"\n";
	if (Entry.HasNote) {
		Text += "note = " + TomlQuote(Entry.Note) + "\n";
	}

	Text += "conditions = [\n";
	for (int i = Rule.FirstCondition; i <= Rule.LastCondition; i++) {
		const TMoveUseCondition &Condition = MoveUseConditions(i);
		if (Condition.Condition < 0 || Condition.Condition >= NUMBER_OF_CONDITION_TYPES) {
			throw "a condition the game does not know (an unknown name in moveuse.dat)";
		}
		std::string Modifier;
		if (Condition.Modifier == MODIFIER_INVERT) {
			Modifier = ", negate = true";
		} else if (Condition.Modifier == MODIFIER_TRUE) {
			Modifier = ", ignore_result = true";
		} else if (Condition.Modifier != MODIFIER_NORMAL) {
			throw "a condition modifier the game does not know";
		}
		const TMoveUseSignature &Signature = ConditionSignatures[Condition.Condition];
		std::vector<std::string> Comments;
		std::string Args = Arguments(Signature, Condition.Parameters, Defined, Comments);
		Text += EntryLine(Signature.Key, Modifier, Args, Comments);
	}
	Text += "]\n";

	Text += "actions = [\n";
	for (int i = Rule.FirstAction; i <= Rule.LastAction; i++) {
		const TMoveUseAction &Action = MoveUseActions(i);
		if (Action.Action < 0 || Action.Action >= NUMBER_OF_ACTION_TYPES) {
			throw "an action the game does not know (an unknown name in moveuse.dat)";
		}
		const TMoveUseSignature &Signature = ActionSignatures[Action.Action];
		std::vector<std::string> Comments;
		std::string Args = Arguments(Signature, Action.Parameters, Defined, Comments);
		Text += EntryLine(Signature.Key, "", Args, Comments);
	}
	Text += "]\n";
	return Text;
}

std::string MoveUseToToml(const TMoveUseDatExtras &Extras, const std::vector<bool> &Defined) {
	std::string Text = Header();

	// The sections come back from the rules' section lists: between two
	// rules, the ones the first is in beyond what both share end, and the
	// ones the second is in beyond it start. Any other BEGIN or END between
	// them would be lost.
	std::vector<std::string> Stack;
	std::vector<std::string> Last;
	int Changes = 0;
	auto Check = [&](const std::vector<std::string> &Next) {
		size_t Common = 0;
		while (Common < Last.size() && Common < Next.size() && Last[Common] == Next[Common]) {
			Common++;
		}
		if (static_cast<size_t>(Changes) != (Last.size() - Common) + (Next.size() - Common)) {
			throw "a section without rules, or two neighbouring sections of the same name, which moveuse.toml cannot "
					"tell apart";
		}
		Changes = 0;
		Last = Next;
	};
	for (const TMoveUseDatEntry &Entry : Extras.Entries) {
		if (Entry.Kind == TMoveUseDatEntry::BEGIN) {
			Stack.push_back(Entry.Name);
			Changes++;
		} else if (Entry.Kind == TMoveUseDatEntry::END) {
			if (Stack.empty()) {
				throw "an END without its BEGIN, which moveuse.toml cannot hold";
			}
			Stack.pop_back();
			Changes++;
		} else {
			Check(Stack);
			Text += RuleText(Entry, Stack, Defined);
		}
	}
	if (!Stack.empty()) {
		throw "a BEGIN without its END, which moveuse.toml cannot hold";
	}
	Check(Stack);
	return Text;
}
