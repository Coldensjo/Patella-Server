// moveuse_dat.cc: the reader of the original moveuse.dat (moveuse_dat.hh).
// This is the game's LoadDataBase, with its LoadParameters, LoadCondition and
// LoadAction, as it was before moveuse.toml, moved here unchanged but for
// these things: the file name is given instead of made from DATAPATH; the
// condition and action counts are set to 0 first, as InitMoveUse did; the two
// log lines are left out (the TOML may go to standard output); the sections,
// the rules' order and their notes are noted for the conversion; and each
// condition and action is marked unknown before it is read, so that one of an
// unknown name, which the old reader let through with whatever its entry held
// (when no "(" followed the name), stays marked and the writer refuses it.
//
// GetSkillByName (crskill.cc) and GetInstanceAttributeByName with
// InstanceAttributeNames (map.cc), which the reader asks for the numbers of
// skills and instance attributes, are copied here unchanged: crskill.cc and
// map.cc cannot be compiled into the converter, and the game still uses them
// for the monsters and the map sectors.

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "script.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "crskill.hh"
#include "mudinc.hh"
#include "strings.hh"
#include "utils.hh"
#include "moveusedata.hh"

#include "moveuse_dat.hh"

// The marks of a condition and an action whose name the reader did not know.
static const ConditionType UNKNOWN_CONDITION = static_cast<ConditionType>(NUMBER_OF_CONDITION_TYPES);
static const ActionType UNKNOWN_ACTION = static_cast<ActionType>(NUMBER_OF_ACTION_TYPES);

char InstanceAttributeNames[INSTANCEATTRIBUTES][30] = {
	"Content",
	"ChestQuestNumber",
	"Amount",
	"KeyNumber",
	"KeyholeNumber",
	"Level",
	"DoorQuestNumber",
	"DoorQuestValue",
	"Charges",
	"String",
	"Editor",
	"ContainerLiquidType",
	"PoolLiquidType",
	"AbsTeleportDestination",
	"Responsible",
	"RemainingExpireTime",
	"SavedExpireTime",
	"RemainingUses",
};

int GetSkillByName(char *Name) {
	if (strcmp(Name, "level") == 0) {
		return SKILL_LEVEL;
	}
	if (strcmp(Name, "magiclevel") == 0) {
		return SKILL_MAGICLEVEL;
	}
	if (strcmp(Name, "hitpoints") == 0) {
		return SKILL_HITPOINTS;
	}
	if (strcmp(Name, "mana") == 0) {
		return SKILL_MANA;
	}
	if (strcmp(Name, "gostrength") == 0) {
		return SKILL_GOSTRENGTH;
	}
	if (strcmp(Name, "carrystrength") == 0) {
		return SKILL_CARRYSTRENGTH;
	}
	if (strcmp(Name, "shielding") == 0) {
		return SKILL_SHIELDING;
	}
	if (strcmp(Name, "distancefighting") == 0) {
		return SKILL_DISTANCEFIGHTING;
	}
	if (strcmp(Name, "swordfighting") == 0) {
		return SKILL_SWORDFIGHTING;
	}
	if (strcmp(Name, "clubfighting") == 0) {
		return SKILL_CLUBFIGHTING;
	}
	if (strcmp(Name, "axefighting") == 0) {
		return SKILL_AXEFIGHTING;
	}
	if (strcmp(Name, "fistfighting") == 0) {
		return SKILL_FISTFIGHTING;
	}
	if (strcmp(Name, "magicdefense") == 0) {
		return SKILL_MAGICDEFENSE;
	}
	if (strcmp(Name, "fishing") == 0) {
		return SKILL_FISHING;
	}
	if (strcmp(Name, "eating") == 0) {
		return SKILL_EATING;
	}
	if (strcmp(Name, "shining") == 0) {
		return SKILL_SHINING;
	}
	if (strcmp(Name, "illusion") == 0) {
		return SKILL_ILLUSION;
	}
	if (strcmp(Name, "poison") == 0) {
		return SKILL_POISON;
	}
	if (strcmp(Name, "burning") == 0) {
		return SKILL_BURNING;
	}
	if (strcmp(Name, "energy") == 0) {
		return SKILL_ENERGY;
	}
	if (strcmp(Name, "drunken") == 0) {
		return SKILL_DRUNKEN;
	}
	if (strcmp(Name, "manashield") == 0) {
		return SKILL_MANASHIELD;
	}
	if (strcmp(Name, "soulpoints") == 0) {
		return SKILL_SOULPOINTS;
	}
	return -1;
}

int GetInstanceAttributeByName(char *Name) {
	for (int i = 0; i < INSTANCEATTRIBUTES; i++) {
		if (stricmp(InstanceAttributeNames[i], Name, -1) == 0) {
			return i;
		}
	}

	return -1;
}

static void LoadParameters(TReadScriptFile &Script, int *Parameters, int NumberOfParameters, ...) {
	if (NumberOfParameters > MAX_PARAMETERS) {
		error("LoadParameters: Zu viele Parameter (%d).\n", NumberOfParameters);
		Script.error("too many parameters");
	}

	if (NumberOfParameters == 0) {
		return;
	}

	if (Script.Token != SPECIAL || Script.getSpecial() != '(') {
		Script.error("'(' expected");
	}

	va_list va;
	char *Ident;
	int x, y, z;
	va_start(va, NumberOfParameters);
	for (int i = 0; i < NumberOfParameters; i++) {
		if (i > 0) {
			Script.readSymbol(',');
		}

		switch (va_arg(va, int)) {
			case PARAMETER_OBJECT:
				Ident = Script.readIdentifier();
				if (strcmp(Ident, "null") == 0) {
					Parameters[i] = EVENTOBJECT_NULL;
				} else if (strcmp(Ident, "obj1") == 0) {
					Parameters[i] = EVENTOBJECT_OBJ1;
				} else if (strcmp(Ident, "obj2") == 0) {
					Parameters[i] = EVENTOBJECT_OBJ2;
				} else if (strcmp(Ident, "user") == 0) {
					Parameters[i] = EVENTOBJECT_USER;
				} else if (strcmp(Ident, "temp") == 0) {
					Parameters[i] = EVENTOBJECT_TEMP;
				} else {
					Script.error("Object expected");
				}
				break;
			case PARAMETER_TYPE:
				Parameters[i] = Script.readNumber();
				if (!ObjectTypeExists(Parameters[i])) {
					Script.error("Unknown object type");
				}
				break;
			case PARAMETER_FLAG:
				Parameters[i] = GetFlagByName(Script.readIdentifier());
				if (Parameters[i] == -1) {
					Script.error("Unknown flag");
				}
				break;
			case PARAMETER_TYPEATTRIBUTE:
				Parameters[i] = GetTypeAttributeByName(Script.readIdentifier());
				if (Parameters[i] == -1) {
					Script.error("Unknown type attribute");
				}
				break;
			case PARAMETER_INSTANCEATTRIBUTE:
				Parameters[i] = GetInstanceAttributeByName(Script.readIdentifier());
				if (Parameters[i] == -1) {
					Script.error("Unknown instance attribute");
				}
				break;
			case PARAMETER_COORDINATE:
				Script.readCoordinate(x, y, z);
				Parameters[i] = PackAbsoluteCoordinate(x, y, z);
				break;
			case PARAMETER_VECTOR:
				Script.readCoordinate(x, y, z);
				Parameters[i] = PackRelativeCoordinate(x, y, z);
				break;
			case PARAMETER_RIGHT:
				Ident = Script.readIdentifier();
				if (strcmp(Ident, "premium_account") == 0) {
					Parameters[i] = PREMIUM_ACCOUNT;
				} else if (strcmp(Ident, "special_moveuse") == 0) {
					Parameters[i] = SPECIAL_MOVEUSE;
				} else {
					Script.error("Unknown right");
				}
				break;
			case PARAMETER_SKILL:
				Parameters[i] = GetSkillByName(Script.readIdentifier());
				if (Parameters[i] == -1) {
					Script.error("Unknown skill");
				}
				break;
			case PARAMETER_NUMBER:
				Parameters[i] = Script.readNumber();
				break;
			case PARAMETER_TEXT:
				Parameters[i] = AddDynamicString(Script.readString());
				break;
			case PARAMETER_COMPARISON:
				Parameters[i] = Script.readSpecial();
				if (strchr("<L=G>N", Parameters[i]) == nullptr) {
					Script.error("Unknown comparison operator");
				}
				break;
		}
	}
	va_end(va);

	Script.readSymbol(')');
	Script.nextToken();
}

static void LoadCondition(TReadScriptFile &Script, TMoveUseCondition &Condition) {
	char Ident[30];
	strcpy(Ident, Script.getIdentifier());
	Script.nextToken();
	if (strcmp(Ident, "isposition") == 0) {
		Condition.Condition = CONDITION_ISPOSITION;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "istype") == 0) {
		Condition.Condition = CONDITION_ISTYPE;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_TYPE);
	}
	if (strcmp(Ident, "iscreature") == 0) {
		Condition.Condition = CONDITION_ISCREATURE;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "isplayer") == 0) {
		Condition.Condition = CONDITION_ISPLAYER;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "hasflag") == 0) {
		Condition.Condition = CONDITION_HASFLAG;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_FLAG);
	}
	if (strcmp(Ident, "hastypeattribute") == 0) {
		Condition.Condition = CONDITION_HASTYPEATTRIBUTE;
		LoadParameters(Script, Condition.Parameters, 4, PARAMETER_OBJECT, PARAMETER_TYPEATTRIBUTE, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "hasinstanceattribute") == 0) {
		Condition.Condition = CONDITION_HASINSTANCEATTRIBUTE;
		LoadParameters(Script, Condition.Parameters, 4, PARAMETER_OBJECT, PARAMETER_INSTANCEATTRIBUTE, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "hastext") == 0) {
		Condition.Condition = CONDITION_HASTEXT;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_TEXT);
	}
	if (strcmp(Ident, "ispeaceful") == 0) {
		Condition.Condition = CONDITION_ISPEACEFUL;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "maylogout") == 0) {
		Condition.Condition = CONDITION_MAYLOGOUT;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "hasprofession") == 0) {
		Condition.Condition = CONDITION_HASPROFESSION;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "haslevel") == 0) {
		Condition.Condition = CONDITION_HASLEVEL;
		LoadParameters(Script, Condition.Parameters, 3, PARAMETER_OBJECT, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "hasright") == 0) {
		Condition.Condition = CONDITION_HASRIGHT;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_RIGHT);
	}
	if (strcmp(Ident, "hasquestvalue") == 0) {
		Condition.Condition = CONDITION_HASQUESTVALUE;
		LoadParameters(Script, Condition.Parameters, 4, PARAMETER_OBJECT, PARAMETER_NUMBER, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "testskill") == 0) {
		Condition.Condition = CONDITION_TESTSKILL;
		LoadParameters(Script, Condition.Parameters, 4, PARAMETER_OBJECT, PARAMETER_SKILL, PARAMETER_NUMBER, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "countobjects") == 0) {
		Condition.Condition = CONDITION_COUNTOBJECTS;
		LoadParameters(Script, Condition.Parameters, 3, PARAMETER_OBJECT, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "countobjectsonmap") == 0) {
		Condition.Condition = CONDITION_COUNTOBJECTSONMAP;
		LoadParameters(Script, Condition.Parameters, 3, PARAMETER_COORDINATE, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "isobjectthere") == 0) {
		Condition.Condition = CONDITION_ISOBJECTTHERE;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_TYPE);
	}
	if (strcmp(Ident, "iscreaturethere") == 0) {
		Condition.Condition = CONDITION_ISCREATURETHERE;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "isplayerthere") == 0) {
		Condition.Condition = CONDITION_ISPLAYERTHERE;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "isobjectininventory") == 0) {
		Condition.Condition = CONDITION_ISOBJECTININVENTORY;
		LoadParameters(Script, Condition.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "isprotectionzone") == 0) {
		Condition.Condition = CONDITION_ISPROTECTIONZONE;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "ishouse") == 0) {
		Condition.Condition = CONDITION_ISHOUSE;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "ishouseowner") == 0) {
		Condition.Condition = CONDITION_ISHOUSEOWNER;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "isdressed") == 0) {
		Condition.Condition = CONDITION_ISDRESSED;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "random") == 0) {
		Condition.Condition = CONDITION_RANDOM;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_NUMBER);
	}
}

static void LoadAction(TReadScriptFile &Script, TMoveUseAction &Action) {
	char Ident[30];
	strcpy(Ident, Script.getIdentifier());
	Script.nextToken();
	if (strcmp(Ident, "createonmap") == 0) {
		Action.Action = ACTION_CREATEONMAP;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "create") == 0) {
		Action.Action = ACTION_CREATE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "monsteronmap") == 0) {
		Action.Action = ACTION_MONSTERONMAP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "monster") == 0) {
		Action.Action = ACTION_MONSTER;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "effectonmap") == 0) {
		Action.Action = ACTION_EFFECTONMAP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "effect") == 0) {
		Action.Action = ACTION_EFFECT;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "textonmap") == 0) {
		Action.Action = ACTION_TEXTONMAP;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_COORDINATE, PARAMETER_TEXT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "text") == 0) {
		Action.Action = ACTION_TEXT;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TEXT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "changeonmap") == 0) {
		Action.Action = ACTION_CHANGEONMAP;
		LoadParameters(Script, Action.Parameters, 4, PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "change") == 0) {
		Action.Action = ACTION_CHANGE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "changerel") == 0) {
		Action.Action = ACTION_CHANGEREL;
		LoadParameters(Script, Action.Parameters, 5, PARAMETER_OBJECT, PARAMETER_VECTOR, PARAMETER_TYPE, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "setattribute") == 0) {
		Action.Action = ACTION_SETATTRIBUTE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_INSTANCEATTRIBUTE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "changeattribute") == 0) {
		Action.Action = ACTION_CHANGEATTRIBUTE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_INSTANCEATTRIBUTE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "setquestvalue") == 0) {
		Action.Action = ACTION_SETQUESTVALUE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_NUMBER, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "damage") == 0) {
		Action.Action = ACTION_DAMAGE;
		LoadParameters(Script, Action.Parameters, 4, PARAMETER_OBJECT, PARAMETER_OBJECT, PARAMETER_NUMBER, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "setstart") == 0) {
		Action.Action = ACTION_SETSTART;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "writename") == 0) {
		Action.Action = ACTION_WRITENAME;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TEXT, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "writetext") == 0) {
		Action.Action = ACTION_WRITETEXT;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_TEXT, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "logout") == 0) {
		Action.Action = ACTION_LOGOUT;
		LoadParameters(Script, Action.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "moveallonmap") == 0) {
		Action.Action = ACTION_MOVEALLONMAP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "moveall") == 0) {
		Action.Action = ACTION_MOVEALL;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "moveallrel") == 0) {
		Action.Action = ACTION_MOVEALLREL;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_VECTOR);
	}
	if (strcmp(Ident, "movetoponmap") == 0) {
		Action.Action = ACTION_MOVETOPONMAP;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "movetop") == 0) {
		Action.Action = ACTION_MOVETOP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "movetoprel") == 0) {
		Action.Action = ACTION_MOVETOPREL;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_VECTOR);
	}
	if (strcmp(Ident, "move") == 0) {
		Action.Action = ACTION_MOVE;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "moverel") == 0) {
		Action.Action = ACTION_MOVEREL;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_OBJECT, PARAMETER_VECTOR);
	}
	if (strcmp(Ident, "retrieve") == 0) {
		Action.Action = ACTION_RETRIEVE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_VECTOR, PARAMETER_VECTOR);
	}
	if (strcmp(Ident, "deleteallonmap") == 0) {
		Action.Action = ACTION_DELETEALLONMAP;
		LoadParameters(Script, Action.Parameters, 1, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "deletetoponmap") == 0) {
		Action.Action = ACTION_DELETETOPONMAP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_TYPE);
	}
	if (strcmp(Ident, "deleteonmap") == 0) {
		Action.Action = ACTION_DELETEONMAP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_TYPE);
	}
	if (strcmp(Ident, "delete") == 0) {
		Action.Action = ACTION_DELETE;
		LoadParameters(Script, Action.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "deleteininventory") == 0) {
		Action.Action = ACTION_DELETEININVENTORY;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "description") == 0) {
		Action.Action = ACTION_DESCRIPTION;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "loaddepot") == 0) {
		Action.Action = ACTION_LOADDEPOT;
		LoadParameters(Script, Action.Parameters, 4, PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_OBJECT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "savedepot") == 0) {
		Action.Action = ACTION_SAVEDEPOT;
		LoadParameters(Script, Action.Parameters, 4, PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_OBJECT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "sendmail") == 0) {
		Action.Action = ACTION_SENDMAIL;
		LoadParameters(Script, Action.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "nop") == 0) {
		Action.Action = ACTION_NOP;
		LoadParameters(Script, Action.Parameters, 0);
	}
}

void ReadMoveUseDat(const char *FileName, TMoveUseDatExtras &Extras) {
	NumberOfMoveUseConditions = 0;
	NumberOfMoveUseActions = 0;

	TReadScriptFile Script;
	Script.open(const_cast<char *>(FileName));
	for (int i = 0; i < NUMBER_OF_EVENT_TYPES; i++) {
		MoveUseDatabases[i].NumberOfRules = 0;
	}

	Script.nextToken();
	while (Script.Token != ENDOFFILE) {
		char Ident[30];
		strcpy(Ident, Script.getIdentifier());
		if (strcmp(Ident, "begin") == 0) {
			TMoveUseDatEntry Begin;
			Begin.Kind = TMoveUseDatEntry::BEGIN;
			Begin.Name = Script.readString();
			Extras.Entries.push_back(Begin);
			Script.nextToken();
			continue;
		}

		if (strcmp(Ident, "end") == 0) {
			TMoveUseDatEntry End;
			End.Kind = TMoveUseDatEntry::END;
			Extras.Entries.push_back(End);
			Script.nextToken();
			continue;
		}

		int EventType = -1;
		if (strcmp(Ident, "use") == 0) {
			EventType = EVENT_USE;
		}
		if (strcmp(Ident, "multiuse") == 0) {
			EventType = EVENT_MULTIUSE;
		}
		if (strcmp(Ident, "movement") == 0) {
			EventType = EVENT_MOVEMENT;
		}
		if (strcmp(Ident, "collision") == 0) {
			EventType = EVENT_COLLISION;
		}
		if (strcmp(Ident, "separation") == 0) {
			EventType = EVENT_SEPARATION;
		}
		if (EventType == -1) {
			Script.error("Unknown event type");
		}

		MoveUseDatabases[EventType].NumberOfRules++;
		TMoveUseRule &Rule = MoveUseDatabases[EventType].Rules(MoveUseDatabases[EventType].NumberOfRules);
		Rule.FirstCondition = NumberOfMoveUseConditions + 1;
		Script.readSymbol(',');
		while (true) {
			Script.nextToken();
			NumberOfMoveUseConditions++;
			TMoveUseCondition &Condition = MoveUseConditions(NumberOfMoveUseConditions);
			Condition.Modifier = MODIFIER_NORMAL;
			if (Script.Token == SPECIAL && Script.getSpecial() == '!') {
				Condition.Modifier = MODIFIER_INVERT;
				Script.nextToken();
			} else if (Script.Token == SPECIAL && Script.getSpecial() == '~') {
				Condition.Modifier = MODIFIER_TRUE;
				Script.nextToken();
			}

			Condition.Condition = UNKNOWN_CONDITION;
			LoadCondition(Script, Condition);
			if (Script.Token != SPECIAL || Script.getSpecial() != ',') {
				break;
			}
		}

		if (Script.Token != SPECIAL || Script.getSpecial() != 'I') {
			Script.error("'->' expected");
		}

		Rule.LastCondition = NumberOfMoveUseConditions;
		Script.nextToken();
		Rule.FirstAction = NumberOfMoveUseActions + 1;
		while (true) {
			NumberOfMoveUseActions++;
			TMoveUseAction &Action = MoveUseActions(NumberOfMoveUseActions);
			Action.Action = UNKNOWN_ACTION;
			LoadAction(Script, Action);
			if (Script.Token != SPECIAL || Script.getSpecial() != ',') {
				break;
			}
			Script.nextToken();
		}

		TMoveUseDatEntry Entry;
		Entry.Kind = TMoveUseDatEntry::RULE;
		Entry.Event = EventType;
		Entry.Rule = MoveUseDatabases[EventType].NumberOfRules;
		if (Script.Token == STRING) {
			Entry.HasNote = true;
			Entry.Note = Script.getString();
			Script.nextToken();
		}
		Extras.Entries.push_back(Entry);
		Rule.LastAction = NumberOfMoveUseActions;
	}

	Script.close();
}
