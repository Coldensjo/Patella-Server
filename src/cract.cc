#include <cstdint>
#include <cstdlib>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "crinc.hh"
#include "operate.hh"
#include "connections.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"
#include "cract.hh"

struct TShortwayPoint {
	// inline  cract.c:34
	TShortwayPoint() { abort(); /* STUB */ }
	int x;
	int y;
	int Waypoints;
	int Waylength;
	int Heuristic;
	TShortwayPoint *Predecessor;
	TShortwayPoint *NextToExpand;
};

struct TShortway {
	TShortway(TCreature *cr, int VisibleX, int VisibleY);
	~TShortway();
	bool Calculate(int DestX, int DestY, bool MustReach, int MaxSteps);
private:
	void FillMap();
	void ClearMap();
	void Expand(TShortwayPoint *Node);
	matrix<TShortwayPoint> *Map;
	TShortwayPoint *FirstToExpand;
	TCreature *cr;
	int VisibleX;
	int VisibleY;
	int StartX;
	int StartY;
	int StartZ;
	int MinWaypoints;
};

// 0x080be040  cract.c:66
TShortway::TShortway(TCreature * /*cr*/, int /*VisibleX*/, int /*VisibleY*/) { abort(); /* STUB */ }

// 0x080be190  cract.c:91
TShortway::~TShortway() { abort(); /* STUB */ }

// 0x080be1d0  cract.c:102
void TShortway::FillMap() { abort(); /* STUB */ }

// 0x080be6d0  cract.c:136
void TShortway::ClearMap() { abort(); /* STUB */ }

// 0x080be8e0  cract.c:155
bool TShortway::Calculate(int /*DestX*/, int /*DestY*/, bool /*MustReach*/, int /*MaxSteps*/) { abort(); /* STUB */ }

// 0x080bec20  cract.c:190
void TShortway::Expand(TShortwayPoint * /*Node*/) { abort(); /* STUB */ }

// 0x080beed0  cract.c:244
bool TCreature::SetOnMap() { abort(); /* STUB */ }

// 0x080bf130  cract.c:288
bool TCreature::DelOnMap() { abort(); /* STUB */ }

// 0x080bf1c0  cract.c:308
void TCreature::Go(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x080bf6d0  cract.c:345
void TCreature::Rotate(unsigned char /*Dir*/) { abort(); /* STUB */ }

// 0x080bf710  cract.c:354
void TCreature::Rotate(TCreature * /*Target*/) { abort(); /* STUB */ }

// 0x080bf7b0  cract.c:371
void TCreature::Move(Object /*Obj*/, int /*zx*/, int /*zy*/, int /*zz*/, unsigned char /*Count*/) { abort(); /* STUB */ }

// 0x080c0200  cract.c:456
void TCreature::Trade(Object /*Obj*/, uint32_t /*PartnerID*/) { abort(); /* STUB */ }

// 0x080c0630  cract.c:498
void TCreature::Use(Object /*Obj1*/, Object /*Obj2*/, unsigned char /*Dummy*/) { abort(); /* STUB */ }

// 0x080c09c0  cract.c:522
void TCreature::Turn(Object /*Obj*/) { abort(); /* STUB */ }

// 0x080c0a40  cract.c:531
void TCreature::Attack() { abort(); /* STUB */ }

// 0x080c0a80  cract.c:543
void TCreature::NotifyGo() { abort(); /* STUB */ }

// 0x080c10d0  cract.c:639
void TCreature::NotifyTurn(Object /*DestCon*/) { abort(); /* STUB */ }

// 0x080c1160  cract.c:656
void TCreature::NotifyDelete() { abort(); /* STUB */ }

// 0x080c11d0  cract.c:670
void TCreature::NotifyCreate() { abort(); /* STUB */ }

// 0x080c1200  cract.c:679
void TCreature::NotifyChangeInventory() { abort(); /* STUB */ }

// 0x080c15c0  cract.c:736
void TCreature::CreatureMoveStimulus(uint32_t /*CreatureID*/, int /*Type*/) { abort(); /* STUB */ }

// 0x080c19d0  cract.c:763
int TCreature::CalculateDelay() { abort(); /* STUB */ }

// 0x080c1af0  cract.c:780
void TCreature::Execute() { abort(); /* STUB */ }

// 0x080c2070  cract.c:843
bool TCreature::ToDoClear() { abort(); /* STUB */ }

// 0x080c2150  cract.c:874
void TCreature::ToDoStop() { abort(); /* STUB */ }

// 0x080c2190  cract.c:884
void TCreature::ToDoAdd(TToDoEntry & /*TD*/) { abort(); /* STUB */ }

// 0x080c2220  cract.c:895
void TCreature::ToDoStart() { abort(); /* STUB */ }

// 0x080c22a0  cract.c:909
void TCreature::ToDoYield() { abort(); /* STUB */ }

// 0x080c2320  cract.c:919
void TCreature::ToDoWait(int /*Delay*/) { abort(); /* STUB */ }

// 0x080c23d0  cract.c:930
void TCreature::ToDoTalk(int /*Mode*/, char * /*Addressee*/, char * /*Text*/, bool /*CheckSpamming*/) { abort(); /* STUB */ }

// 0x080c24f0  cract.c:947
void TCreature::ToDoChangeState(int /*NewState*/) { abort(); /* STUB */ }

// 0x080c2590  cract.c:961
void TCreature::ToDoGo(int /*DestX*/, int /*DestY*/, int /*DestZ*/, bool /*Dest*/, int /*MaxSteps*/) { abort(); /* STUB */ }

// 0x080c2880  cract.c:993
void TCreature::ToDoRotate(int /*Direction*/) { abort(); /* STUB */ }

// 0x080c2970  cract.c:1006
void TCreature::ToDoMove(int /*sx*/, int /*sy*/, int /*sz*/, ObjectType /*Type*/, unsigned char /*RNum*/, int /*zx*/, int /*zy*/, int /*zz*/, unsigned char /*Count*/) { abort(); /* STUB */ }

// 0x080c2cb0  cract.c:1044
void TCreature::ToDoMove(Object /*Obj*/, int /*zx*/, int /*zy*/, int /*zz*/, unsigned char /*Count*/) { abort(); /* STUB */ }

// 0x080c2e80  cract.c:1068
void TCreature::ToDoTrade(int /*x*/, int /*y*/, int /*z*/, ObjectType /*Type*/, unsigned char /*RNum*/, uint32_t /*TradePartner*/) { abort(); /* STUB */ }

// 0x080c31a0  cract.c:1107
void TCreature::ToDoUse(unsigned char /*Count*/, int /*x1*/, int /*y1*/, int /*z1*/, ObjectType /*Type1*/, unsigned char /*RNum1*/, unsigned char /*Dummy*/, int /*x2*/, int /*y2*/, int /*z2*/, ObjectType /*Type2*/, unsigned char /*RNum2*/) { abort(); /* STUB */ }

// 0x080c3490  cract.c:1135
void TCreature::ToDoUse(unsigned char /*Count*/, Object /*Obj1*/, Object /*Obj2*/) { abort(); /* STUB */ }

// 0x080c3660  cract.c:1160
void TCreature::ToDoTurn(int /*x*/, int /*y*/, int /*z*/, ObjectType /*Type*/, unsigned char /*RNum*/) { abort(); /* STUB */ }

// 0x080c3870  cract.c:1181
void TCreature::ToDoAttack() { abort(); /* STUB */ }
