// info.hh: the queries of info.cc, the rights (RIGHT), the banishment actions,
// the liquid colours and the magic field types.

#ifndef PATELLA_INFO_HH
#define PATELLA_INFO_HH

#include <cstdint>
#include <cstdlib>

struct Object;
struct ObjectType;

inline bool CoordinateAvoid(int x, int y, int z) { return CoordinateFlag(x, y, z, AVOID); }

inline bool CoordinateHookable(int x, int y, int z) { return CoordinateFlag(x, y, z, HOOKSOUTH) || CoordinateFlag(x, y, z, HOOKEAST); }

inline bool CoordinateLayable(int x, int y, int z) { return CoordinateFlag(x, y, z, BANK) && !CoordinateFlag(x, y, z, UNLAY); }

inline bool MovePossible(int x, int y, int z) { return CoordinateFlag(x, y, z, BANK) && !CoordinateFlag(x, y, z, UNPASS); }

enum RIGHT {
	PREMIUM_ACCOUNT = 0,
	NOTATION = 1,
	NAMELOCK = 2,
	STATEMENT_REPORT = 3,
	BANISHMENT = 4,
	FINAL_WARNING = 5,
	IP_BANISHMENT = 6,
	KICK = 7,
	HOME_TELEPORT = 8,
	GAMEMASTER_BROADCAST = 9,
	ANONYMOUS_BROADCAST = 10,
	NO_BANISHMENT = 11,
	ALLOW_MULTICLIENT = 12,
	LOG_COMMUNICATION = 13,
	READ_GAMEMASTER_CHANNEL = 14,
	READ_TUTOR_CHANNEL = 15,
	HIGHLIGHT_HELP_CHANNEL = 16,
	SEND_BUGREPORTS = 17,
	NAME_INSULTING = 18,
	NAME_SENTENCE = 19,
	NAME_NONSENSICAL_LETTERS = 20,
	NAME_BADLY_FORMATTED = 21,
	NAME_NO_PERSON = 22,
	NAME_CELEBRITY = 23,
	NAME_COUNTRY = 24,
	NAME_FAKE_IDENTITY = 25,
	NAME_FAKE_POSITION = 26,
	STATEMENT_INSULTING = 27,
	STATEMENT_SPAMMING = 28,
	STATEMENT_ADVERT_OFFTOPIC = 29,
	STATEMENT_ADVERT_MONEY = 30,
	STATEMENT_NON_ENGLISH = 31,
	STATEMENT_CHANNEL_OFFTOPIC = 32,
	STATEMENT_VIOLATION_INCITING = 33,
	CHEATING_BUG_ABUSE = 34,
	CHEATING_GAME_WEAKNESS = 35,
	CHEATING_MACRO_USE = 36,
	CHEATING_MODIFIED_CLIENT = 37,
	CHEATING_HACKING = 38,
	CHEATING_MULTI_CLIENT = 39,
	CHEATING_ACCOUNT_TRADING = 40,
	CHEATING_ACCOUNT_SHARING = 41,
	GAMEMASTER_THREATENING = 42,
	GAMEMASTER_PRETENDING = 43,
	GAMEMASTER_INFLUENCE = 44,
	GAMEMASTER_FALSE_REPORTS = 45,
	KILLING_EXCESSIVE_UNJUSTIFIED = 46,
	DESTRUCTIVE_BEHAVIOUR = 47,
	SPOILING_AUCTION = 48,
	INVALID_PAYMENT = 49,
	TELEPORT_TO_CHARACTER = 50,
	TELEPORT_TO_MARK = 51,
	TELEPORT_VERTICAL = 52,
	TELEPORT_TO_COORDINATE = 53,
	LEVITATE = 54,
	SPECIAL_MOVEUSE = 55,
	MODIFY_GOSTRENGTH = 56,
	SHOW_COORDINATE = 57,
	RETRIEVE = 58,
	ENTER_HOUSES = 59,
	OPEN_NAMEDOORS = 60,
	INVULNERABLE = 61,
	UNLIMITED_MANA = 62,
	KEEP_INVENTORY = 63,
	ALL_SPELLS = 64,
	UNLIMITED_CAPACITY = 65,
	ZERO_CAPACITY = 66,
	ATTACK_EVERYWHERE = 67,
	NO_ATTACK = 68,
	NO_RUNES = 69,
	NO_LOGOUT_BLOCK = 70,
	GAMEMASTER_OUTFIT = 71,
	ILLUMINATE = 72,
	CHANGE_PROFESSION = 73,
	IGNORED_BY_MONSTERS = 74,
	SHOW_KEYHOLE_NUMBERS = 75,
	CREATE_OBJECTS = 76,
	CREATE_MONEY = 77,
	CREATE_MONSTERS = 78,
	CHANGE_SKILLS = 79,
	CLEANUP_FIELDS = 80,
	NO_STATISTICS = 81,
	RELOAD_SCRIPTS = 82,
};

const int FIRST_BANISHMENT_RIGHT = 18;
const int LAST_BANISHMENT_RIGHT = 49;
const int FIRST_NAMEBANISHMENT_RIGHT = 18;
const int LAST_NAMEBANISHMENT_RIGHT = 26;
const int FIRST_REPORT_RIGHT = 27;
const int LAST_REPORT_RIGHT = 45;

// What a gamemaster does with a reported player or statement: the Action of
// CheckBanishmentRight and PunishmentOrder, as the client's rule violation
// dialog sends it. Each one needs the rights that CheckBanishmentRight names
// and makes the query manager requests of ProcessPunishmentOrder (writer.cc).
const int BANISHMENT_ACTION_NOTATION = 0;
const int BANISHMENT_ACTION_NAMELOCK = 1;
const int BANISHMENT_ACTION_BANISHMENT = 2;
const int BANISHMENT_ACTION_NAMELOCK_BANISHMENT = 3;
const int BANISHMENT_ACTION_BANISHMENT_FINAL_WARNING = 4;
const int BANISHMENT_ACTION_NAMELOCK_BANISHMENT_FINAL_WARNING = 5;
const int BANISHMENT_ACTION_STATEMENT_REPORT = 6;

enum RESULT {
	ERROR = -1,
	NOERROR = 0,
	NOTACCESSIBLE = 1,
	NOTMOVABLE = 2,
	NOTTAKABLE = 3,
	NOROOM = 4,
	OUTOFRANGE = 5,
	OUTOFHOME = 6,
	CANNOTTHROW = 7,
	TOOHEAVY = 8,
	CROSSREFERENCE = 9,
	CONTAINERFULL = 10,
	WRONGPOSITION = 11,
	WRONGPOSITION2 = 12,
	WRONGCLOTHES = 13,
	HANDSNOTFREE = 14,
	HANDBLOCKED = 15,
	ONEWEAPONONLY = 16,
	NOMATCH = 17,
	NOTCUMULABLE = 18,
	TOOMANYPARTS = 19,
	EMPTYCONTAINER = 20,
	SPLITOBJECT = 21,
	NOKEYMATCH = 22,
	UPSTAIRS = 23,
	DOWNSTAIRS = 24,
	CREATURENOTEXISTING = 25,
	PLAYERNOTEXISTING = 26,
	PLAYERNOTONLINE = 27,
	NAMEAMBIGUOUS = 28,
	NOTUSABLE = 29,
	FEDUP = 30,
	DESTROYED = 31,
	SPELLUNKNOWN = 32,
	LOWMAGICLEVEL = 33,
	MAGICITEM = 34,
	NOTENOUGHMANA = 35,
	NOSKILL = 36,
	TARGETLOST = 37,
	OUTOFAMMO = 38,
	NOCREATURE = 39,
	TOOLONG = 40,
	TARGETOUTOFRANGE = 41,
	TARGETHIDDEN = 42,
	ATTACKNOTALLOWED = 43,
	NOWAY = 44,
	LOGINERROR = 45,
	LOGINABORT = 46,
	PROTECTIONZONE = 47,
	ENTERPROTECTIONZONE = 48,
	EXHAUSTED = 49,
	NOTINVITED = 50,
	NOPREMIUMACCOUNT = 51,
	MOVENOTPOSSIBLE = 52,
	ALREADYTRADING = 53,
	PARTNERTRADING = 54,
	TOOMANYOBJECTS = 55,
	TOOMANYSLAVES = 56,
	NOTTURNABLE = 57,
	SECUREMODE = 58,
	NOTENOUGHSOULPOINTS = 59,
	LOWLEVEL = 60,
};

const char *GetName(Object Obj);
const char *GetInfo(Object Obj);
int GetWeight(Object Obj, int Count);
int GetRowWeight(Object Obj);
int GetCompleteWeight(Object Obj);
uint32_t GetObjectCreatureID(Object Obj);
int GetObjectBodyPosition(Object Obj);
int GetObjectRNum(Object Obj);
bool ObjectInRange(uint32_t CreatureID, Object Obj, int Range);
bool ObjectAccessible(uint32_t CreatureID, Object Obj, int Range);
// The colours the client draws a liquid in, which GetLiquidColor gives for
// each liquid (objects.hh's LIQUID_*): water blue, wine and manafluid purple,
// blood and lifefluid red, slime green, urine and lemonade yellow, milk white;
// beer, mud and oil brown. 0 is no liquid.
const int LIQUID_COLOR_NONE = 0;
const int LIQUID_COLOR_BLUE = 1;
const int LIQUID_COLOR_RED = 2;
const int LIQUID_COLOR_BROWN = 3;
const int LIQUID_COLOR_GREEN = 4;
const int LIQUID_COLOR_YELLOW = 5;
const int LIQUID_COLOR_WHITE = 6;
const int LIQUID_COLOR_PURPLE = 7;

const char *GetLiquidName(uint32_t nr);
unsigned char GetLiquidColor(uint32_t nr);
int ObjectDistance(Object Obj1, Object Obj2);
Object GetBodyContainer(uint32_t CreatureID, int Position);
Object GetBodyObject(uint32_t CreatureID, int Position);
Object GetTopObject(int x, int y, int z, bool Move);
Object GetObject(uint32_t CreatureID, int x, int y, int z, int RNum, ObjectType Type);
Object GetRowObject(Object Obj, ObjectType Type, uint32_t Value, bool Recurse);
Object GetInventoryObject(uint32_t CreatureID, ObjectType Type, uint32_t Value);
int CountObjectsInContainer(Object Con);
int CountObjects(Object Obj);
int CountObjects(Object Obj, ObjectType Type, uint32_t Value);
int CountInventoryObjects(uint32_t CreatureID, ObjectType Type, uint32_t Value);
int CountMoney(Object Obj);
int CountInventoryMoney(uint32_t CreatureID);
void CalculateChange(int Amount, int &Gold, int &Platinum, int &Crystal);
int GetHeight(int x, int y, int z);
bool JumpPossible(int x, int y, int z, bool AvoidPlayers);
// The field types of magic.cc's CreateField, the magic fields its switch
// creates (the harmless kind of the first three on a non-PvP world for a
// peaceful caster); fire, poison and energy fields block the caster's logout.
// FieldPossible tests a magic wall and rush wood against every unpassable
// object, creatures included; the other fields only against objects that are
// no creatures.
const int FIELD_FIRE = 1;
const int FIELD_POISON = 2;
const int FIELD_ENERGY = 3;
const int FIELD_MAGICWALL = 4;
const int FIELD_RUSHWOOD = 5;

bool FieldPossible(int x, int y, int z, int Type);
bool SearchFreeField(int &x, int &y, int &z, int Distance, unsigned short HouseID, bool Jump);
bool SearchLoginField(int &x, int &y, int &z, int Distance, bool Player);
bool SearchSpawnField(int &x, int &y, int &z, int Distance, bool Player);
bool SearchFlightField(uint32_t FugitiveID, uint32_t PursuerID, int &x, int &y, int &z);
bool SearchSummonField(int &x, int &y, int &z, int Distance);
bool ThrowPossible(int sx, int sy, int sz, int zx, int zy, int zz, int Power);
void GetCreatureLight(uint32_t CreatureID, int &Brightness, int &Color);
int GetInventoryWeight(uint32_t CreatureID);
bool CheckRight(uint32_t CharacterID, RIGHT Right);
bool CheckBanishmentRight(uint32_t CharacterID, int Reason, int Action);
const char *GetBanishmentReason(int Reason);

#endif
