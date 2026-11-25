// config.hh: the server's settings (config.cc): the paths, the world config and
// the other values read from config.toml or from the query manager.

#ifndef PATELLA_CONFIG_HH
#define PATELLA_CONFIG_HH

#include <string>
#include <vector>

enum TWorldType {
	NORMAL = 0,
	NON_PVP = 1,
	PVP_ENFORCED = 2,
};

extern char DATAPATH[4096];
extern char MAPPATH[4096];
extern char ORIGMAPPATH[4096];
extern char USERPATH[4096];
extern char LOGPATH[4096];
extern char SAVEPATH[4096];
extern char BINPATH[4096];
// The file of the server's RSA private key (rsa_key, by default key.pem in
// the config file's folder), as a full path unless written as an absolute one.
extern char RSAKEYPATH[4096];
extern int SHMKey;
extern int GamePort;
extern char GameAddress[16];
extern int DebugLevel;
extern bool PrivateWorld;
extern TWorldType WorldType;
extern char WorldName[30];
extern int MaxPlayers;
extern int MaxNewbies;
extern int PremiumPlayerBuffer;
extern int PremiumNewbieBuffer;
extern int Beat;
extern int RebootTime;

// [decay]: how long objects lie before they decay, as multipliers of the
// times items.toml gives (total_expire_time), each MIN_DECAY_MULTIPLIER to
// MAX_DECAY_MULTIPLIER and 1 by default: of the corpses monsters leave
// (monster_corpse), of those players leave (player_corpse), and of the liquid
// pools (blood): the splash a hit leaves, the pool a death leaves, and with
// that pool's type also a spilled liquid. map.cc's CronExpire applies them.
extern double MonsterCorpseDecay;
extern double PlayerCorpseDecay;
extern double BloodDecay;

const double MIN_DECAY_MULTIPLIER = 0.01;
const double MAX_DECAY_MULTIPLIER = 1000.0;

// Time (seconds, a type's TOTALEXPIRETIME) times a [decay] multiplier, rounded
// to whole seconds: at least 1, so that what decays still lies for a moment,
// and at most INT_MAX. A multiplier of 1 and a time of 0 or less give Time as
// it is.
int MultiplyDecayTime(int Time, double Multiplier);

// [rates]: whole-number multipliers of what players gain, each
// MIN_RATE_MULTIPLIER to MAX_RATE_MULTIPLIER and 1 by default: the experience
// a slain monster gives (experience; not that of a slain player), the mana
// spent that counts for the magic level (magic_level), and the tries of the
// other skills, fishing's included (skill). crnonpl.cc's ~TMonster,
// magic.cc's CheckMana and crskill.cc's TSkillProbe::Probe/ProbeValue apply
// them.
extern int ExperienceRate;
extern int MagicLevelRate;
extern int SkillRate;

const int MIN_RATE_MULTIPLIER = 1;
const int MAX_RATE_MULTIPLIER = 1000;

// Amount times a [rates] multiplier, at most INT_MAX. A multiplier of 1 gives
// Amount as it is.
int MultiplyRate(int Amount, int Multiplier);

// [spawn]: how fast monster homes bring their monsters back (speed: the
// home's regeneration time divided by it, 4 = four times as fast) and how
// many monsters a home holds (amount: its amount times it), each
// MIN_SPAWN_MULTIPLIER to MAX_SPAWN_MULTIPLIER and 1 by default.
// [spawn.monsters] gives a race (by its key in monsters.toml) its own speed
// and/or amount, which replace the global ones for that race; one it leaves
// out stays the global one. crnonpl.cc's LoadMonsterhomes applies them (at
// the start; monster_homes.toml is not reloaded) and checks the race keys.
extern double SpawnSpeed;
extern double SpawnAmount;

struct TMonsterSpawnRate {
	std::string Race;
	double Speed;       // 0: not given, the global one.
	double Amount;      // 0: not given, the global one.
	std::string Where;  // file:line:column of the entry, for errors.
};
extern std::vector<TMonsterSpawnRate> MonsterSpawnRates;

const double MIN_SPAWN_MULTIPLIER = 0.01;
const double MAX_SPAWN_MULTIPLIER = 1000.0;

// The speed and amount multipliers for the race with key RaceKey: its own
// from [spawn.monsters], or the global ones.
double GetSpawnSpeed(const char *RaceKey);
double GetSpawnAmount(const char *RaceKey);

// A home's regeneration time (seconds) divided by a speed multiplier, and
// its amount times an amount multiplier, each rounded, at least 1 and at most
// INT_MAX. A multiplier of 1 gives the value as it is.
int ApplySpawnSpeed(int Regeneration, double Speed);
int ApplySpawnAmount(int Amount, double Multiplier);

// [loot]: a multiplier of the chance of each item a monster carries
// (monsters.toml's loot, and the extra items of a raid's wave), chance,
// MIN_LOOT_MULTIPLIER to MAX_LOOT_MULTIPLIER and 1 by default; 2 makes every
// item twice as likely, at most certain. [loot.monsters] gives a race (by
// key) its own chance in place of the global one. crnonpl.cc's TMonster
// (a monster's items) and crmain.cc's ProcessMonsterRaids (a wave's extra
// items, with the global chance) apply it; LoadMonsterhomes checks the keys.
extern double LootChance;

struct TMonsterLootRate {
	std::string Race;
	double Chance;      // 0: not given, the global one.
	std::string Where;  // file:line:column of the entry, for errors.
};
extern std::vector<TMonsterLootRate> MonsterLootRates;

const double MIN_LOOT_MULTIPLIER = 0.01;
const double MAX_LOOT_MULTIPLIER = 1000.0;

// The loot multiplier for the race with key RaceKey: its own from
// [loot.monsters], or the global one.
double GetLootChance(const char *RaceKey);

// An item's Probability (it drops when random(0, 999) <= Probability, in
// Probability + 1 of 1000) with its chance times Multiplier: rounded, from 0
// (1 in 1000) to 999 (certain). A multiplier of 1, and a Probability of 999
// or more, give Probability as it is.
int ApplyLootChance(int Probability, double Multiplier);

// [death]: what a player loses by dying. penalty multiplies the part of the
// experience, magic level and skills lost (10 percent, 7 promoted, one less
// per blessing; those rules stay), MIN_DEATH_PENALTY to MAX_DEATH_PENALTY and
// 1 by default, the loss at most all; item_drop is the percent chance of
// each item worn to drop into the corpse (default 10); container_drop
// whether a container worn (the backpack) always drops (default true; false:
// it takes the chance of item_drop too). An amulet of loss and a red skull
// keep their rules. crplayer.cc's TPlayer::Death (DeathLoss) and crmain.cc's
// ~TCreature apply them.
extern double DeathPenalty;
extern int DeathItemDrop;
extern bool DeathContainerDrop;

const double MIN_DEATH_PENALTY = 0.0;
const double MAX_DEATH_PENALTY = 10.0;

// The points of a skill with Exp points lost by a death of Percent percent,
// times penalty: with penalty 1 (int64)Exp * Percent / 100, as ever;
// otherwise rounded, at most Exp, at least 0.
int DeathLoss(int Exp, int Percent);

// [regeneration]: whole-number multipliers of the hitpoints (hp) and mana
// (mana) a fed player regains each time (1 and 4 every few rounds, by
// vocation; and in a bed, for the time offline), MIN_RATE_MULTIPLIER to
// MAX_RATE_MULTIPLIER and 1 by default. The food lasts as long as ever.
// crmain.cc's ProcessCreatures and crplayer.cc's TPlayer::Regenerate apply
// them (MultiplyRate).
extern int HitpointRegeneration;
extern int ManaRegeneration;

// [soul], in seconds: a kill that gives a player at least their level in
// experience makes them regain one soul point every interval (120 by
// default; promoted_interval, 15, for a promoted player) for duration
// (240), and a bed gives one per offline_interval (900) of the time offline.
// Each 1 to MAX_SOUL_TIME; the intervals at most duration. crcombat.cc's
// DistributeExperiencePoints and crplayer.cc's TPlayer (login, promotion,
// Regenerate) apply them.
extern int SoulInterval;
extern int SoulPromotedInterval;
extern int SoulDuration;
extern int SoulOfflineInterval;

const int MAX_SOUL_TIME = 86400;

// [pvp]: fight_lock, the seconds a fight keeps a player from logging out
// (and, after an attack on a player, out of protection zones; 60 by
// default), and kill_lock, the same after a kill of a player (900); each 1
// to MAX_LOCK_TIME. red_skull_days (1 to 365, 30) is how long a red skull
// lasts. RedSkullKills and BanishmentKills are the unjustified kills within
// a day, a week and 30 days that give a red skull (red_skull, 3, 5 and 10)
// and a banishment as well (banishment, 6, 10 and 20); each 1 to
// MAX_MURDER_KILLS, the murders a player's data keeps. crcombat.cc,
// crmain.cc, crplayer.cc and magic.cc apply them (BlockLogout,
// TPlayer::RecordMurder and CheckPlayerkilling).
extern int FightLock;
extern int KillLock;
extern int RedSkullDays;
extern int RedSkullKills[3];
extern int BanishmentKills[3];

const int MAX_LOCK_TIME = 86400;
const int MAX_MURDER_KILLS = 20;

// [idle]: minutes, after how many minutes without an action a player is
// warned, and logged out a minute later (1 to MAX_IDLE_MINUTES, 15 by
// default; a player with NO_LOGOUT_BLOCK never is). connections.cc's
// TConnection::Process applies it.
extern int IdleMinutes;

const int MAX_IDLE_MINUTES = 1440;

// [lure]: how far a monster of a monster home may be lured. limit (LureLimit,
// true by default): a monster farther from its home's middle than radius
// (LureRadius, 1 to MAX_LURE_RADIUS, 1 by default) times the home's radius
// along x or y, or more than floors (LureFloors, 0 to MAX_LURE_FLOORS, 2 by
// default) floors above or below it, vanishes; false: it never does.
// crnonpl.cc's MonsterhomeInLureRange applies it. With other values than the
// defaults, a monster standing outside its home's area may walk anywhere when
// it is not attacking or fleeing (TMonster::MovePossible); with the defaults
// it may only step into the area, as in the original.
extern bool LureLimit;
extern double LureRadius;
extern int LureFloors;

const double MAX_LURE_RADIUS = 100.0;
const int MAX_LURE_FLOORS = 15;

// Whether [lure] is at its defaults (the original behaviour).
bool LureIsOriginal();

// [logout]: to_temple (LogoutToTemple, false by default): a player who logs
// out (any way: quit, lost connection, idle, server shutdown) comes back at
// the start position of the home town (the temple) instead of where the
// player left; the destructor TPlayer::~TPlayer saves it so. fight_block
// (LogoutFightBlock, true by default): a player may not log out during or
// right after a fight or a kill ([pvp] fight_lock and kill_lock), and a
// character whose connection is lost stays in the game until then; false:
// TCreature::LogoutPossible ignores those locks. The locks still keep the
// player out of protection zones and out of leaving a party.
extern bool LogoutToTemple;
extern bool LogoutFightBlock;

// [admin]: the admin port of patella-admin (admin.cc). enabled (AdminEnabled,
// false by default) opens it; port (AdminPort, 1 to 65535, 7177 by default)
// is its TCP port, on 127.0.0.1 only; password (AdminPassword, at most
// MAX_ADMIN_PASSWORD - 1 characters) is what each request must begin with,
// and must not be empty when enabled is true.
extern bool AdminEnabled;
extern int AdminPort;
const int MAX_ADMIN_PASSWORD = 64;
extern char AdminPassword[MAX_ADMIN_PASSWORD];

// [vocations]: what a player gains per level, by vocation (none, knight,
// paladin, sorcerer, druid: the numbers of the professions; a promoted one
// counts as its vocation): hitpoints (hp), mana (mana) and capacity
// (capacity), each 0 to MAX_LEVEL_GAIN. VocationGains[vocation][gain], gain
// in that order; -1 where config.toml gives none: the character keeps what
// it holds (set when it chose its vocation). crskill.cc's TSkillLevel::Jump
// sets the given ones before each level gained or lost.
const int VOCATIONS = 5;
const int LEVEL_GAINS = 3;
const int MAX_LEVEL_GAIN = 10000;
extern int VocationGains[VOCATIONS][LEVEL_GAINS];

// The key DisguisePassword is given for the query manager passwords.
extern const char PasswordKey[9];

void DisguisePassword(char *Password, const char *Key);

// Sets every setting to its value for a config that does not name it: empty
// paths (LoadConfig then sets RSAKEYPATH to key.pem in the file's folder),
// SHM key 0, debug level 1, a public world without a name, beat 200, the
// gameplay settings above at their defaults (the original behaviour; no
// per-race entries, no vocation gains), no query managers; and the world config to type
// NORMAL, reboot time 540, address 0.0.0.0, port 0 and no players.
void SetConfigDefaults();

// Reads the settings from the TOML file FileName, over the defaults. Throws a
// const char * "<file>:<line>:<column>: <what>" for a syntax error, an
// unknown key or a value of the wrong type or out of range.
void LoadConfig(const char *FileName);

// Reads the settings from FileName, or with nullptr from config.toml in the
// home folder ($home before $HOME, the current folder without either).
// Throws a const char * when the file does not exist or LoadConfig fails.
void ReadConfig(const char *FileName);

#endif
