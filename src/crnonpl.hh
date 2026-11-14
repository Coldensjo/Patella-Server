// crnonpl.hh: the functions and data of crnonpl.cc, the creatures that are
// not players: NPCs, monsters, the monster homes that spawn monsters, and the
// node store of the NPCs' behaviour expressions.

#ifndef PATELLA_CRNONPL_HH
#define PATELLA_CRNONPL_HH

#include <cstdint>

#include "containr.hh"

struct TCreature;
struct TMonsterhome;
struct TNode;
struct TNonplayer;

TNode *NewNode(int op, TNode *left, TNode *right);
TNode *NewNode(int type, int val);
void DeleteNode(TNode *Node);
void StartMonsterhomeTimer(int nr);
void LoadMonsterhomes();
void ProcessMonsterhomes();
void NotifyMonsterhomeOfDeath(int nr);
bool MonsterhomeInRange(int nr, int x, int y, int z);
void ChangeNPCState(TCreature *npc, int NewState, bool Stimulus);
TCreature *CreateMonster(int Race, int x, int y, int z, int Home, uint32_t Father, bool ShowEffect);
void ConvinceMonster(TCreature *Master, TCreature *Slave);
void ChallengeMonster(TCreature *Challenger, TCreature *Monster);
void InitNPCs();
void InitNonplayer();
void ExitNonplayer();

extern vector<TNonplayer *> NonplayerList;
extern int FirstFreeNonplayer;
extern vector<TMonsterhome> Monsterhome;
extern int Monsterhomes;
extern store<TNode, 256> *NodeTable;

#endif
