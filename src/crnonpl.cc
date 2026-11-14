#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>

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
#include "crmain.hh"
#include "crplayer.hh"
#include "connections.hh"
#include "operate.hh"
#include "magic.hh"
#include "strings.hh"
#include "writer.hh"
#include "utils.hh"
#include "time.hh"
#include "communication.hh"
#include "TXTEASymmetricKey.hh"
#include "crnonpl.hh"

struct TNPC;

enum STATE {
	SLEEPING = 0,
	IDLE = 1,
	UNDERATTACK = 2,
	TALKING = 3,
	LEAVING = 4,
	ATTACKING = 5,
	PANIC = 6,
};

enum SITUATION {
	DEFAULT = 0,
	ADDRESS = 1,
	ADDRESSQUEUE = 2,
	BUSY = 3,
	VANISH = 4,
};

struct TNode {
	int Type;
	int Data;
	TNode *Left;
	TNode *Right;
};

struct TCondition {
	// inline  crnonpl.c:56
	TCondition() { Type = 0; Text = 0; }
	bool set(int Type, void *Data);
	void clear();
	int Type;
	uint32_t Text;
	TNode *Expression;
	int Property;
	int Number;
};

struct TAction {
	// inline  crnonpl.c:68
	TAction() { Type = 0; Text = 0; }
	bool set(int Type, void *Data, void *Data2, void *Data3, void *Data4);
	void clear();
	int Type;
	uint32_t Text;
	int Number;
	TNode *Expression;
	TNode *Expression2;
	TNode *Expression3;
};

struct TBehaviour {
	TBehaviour();
	~TBehaviour();
	bool addCondition(int Type, void *Data);
	bool addAction(int Type, void *Data, void *Data2, void *Data3, void *Data4);
	vector<TCondition> Condition;
	vector<TAction> Action;
	int Conditions;
	int Actions;
};

struct TMonsterhome {
	int Race;
	int x;
	int y;
	int z;
	int Radius;
	int MaxMonsters;
	int ActMonsters;
	int RegenerationTime;
	int Timer;
};

struct TBehaviourDatabase {
	TBehaviourDatabase(TReadScriptFile &Script);
	void react(TNPC *npc, char *Text, SITUATION Situation);
private:
	vector<TBehaviour> Behaviour;
	int Behaviours;
	TNode *readTerm(TReadScriptFile &Script);
	TNode *readFactor(TReadScriptFile &Script);
	TNode *readValue(TReadScriptFile &Script);
	int evaluate(TNPC *npc, TNode *Node, int *Number);
};

struct TNonplayer : public TCreature {
	TNonplayer();
	virtual ~TNonplayer();
	void SetInList();
	void DelInList();
protected:
	STATE State;
};

struct TNPC : public TNonplayer {
	TNPC(char *FileName);
	virtual ~TNPC();
	virtual void TalkStimulus(uint32_t Speaker, char *Text);
	virtual void DamageStimulus(uint32_t, int, int);
	virtual void IdleStimulus();
	virtual void CreatureMoveStimulus(uint32_t CreatureID, int Type);
	virtual bool MovePossible(int x, int y, int z, bool, bool);
	void ChangeState(STATE NewState, bool Stimulus);
private:
	// react() and evaluate() read and change the conversation state.
	friend struct TBehaviourDatabase;
	void GiveTo(ObjectType Type, int Amount);
	void GetFrom(ObjectType Type, int Amount);
	void GiveMoney(int Amount);
	void GetMoney(int Amount);
	void Enqueue(uint32_t Interlocutor, char *Text);
	void TurnToInterlocutor();
	uint32_t Interlocutor;
	int Topic;
	int Price;
	int Amount;
	int TypeID;
	uint32_t Data;
	uint32_t LastTalk;
	vector<uint32_t> QueuedPlayers;
	vector<uint32_t> QueuedAddresses;
	int QueueLength;
	TBehaviourDatabase *Behaviour;
};

struct TMonster : public TNonplayer {
	TMonster(int Race, int x, int y, int z, int Home, uint32_t Father);
	virtual ~TMonster();
	virtual void DamageStimulus(uint32_t Attacker, int Damage, int);
	virtual void IdleStimulus();
	virtual void CreatureMoveStimulus(uint32_t CreatureID, int Type);
	virtual bool MovePossible(int x, int y, int z, bool Execute, bool);
	virtual bool IsPeaceful();
	virtual uint32_t GetMaster() { return Master; }
	void Convince(TCreature *newMaster);
	void SetTarget(TCreature *NewTarget);
private:
	bool CanKickBoxes();
	void KickBoxes(Object Obj);
	bool KickCreature(TCreature *Obstacle);
	int Home;
	uint32_t Master;
	uint32_t Target;
};

vector<TNonplayer *> NonplayerList(0, 10000, 1000, nullptr);	// crnonpl.c:174
int FirstFreeNonplayer;	// crnonpl.c:175
int CurrentNonplayer;	// crnonpl.c:176
vector<TMonsterhome> Monsterhome(1, 5000, 1000);	// crnonpl.c:177
int Monsterhomes;	// crnonpl.c:178
store<TNode, 256> *NodeTable;	// crnonpl.c:179

// Data stays as the store hands it out.
TNode *NewNode(int op, TNode *left, TNode *right) {
	TNode *Node = NodeTable->getFreeItem();
	Node->Type = op;
	Node->Left = left;
	Node->Right = right;
	return Node;
}

TNode *NewNode(int type, int val) {
	TNode *Node = NodeTable->getFreeItem();
	Node->Type = type;
	Node->Data = val;
	Node->Left = nullptr;
	Node->Right = nullptr;
	return Node;
}

void DeleteNode(TNode *Node) {
	if (Node != nullptr) {
		DeleteNode(Node->Left);
		DeleteNode(Node->Right);
		NodeTable->deleteItem(Node);
	}
}

bool TCondition::set(int Type, void *Data) {
	if (Type != 5 && Data == nullptr) {
		error("TCondition::set: Data ist NULL.\n");
		this->Type = 0;
		return false;
	}

	this->Type = Type;
	switch (Type) {
		case 1:
			this->Text = AddDynamicString((char *)Data);
			break;
		case 2:
			this->Property = 0;
			if (strcmp((char *)Data, "address") == 0) {
				this->Property = 1;
			}
			if (strcmp((char *)Data, "busy") == 0) {
				this->Property = 2;
			}
			if (strcmp((char *)Data, "vanish") == 0) {
				this->Property = 3;
			}
			if (strcmp((char *)Data, "male") == 0) {
				this->Property = 4;
			}
			if (strcmp((char *)Data, "female") == 0) {
				this->Property = 5;
			}
			if (strcmp((char *)Data, "knight") == 0) {
				this->Property = 6;
			}
			if (strcmp((char *)Data, "paladin") == 0) {
				this->Property = 7;
			}
			if (strcmp((char *)Data, "sorcerer") == 0) {
				this->Property = 8;
			}
			if (strcmp((char *)Data, "druid") == 0) {
				this->Property = 9;
			}
			if (strcmp((char *)Data, "premium") == 0) {
				this->Property = 10;
			}
			if (strcmp((char *)Data, "promoted") == 0) {
				this->Property = 11;
			}
			if (strcmp((char *)Data, "pzblock") == 0) {
				this->Property = 12;
			}
			if (strcmp((char *)Data, "nonpvp") == 0) {
				this->Property = 13;
			}
			if (strcmp((char *)Data, "pvpenforced") == 0) {
				this->Property = 14;
			}
			if (this->Property == 0) {
				return false;
			}
			break;
		case 3:
			this->Number = *(int *)Data;
			break;
		case 4:
			this->Expression = (TNode *)Data;
			break;
		case 5:
			break;
		default:
			error("TCondition::set: Ung\xfcltiger Bedingungstyp %d\n", Type);
			return false;
	}

	return true;
}

void TCondition::clear() {
	if (this->Type == 4) {
		DeleteNode(this->Expression);
	}
	if (this->Type == 1) {
		DeleteDynamicString(this->Text);
	}
	this->Type = 0;
}

bool TAction::set(int Type, void *Data, void *Data2, void *Data3, void *Data4) {
	this->Type = Type;
	switch (Type) {
		case 1:
			this->Text = AddDynamicString((char *)Data);
			break;
		case 2:
		case 3:
		case 5:
			this->Number = *(int *)Data;
			this->Expression = (TNode *)Data2;
			break;
		case 4:
		case 8:
			this->Number = *(int *)Data;
			this->Expression = (TNode *)Data2;
			this->Expression2 = (TNode *)Data3;
			break;
		case 6:
		case 7:
			this->Number = *(int *)Data;
			break;
		case 9:
			break;
		case 10:
			this->Number = *(int *)Data;
			this->Expression = (TNode *)Data2;
			this->Expression2 = (TNode *)Data3;
			this->Expression3 = (TNode *)Data4;
			break;
		default:
			error("TAction::set: Ung\xfcltiger Aktionstyp %d\n", Type);
			return false;
	}

	return true;
}

void TAction::clear() {
	if (this->Type == 2 || this->Type == 3 || this->Type == 4 || this->Type == 5
			|| this->Type == 8 || this->Type == 10) {
		DeleteNode(this->Expression);
	}
	if (this->Type == 4 || this->Type == 8 || this->Type == 10) {
		DeleteNode(this->Expression2);
	}
	if (this->Type == 10) {
		DeleteNode(this->Expression3);
	}
	if (this->Type == 1) {
		DeleteDynamicString(this->Text);
	}
	this->Type = 0;
}

TBehaviour::TBehaviour() : Condition(0, 5, 5), Action(0, 5, 5) {
	this->Conditions = 0;
	this->Actions = 0;
}

TBehaviour::~TBehaviour() {
	for (int i = 0; i < this->Conditions; i++) {
		this->Condition(i).clear();
	}
	for (int i = 0; i < this->Actions; i++) {
		this->Action(i).clear();
	}
}

bool TBehaviour::addCondition(int Type, void *Data) {
	bool Result = this->Condition(this->Conditions).set(Type, Data);
	if (Result) {
		this->Conditions++;
	}
	return Result;
}

bool TBehaviour::addAction(int Type, void *Data, void *Data2, void *Data3, void *Data4) {
	bool Result = this->Action(this->Actions).set(Type, Data, Data2, Data3, Data4);
	if (Result) {
		this->Actions++;
	}
	return Result;
}

TBehaviourDatabase::TBehaviourDatabase(TReadScriptFile &Script) : Behaviour(0, 50, 25) {
	this->Behaviours = 0;
	Script.readSymbol('{');
	Script.nextToken();
	while (true) {
		if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
			break;
		}

		if (Script.Token != SPECIAL || Script.getSpecial() != 'I') {
			while (true) {
				bool Success = false;
				if (Script.Token == STRING) {
					Success = this->Behaviour(this->Behaviours).addCondition(1, Script.getString());
				}
				if (Script.Token == IDENTIFIER) {
					Success = this->Behaviour(this->Behaviours).addCondition(2, Script.getIdentifier());
				}
				if (Script.Token == SPECIAL && Script.getSpecial() == '!') {
					Success = this->Behaviour(this->Behaviours).addCondition(5, nullptr);
				}
				if (Script.Token == SPECIAL && Script.getSpecial() == '%') {
					int Nr = Script.readNumber();
					if (Nr < 1 || Nr > 3) {
						Script.error("illegal ordinal number");
					}
					Success = this->Behaviour(this->Behaviours).addCondition(3, &Nr);
				}

				if (!Success) {
					TNode *Term1 = this->readTerm(Script);
					if (Script.Token != SPECIAL) {
						Script.error("relational operator expected");
					}
					int Operator = 0;
					switch (Script.getSpecial()) {
						case '<': Operator = 1; break;
						case '>': Operator = 2; break;
						case '=': Operator = 3; break;
						case 'N': Operator = 4; break;
						case 'L': Operator = 5; break;
						case 'G': Operator = 6; break;
						default:
							Script.error("relational operator expected");
							break;
					}
					Script.nextToken();
					TNode *Term2 = this->readTerm(Script);
					TNode *Comparison = NewNode(Operator, Term1, Term2);
					this->Behaviour(this->Behaviours).addCondition(4, Comparison);
				} else {
					Script.nextToken();
				}

				if (Script.Token != SPECIAL || Script.getSpecial() != ',') {
					break;
				}
				Script.nextToken();
			}

			if (Script.Token != SPECIAL || Script.getSpecial() != 'I') {
				Script.error("'->' expected");
			}
		}

		while (true) {
			Script.nextToken();
			if (Script.Token == STRING) {
				this->Behaviour(this->Behaviours).addAction(1, Script.getString(), nullptr, nullptr, nullptr);
				Script.nextToken();
			} else if (Script.Token == IDENTIFIER) {
				char *Ident = Script.getIdentifier();
				int Type = -1;
				int Data = 0;
				if (strcmp(Ident, "topic") == 0) {
					Type = 2;
					Data = 1;
				}
				if (strcmp(Ident, "price") == 0) {
					Type = 2;
					Data = 2;
				}
				if (strcmp(Ident, "amount") == 0) {
					Type = 2;
					Data = 3;
				}
				if (strcmp(Ident, "type") == 0) {
					Type = 2;
					Data = 4;
				}
				if (strcmp(Ident, "data") == 0) {
					Type = 2;
					Data = 6;
				}
				if (strcmp(Ident, "hp") == 0) {
					Type = 3;
					Data = 2;
				}
				if (strcmp(Ident, "burning") == 0) {
					Type = 8;
					Data = 18;
				}
				if (strcmp(Ident, "poison") == 0) {
					Type = 8;
					Data = 17;
				}
				if (strcmp(Ident, "setquestvalue") == 0) {
					Type = 4;
					Data = 3;
				}
				if (strcmp(Ident, "effectme") == 0) {
					Type = 5;
					Data = 1;
				}
				if (strcmp(Ident, "effectopp") == 0) {
					Type = 5;
					Data = 2;
				}
				if (strcmp(Ident, "profession") == 0) {
					Type = 5;
					Data = 3;
				}
				if (strcmp(Ident, "teachspell") == 0) {
					Type = 5;
					Data = 4;
				}
				if (strcmp(Ident, "summon") == 0) {
					Type = 5;
					Data = 5;
				}
				if (strcmp(Ident, "create") == 0) {
					Type = 5;
					Data = 6;
				}
				if (strcmp(Ident, "delete") == 0) {
					Type = 5;
					Data = 7;
				}
				if (strcmp(Ident, "createmoney") == 0) {
					Type = 6;
					Data = 1;
				}
				if (strcmp(Ident, "deletemoney") == 0) {
					Type = 6;
					Data = 2;
				}
				if (strcmp(Ident, "queue") == 0) {
					Type = 6;
					Data = 3;
				}
				if (strcmp(Ident, "teleport") == 0) {
					Type = 10;
					Data = 1;
				}
				if (strcmp(Ident, "startposition") == 0) {
					Type = 10;
					Data = 2;
				}
				if (strcmp(Ident, "idle") == 0) {
					Type = 7;
					Data = 1;
				}
				if (strcmp(Ident, "nop") == 0) {
					Type = 0;
				}

				switch (Type) {
					case 0:
						Script.nextToken();
						break;
					case 2:
					case 3:
						Script.readSymbol('=');
						Script.nextToken();
						this->Behaviour(this->Behaviours).addAction(Type, &Data, this->readTerm(Script), nullptr, nullptr);
						break;
					case 4:
					case 8: {
						Script.readSymbol('(');
						Script.nextToken();
						TNode *Param1 = this->readTerm(Script);
						if (Script.Token != SPECIAL || Script.getSpecial() != ',') {
							Script.error(", expected");
						}
						Script.nextToken();
						TNode *Param2 = this->readTerm(Script);
						if (Script.Token != SPECIAL || Script.getSpecial() != ')') {
							Script.error(") expected");
						}
						this->Behaviour(this->Behaviours).addAction(Type, &Data, Param1, Param2, nullptr);
						Script.nextToken();
						break;
					}
					case 5:
						Script.readSymbol('(');
						Script.nextToken();
						this->Behaviour(this->Behaviours).addAction(Type, &Data, this->readTerm(Script), nullptr, nullptr);
						if (Script.Token != SPECIAL || Script.getSpecial() != ')') {
							Script.error(") expected");
						}
						Script.nextToken();
						break;
					case 6:
					case 7:
						this->Behaviour(this->Behaviours).addAction(Type, &Data, nullptr, nullptr, nullptr);
						Script.nextToken();
						break;
					case 10: {
						Script.readSymbol('(');
						Script.nextToken();
						TNode *Param1 = this->readTerm(Script);
						if (Script.Token != SPECIAL || Script.getSpecial() != ',') {
							Script.error(", expected");
						}
						Script.nextToken();
						TNode *Param2 = this->readTerm(Script);
						if (Script.Token != SPECIAL || Script.getSpecial() != ',') {
							Script.error(", expected");
						}
						Script.nextToken();
						TNode *Param3 = this->readTerm(Script);
						if (Script.Token != SPECIAL || Script.getSpecial() != ')') {
							Script.error(") expected");
						}
						this->Behaviour(this->Behaviours).addAction(Type, &Data, Param1, Param2, Param3);
						Script.nextToken();
						break;
					}
					default:
						Script.error("unknown identifier");
						break;
				}
			} else if (Script.Token == SPECIAL && Script.getSpecial() == '*') {
				if (this->Behaviours == 0) {
					Script.error("no previous pattern");
				}
				this->Behaviour(this->Behaviours).addAction(9, nullptr, nullptr, nullptr, nullptr);
				Script.nextToken();
			} else {
				Script.error("illegal action");
			}

			if (Script.Token != SPECIAL || Script.getSpecial() != ',') {
				break;
			}
		}

		this->Behaviours++;
	}
}

TNode *TBehaviourDatabase::readValue(TReadScriptFile &Script) {
	TNode *First;
	TNode *Second;
	TNode *Node = nullptr;
	if (Script.Token == NUMBER) {
		Node = NewNode(11, Script.getNumber());
	} else if (Script.Token == IDENTIFIER) {
		if (strcmp(Script.getIdentifier(), "topic") == 0) {
			Node = NewNode(12, 0);
		}
		if (strcmp(Script.getIdentifier(), "price") == 0) {
			Node = NewNode(13, 0);
		}
		if (strcmp(Script.getIdentifier(), "amount") == 0) {
			Node = NewNode(14, 0);
		}
		if (strcmp(Script.getIdentifier(), "level") == 0) {
			Node = NewNode(15, 0);
		}
		if (strcmp(Script.getIdentifier(), "maglevel") == 0) {
			Node = NewNode(15, 1);
		}
		if (strcmp(Script.getIdentifier(), "hp") == 0) {
			Node = NewNode(15, 2);
		}
		if (strcmp(Script.getIdentifier(), "burning") == 0) {
			Node = NewNode(15, 18);
		}
		if (strcmp(Script.getIdentifier(), "poison") == 0) {
			Node = NewNode(15, 17);
		}
		if (strcmp(Script.getIdentifier(), "count") == 0) {
			Script.readSymbol('(');
			Script.nextToken();
			First = this->readTerm(Script);
			Node = NewNode(16, First, nullptr);
			if (Script.Token != SPECIAL || Script.getSpecial() != ')') {
				Script.error(") expected");
			}
		} else {
			if (strcmp(Script.getIdentifier(), "countmoney") == 0) {
				Node = NewNode(17, 0);
			}
			if (strcmp(Script.getIdentifier(), "type") == 0) {
				Node = NewNode(18, 0);
			}
			if (strcmp(Script.getIdentifier(), "data") == 0) {
				Node = NewNode(19, 0);
			}
			if (strcmp(Script.getIdentifier(), "spellknown") == 0) {
				Script.readSymbol('(');
				Script.nextToken();
				First = this->readTerm(Script);
				Node = NewNode(20, First, nullptr);
				if (Script.Token != SPECIAL || Script.getSpecial() != ')') {
					Script.error(") expected");
				}
			} else if (strcmp(Script.getIdentifier(), "spelllevel") == 0) {
				Script.readSymbol('(');
				Script.nextToken();
				First = this->readTerm(Script);
				Node = NewNode(21, First, nullptr);
				if (Script.Token != SPECIAL || Script.getSpecial() != ')') {
					Script.error(") expected");
				}
			} else if (strcmp(Script.getIdentifier(), "random") == 0) {
				Script.readSymbol('(');
				Script.nextToken();
				First = this->readTerm(Script);
				if (Script.Token != SPECIAL || Script.getSpecial() != ',') {
					Script.error(", expected");
				}
				Script.nextToken();
				Second = this->readTerm(Script);
				if (Script.Token != SPECIAL || Script.getSpecial() != ')') {
					Script.error(") expected");
				}
				Node = NewNode(22, First, Second);
			} else if (strcmp(Script.getIdentifier(), "questvalue") == 0) {
				Script.readSymbol('(');
				Script.nextToken();
				First = this->readTerm(Script);
				Node = NewNode(23, First, nullptr);
				if (Script.Token != SPECIAL || Script.getSpecial() != ')') {
					Script.error(") expected");
				}
			}
		}
	} else if (Script.Token == SPECIAL) {
		if (Script.getSpecial() != '%') {
			Script.error("illegal character");
		}
		Node = NewNode(10, Script.readNumber());
	} else {
		Script.error("illegal value");
	}

	if (Node == nullptr) {
		Script.error("unknown value");
	}

	Script.nextToken();
	return Node;
}

TNode *TBehaviourDatabase::readFactor(TReadScriptFile &Script) {
	TNode *Left = this->readValue(Script);
	while (Script.Token == SPECIAL && Script.getSpecial() == '*') {
		Script.nextToken();
		TNode *Right = this->readValue(Script);
		Left = NewNode(9, Left, Right);
	}
	return Left;
}

TNode *TBehaviourDatabase::readTerm(TReadScriptFile &Script) {
	TNode *Left = this->readFactor(Script);
	while (Script.Token == SPECIAL && (Script.getSpecial() == '+' || Script.getSpecial() == '-')) {
		int Operator = (Script.getSpecial() == '+' ? 7 : 8);
		Script.nextToken();
		TNode *Right = this->readFactor(Script);
		Left = NewNode(Operator, Left, Right);
	}
	return Left;
}

int TBehaviourDatabase::evaluate(TNPC *npc, TNode *Node, int *Number) {
	if (Node == nullptr) {
		error("TBehaviourDatabase::evaluate: Knoten existiert nicht.\n");
		return 0;
	}

	if (Number == nullptr) {
		error("TBehaviourDatabase::evaluate: Zahlen existieren nicht.\n");
		return 0;
	}

	if (npc == nullptr) {
		error("TBehaviourDatabase::evaluate: NPC existiert nicht.\n");
		return 0;
	}

	TPlayer *Opp = GetPlayer(npc->Interlocutor);
	if (Opp == nullptr) {
		error("TBehaviourDatabase::evaluate: Gespr\xe4" "chspartner existiert nicht.\n");
		return 0;
	}

	// Both operands are evaluated into locals: the order of the evaluations
	// (and of the random() calls inside them) is part of the behavior.
	switch (Node->Type) {
		case 1: {
			int Left = this->evaluate(npc, Node->Left, Number);
			int Right = this->evaluate(npc, Node->Right, Number);
			return Left < Right;
		}
		case 2: {
			int Left = this->evaluate(npc, Node->Left, Number);
			int Right = this->evaluate(npc, Node->Right, Number);
			return Left > Right;
		}
		case 3: {
			int Left = this->evaluate(npc, Node->Left, Number);
			int Right = this->evaluate(npc, Node->Right, Number);
			return Left == Right;
		}
		case 4: {
			int Left = this->evaluate(npc, Node->Left, Number);
			int Right = this->evaluate(npc, Node->Right, Number);
			return Left != Right;
		}
		case 5: {
			int Left = this->evaluate(npc, Node->Left, Number);
			int Right = this->evaluate(npc, Node->Right, Number);
			return Left <= Right;
		}
		case 6: {
			int Left = this->evaluate(npc, Node->Left, Number);
			int Right = this->evaluate(npc, Node->Right, Number);
			return Left >= Right;
		}
		case 7: {
			int Left = this->evaluate(npc, Node->Left, Number);
			int Right = this->evaluate(npc, Node->Right, Number);
			return Left + Right;
		}
		case 8: {
			int Left = this->evaluate(npc, Node->Left, Number);
			int Right = this->evaluate(npc, Node->Right, Number);
			return Left - Right;
		}
		case 9: {
			int Left = this->evaluate(npc, Node->Left, Number);
			int Right = this->evaluate(npc, Node->Right, Number);
			return Left * Right;
		}
		case 10:
			if (Node->Data < 1 || Node->Data > 3) {
				error("TBehaviourDatabase::evaluate: Ung\xfcltiger Zahl-Parameter %d.\n", Node->Data);
				return 0;
			}
			if (Number[Node->Data - 1] < 0) {
				error("TBehaviourDatabase::evaluate: Zahl-Parameter %d nicht belegt.\n", Node->Data);
				return 0;
			}
			return Number[Node->Data - 1];
		case 11:
			return Node->Data;
		case 12:
			return npc->Topic;
		case 13:
			return npc->Price;
		case 14:
			return npc->Amount;
		case 15:
			if (Node->Data >= 0 && Node->Data <= 2) {
				return Opp->Skills[Node->Data]->Get();
			}
			if (Node->Data == 17 || Node->Data == 18) {
				return Opp->Skills[Node->Data]->TimerValue();
			}
			error("TBehaviourDatabase::evaluate: Ung\xfcltiger Skill %d.\n", Node->Data);
			return 0;
		case 16:
			return CountInventoryObjects(npc->Interlocutor, this->evaluate(npc, Node->Left, Number), npc->Data);
		case 17:
			return CountInventoryMoney(npc->Interlocutor);
		case 18:
			return npc->TypeID;
		case 19:
			return (int)npc->Data;
		case 20:
			return Opp->SpellKnown(this->evaluate(npc, Node->Left, Number));
		case 21:
			return GetSpellLevel(this->evaluate(npc, Node->Left, Number));
		case 22: {
			int Max = this->evaluate(npc, Node->Right, Number);
			int Min = this->evaluate(npc, Node->Left, Number);
			return random(Min, Max);
		}
		case 23:
			return Opp->GetQuestValue(this->evaluate(npc, Node->Left, Number));
		default:
			error("TBehaviourDatabase::evaluate: Ung\xfcltiger Knotentyp %d.\n", Node->Type);
			return 0;
	}
}

void TBehaviourDatabase::react(TNPC *npc, char *Text, SITUATION Situation) {
	if (npc == nullptr) {
		error("TBehaviourDatabase::react: NPC existiert nicht.\n");
		return;
	}

	if (Text == nullptr) {
		error("TBehaviourDatabase::react: \xdc" "bergebener Text existiert nicht.\n");
		return;
	}

	TPlayer *Opp = GetPlayer(npc->Interlocutor);
	if (Opp == nullptr) {
		error("TBehaviourDatabase::react: Gespr\xe4" "chspartner existiert nicht (Text=%s, Situation=%d).\n",
				Text, Situation);
		return;
	}

	int maxConditions = -1;
	int bestPattern = -1;
	int Numbers[3];
	Numbers[0] = -1;
	Numbers[1] = -1;
	Numbers[2] = -1;
	for (int i = 0; i < this->Behaviours; i++) {
		TBehaviour &B = this->Behaviour(i);
		bool fulfilled = true;
		char *LastPos = Text;
		for (int j = 0; j < B.Conditions && fulfilled; j++) {
			TCondition &C = B.Condition(j);
			char *Pos;
			switch (C.Type) {
				case 1:
					Pos = SearchForWord(GetDynamicString(C.Text), LastPos);
					if (Pos == nullptr) {
						fulfilled = false;
					} else {
						LastPos = Pos + strlen(GetDynamicString(C.Text));
					}
					break;
				case 2:
					if (C.Property == 1 && Situation != ADDRESS && Situation != ADDRESSQUEUE) {
						fulfilled = false;
					}
					if (C.Property == 2 && Situation != BUSY) {
						fulfilled = false;
					}
					if (C.Property == 3 && Situation != VANISH) {
						fulfilled = false;
					}
					if (C.Property == 4 && Opp->Sex != 1) {
						fulfilled = false;
					}
					if (C.Property == 5 && Opp->Sex != 2) {
						fulfilled = false;
					}
					if (C.Property == 6 && Opp->GetEffectiveProfession() != 1) {
						fulfilled = false;
					}
					if (C.Property == 7 && Opp->GetEffectiveProfession() != 2) {
						fulfilled = false;
					}
					if (C.Property == 8 && Opp->GetEffectiveProfession() != 3) {
						fulfilled = false;
					}
					if (C.Property == 9 && Opp->GetEffectiveProfession() != 4) {
						fulfilled = false;
					}
					if (C.Property == 10 && !CheckRight(Opp->ID, PREMIUM_ACCOUNT)) {
						fulfilled = false;
					}
					if (C.Property == 11 && Opp->GetRealProfession() < 10) {
						fulfilled = false;
					}
					if (C.Property == 12 && Opp->EarliestProtectionZoneRound <= RoundNr) {
						fulfilled = false;
					}
					if (C.Property == 13 && WorldType != NON_PVP) {
						fulfilled = false;
					}
					if (C.Property == 14 && WorldType != PVP_ENFORCED) {
						fulfilled = false;
					}
					break;
				case 3:
					Pos = SearchForNumber(C.Number, LastPos);
					if (Pos == nullptr) {
						fulfilled = false;
					} else {
						// The original strtol() saturated at the 32-bit LONG_MAX.
						Numbers[C.Number - 1] = (int)std::min(strtol(Pos, nullptr, 10), (long)INT32_MAX);
						if (Numbers[C.Number - 1] > 500) {
							Numbers[C.Number - 1] = 500;
						}
						LastPos = Pos + 1;
					}
					break;
				case 4:
					if (this->evaluate(npc, C.Expression, Numbers) == 0) {
						fulfilled = false;
					}
					break;
				case 5:
					bestPattern = i;
					goto Forced;
			}
		}

		if (fulfilled && B.Conditions > maxConditions) {
			maxConditions = B.Conditions;
			bestPattern = i;
		}
	}

	if (maxConditions == -1) {
		return;
	}

Forced:
	if (Situation != BUSY) {
		npc->Topic = 0;
	}

	bool StartToDo = false;
	int TalkDelay = 1000;

PreviousPattern:
	TBehaviour &B = this->Behaviour(bestPattern);
	for (int j = 0; j < B.Actions; j++) {
		TAction &A = B.Action(j);
		char *AText;
		char Response[300];
		int Value;
		int Value2;
		int Value3;
		switch (A.Type) {
			case 1:
				memset(Response, 0, sizeof(Response));
				AText = GetDynamicString(A.Text);
				for (unsigned int pos = 0; pos < strlen(AText); pos++) {
					if (AText[pos] == '%') {
						char Help[50];
						int Hour;
						int Minute;
						pos++;
						switch (AText[pos]) {
							case 'N':
								strcpy(Help, Opp->Name);
								break;
							case 'A':
								sprintf(Help, "%d", npc->Amount);
								break;
							case 'P':
								sprintf(Help, "%d", npc->Price);
								break;
							case 'T':
								GetTime(Hour, Minute);
								sprintf(Help, (Hour < 12 ? "%d:%.2d am" : "%d:%.2d pm"), (Hour + 11) % 12 + 1, Minute);
								break;
							default:
								Help[0] = 0;
								break;
						}
						strcat(Response, Help);
					} else {
						Response[strlen(Response)] = AText[pos];
					}

					if (Response[256] != 0) {
						break;
					}
				}

				if (Response[256] != 0) {
					Response[20] = 0;
					error("TBehaviourDatabase::react: Text von NPC %s wird zu lang (%s...).\n", npc->Name, Response);
				} else {
					npc->ToDoWait(TalkDelay);
					npc->ToDoTalk(1, nullptr, Response, false);
					TalkDelay += 3100 + (int)(strlen(Response) / 2) * 100;
					StartToDo = true;
				}
				break;
			case 2:
				Value = this->evaluate(npc, A.Expression, Numbers);
				switch (A.Number) {
					case 1:
						npc->Topic = Value;
						break;
					case 2:
						npc->Price = Value;
						break;
					case 3:
						npc->Amount = Value;
						break;
					case 4:
						npc->TypeID = Value;
						break;
					case 6:
						npc->Data = (uint32_t)Value;
						break;
					default:
						error("TBehaviourDatabase::react: Ung\xfcltige Variable.\n");
						break;
				}
				break;
			case 3:
				Value = this->evaluate(npc, A.Expression, Numbers);
				if (A.Number != 2) {
					error("TBehaviourDatabase::react: Ung\xfcltiger Skill.\n");
					break;
				}
				Opp->Skills[2]->Set(Value);
				if (Opp->Skills[2]->Get() < 1) {
					error("TBehaviourDatabase::react: NPC %s t\xf6tet Spieler.\n", npc->Name);
					Opp->Death();
				}
				break;
			case 4:
				Value = this->evaluate(npc, A.Expression, Numbers);
				Value2 = this->evaluate(npc, A.Expression2, Numbers);
				if (A.Number != 3) {
					error("TBehaviourDatabase::react (4): Ung\xfcltige Funktionsnummer.\n");
					break;
				}
				Opp->SetQuestValue(Value, Value2);
				break;
			case 5:
				Value = this->evaluate(npc, A.Expression, Numbers);
				switch (A.Number) {
					case 1:
						GraphicalEffect(npc->GetCreatureObject(), Value);
						break;
					case 2:
						GraphicalEffect(Opp->GetCreatureObject(), Value);
						break;
					case 3:
						Opp->SetProfession((unsigned char)Value);
						break;
					case 4:
						Opp->LearnSpell(Value);
						break;
					case 5:
						CreateMonster(Value, npc->posx, npc->posy, npc->posz, 0, 0, true);
						break;
					case 6:
						npc->GiveTo(Value, npc->Amount);
						break;
					case 7:
						npc->GetFrom(Value, npc->Amount);
						break;
					default:
						error("TBehaviourDatabase::react (5): Ung\xfcltige Funktionsnummer.\n");
						break;
				}
				break;
			case 6:
				switch (A.Number) {
					case 1:
						npc->GiveMoney(npc->Price);
						break;
					case 2:
						npc->GetMoney(npc->Price);
						break;
					case 3:
						if (Situation != BUSY) {
							error("TBehaviourDatabase::react (6): falsche Situation f\xfcr Aktion \"Queue\".\n");
						}
						npc->Enqueue(npc->Interlocutor, Text);
						break;
					default:
						error("TBehaviourDatabase::react (6): Ung\xfcltige Funktionsnummer.\n");
						break;
				}
				break;
			case 7:
				if (!StartToDo) {
					npc->State = (STATE)A.Number;
					if (Situation == ADDRESSQUEUE) {
						error("TBehaviourDatabase::react: NPC %s reagiert nicht auf Anrede %s.\n", npc->Name, Text);
					} else {
						StartToDo = true;
					}
				} else {
					if (A.Number == IDLE) {
						npc->State = LEAVING;
					}
					npc->ToDoChangeState(A.Number);
				}
				break;
			case 8:
				Value = this->evaluate(npc, A.Expression, Numbers);
				Value2 = this->evaluate(npc, A.Expression2, Numbers);
				if (A.Number != 17 && A.Number != 18) {
					error("TBehaviourDatabase::react (8): Ung\xfcltiger Skill.\n");
					break;
				}
				if (Value == 0 || Opp->Skills[A.Number]->TimerValue() < Value) {
					Opp->SetTimer((unsigned short)A.Number, Value, Value2, Value2, -1);
					if (A.Number == 18) {
						Opp->FireDamageOrigin = 0;
					} else {
						Opp->PoisonDamageOrigin = 0;
					}
				}
				break;
			case 9:
				if (bestPattern == 0) {
					error("TBehaviourDatabase::react (9): Kein vorheriges Muster.\n");
					break;
				}
				bestPattern--;
				goto PreviousPattern;
			case 10:
				Value = this->evaluate(npc, A.Expression, Numbers);
				Value2 = this->evaluate(npc, A.Expression2, Numbers);
				Value3 = this->evaluate(npc, A.Expression3, Numbers);
				if (A.Number == 1) {
					print(3, "NPC teleportiert Gespr\xe4" "chspartner nach [%d,%d,%d].\n", Value, Value2, Value3);
					try {
						Move(0, Opp->GetCreatureObject(), GetMapContainer(Value, Value2, Value3), -1, false, NONE);
					} catch (RESULT r) {
						error("TBehaviourDatabase::react (10): Exception %d.\n", r);
					}
				} else if (A.Number == 2) {
					print(3, "NPC setzt Startkoordinate f\xfcr Gespr\xe4" "chspartner auf [%d,%d,%d].\n", Value, Value2, Value3);
					if (Opp->Type == PLAYER) {
						Opp->startx = Value;
						Opp->starty = Value2;
						Opp->startz = Value3;
						Opp->SaveData();
					} else {
						error("TBehaviourDatabase::react (10): Gespr\xe4" "chspartner ist kein Spieler.\n");
					}
				} else {
					error("TBehaviourDatabase::react (10): Ung\xfcltige Unternummer.\n");
				}
				break;
		}
	}

	if (!StartToDo) {
		return;
	}

	npc->ToDoWait(TalkDelay);
	npc->ToDoStart();
	if (Situation != BUSY) {
		npc->LastTalk = RoundNr + TalkDelay / 1000;
	}
}

void StartMonsterhomeTimer(int nr) {
	if (nr < 1 || nr > Monsterhomes) {
		error("StartMonsterhomeTimer: Ung\xfcltige Monsterhome-Nummer %d.\n", nr);
		return;
	}

	TMonsterhome &MH = Monsterhome(nr);
	if (MH.Timer > 0) {
		error("StartMonsterhomeTimer: Z\xe4hler l\xe4uft schon.\n");
		return;
	}

	if (MH.ActMonsters >= MH.MaxMonsters) {
		error("StartMonsterhomeTimer: Maximale Monsterzahl schon erreicht.\n");
		error("# Monsterhome mit Rasse %d an [%d,%d,%d]\n", MH.Race, MH.x, MH.y, MH.z);
		return;
	}

	int PlayerAdjustedTime = MH.RegenerationTime;
	int NumberOfPlayers = GetNumberOfPlayers();
	if (NumberOfPlayers > 800) {
		PlayerAdjustedTime = PlayerAdjustedTime * 2 / 5;
	} else if (NumberOfPlayers > 200) {
		PlayerAdjustedTime = PlayerAdjustedTime * 200 / (NumberOfPlayers / 2 + 100);
	}
	MH.Timer = random(PlayerAdjustedTime / 2, PlayerAdjustedTime);
}

void LoadMonsterhomes() {
	char FileName[4096];
	print(1, "Initialisiere Monsterhomes ...\n");
	sprintf(FileName, "%s/monster.db", DATAPATH);

	TReadScriptFile Script;
	Script.open(FileName);
	Monsterhomes = 0;
	while (true) {
		int Race = Script.readNumber();
		if (Race == 0) {
			break;
		}

		Monsterhomes++;
		TMonsterhome &MH = Monsterhome(Monsterhomes);
		MH.Race = Race;
		MH.x = Script.readNumber();
		MH.y = Script.readNumber();
		MH.z = Script.readNumber();
		if (!IsOnMap(MH.x, MH.y, MH.z)) {
			print(1, "WARNUNG: Monsterhome [%d,%d,%d] befindet sich au\xdf" "erhalb der Karte.\n", MH.x, MH.y, MH.z);
		}
		MH.Radius = Script.readNumber();
		MH.MaxMonsters = Script.readNumber();
		MH.ActMonsters = 0;
		MH.RegenerationTime = Script.readNumber();
		MH.Timer = 0;
	}

	print(1, "%d Monsterhomes geladen.\n", Monsterhomes);
	Script.close();

	for (int i = 1; i <= Monsterhomes; i++) {
		TMonsterhome &MH = Monsterhome(i);
		for (int j = 0; j < MH.MaxMonsters; j++) {
			int x = MH.x;
			int y = MH.y;
			int z = MH.z;
			int r;
			if (j == 0) {
				r = std::min(1, MH.Radius);
			} else {
				r = -std::min(10, MH.Radius);
			}
			if (SearchSpawnField(x, y, z, r, false)) {
				CreateMonster(MH.Race, x, y, z, i, 0, false);
				MH.ActMonsters++;
			}
		}

		if (MH.Timer > 0) {
			error("LoadMonsterhomes: Timer l\xe4uft schon (Rasse %d an [%d,%d,%d]).\n", MH.Race, MH.x, MH.y, MH.z);
		} else if (MH.ActMonsters < MH.MaxMonsters) {
			StartMonsterhomeTimer(i);
		}
	}
}

void ProcessMonsterhomes() {
	for (int i = 1; i <= Monsterhomes; i++) {
		TMonsterhome &MH = Monsterhome(i);
		if (MH.Timer == 0) {
			continue;
		}

		MH.Timer--;
		if (MH.Timer > 0) {
			continue;
		}

		int x = MH.x;
		int y = MH.y;
		int z = MH.z;
		int MaxRadius = std::min(10, MH.Radius);
		TFindCreatures Search(MaxRadius + 9, MaxRadius + 7, x, y, 1);
		while (true) {
			uint32_t ID = Search.getNext();
			if (ID == 0) {
				break;
			}

			TCreature *cr = GetCreature(ID);
			if (cr == nullptr) {
				error("ProcessMonsterhomes: Kreatur existiert nicht.\n");
				break;
			}

			if ((cr->posz <= 7 && z <= 7) || (cr->posz > 7 && abs(z - cr->posz) <= 2)) {
				MaxRadius = std::min(MaxRadius, std::max(abs(cr->posx - x) - 9, abs(cr->posy - y) - 7));
			}
		}

		int r;
		if (MH.ActMonsters == 0) {
			r = std::min(1, MH.Radius);
		} else {
			r = -MaxRadius;
		}

		if (MaxRadius >= 0 && SearchSpawnField(x, y, z, r, false)) {
			CreateMonster(MH.Race, x, y, z, i, 0, false);
			MH.ActMonsters++;
			if (MH.Timer > 0) {
				error("ProcessMonsterhomes: Timer l\xe4uft schon (Rasse %d an [%d,%d,%d]).\n", MH.Race, x, y, z);
				continue;
			}
		}

		if (MH.ActMonsters < MH.MaxMonsters) {
			StartMonsterhomeTimer(i);
		}
	}
}

void NotifyMonsterhomeOfDeath(int nr) {
	if (nr < 1 || nr > Monsterhomes) {
		error("NotifyMonsterhomeOfDeath: Ung\xfcltige Monsterhome-Nummer %d.\n", nr);
		return;
	}

	TMonsterhome &MH = Monsterhome(nr);
	if (MH.ActMonsters <= 0) {
		error("NotifyMonsterhomeOfDeath: Monsterhome hat keine lebenden Kreaturen.\n");
		return;
	}

	MH.ActMonsters--;
	if (MH.ActMonsters >= MH.MaxMonsters) {
		error("NotifyMonsterhomeOfDeath: Monsterhome %d hatte zu viele Monster (%d statt %d).\n",
				nr, MH.ActMonsters + 1, MH.MaxMonsters);
	} else if (MH.Timer == 0) {
		StartMonsterhomeTimer(nr);
	}
}

bool MonsterhomeInRange(int nr, int x, int y, int z) {
	if (nr == 0) {
		return true;
	}

	if (nr < 1 || nr > Monsterhomes) {
		error("MonsterhomeInRange: Ung\xfcltige Monsterhome-Nummer %d.\n", nr);
		return false;
	}

	TMonsterhome &MH = Monsterhome(nr);
	return abs(z - MH.z) <= 2 && abs(x - MH.x) <= MH.Radius && abs(y - MH.y) <= MH.Radius;
}

TNonplayer::TNonplayer() {
	this->State = SLEEPING;
}

TNonplayer::~TNonplayer() {
	this->DelInList();
}

void TNonplayer::SetInList() {
	this->SetInCrList();
	NonplayerList(FirstFreeNonplayer) = this;
	FirstFreeNonplayer++;
}

void TNonplayer::DelInList() {
	for (int i = 0; i < FirstFreeNonplayer; i++) {
		if (NonplayerList(i) == this) {
			NonplayerList(i) = NonplayerList(FirstFreeNonplayer - 1);
			NonplayerList(FirstFreeNonplayer - 1) = nullptr;
			FirstFreeNonplayer--;
			return;
		}
	}

	error("TNonplayer::DelInList: Kreatur nicht gefunden.\n");
}

TNPC::TNPC(char *FileName) : QueuedPlayers(0, 9, 10), QueuedAddresses(0, 9, 10) {
	print(1, "Lade NPC-Daten aus Datei %s...\n", FileName);
	this->Type = NPC;
	this->Sex = 1;
	this->Race = 0;
	this->Outfit = TOutfit();
	this->posz = 255;
	this->Direction = 2;
	this->Name[0] = 0;
	this->Topic = 0;
	this->Price = 0;
	this->Amount = 1;
	this->TypeID = 0;
	this->Data = 0;
	this->LastTalk = 0;
	this->QueueLength = 0;
	this->Behaviour = nullptr;

	TReadScriptFile Script;
	Script.open(FileName);
	while (true) {
		Script.nextToken();
		if (Script.Token == ENDOFFILE) {
			break;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("identifier expected");
		}

		char Ident[30];
		strcpy(Ident, Script.getIdentifier());
		Script.readSymbol('=');

		if (strcmp(Ident, "name") == 0) {
			strcpy(this->Name, Script.readString());
		}

		if (strcmp(Ident, "sex") == 0) {
			char *value = Script.readIdentifier();
			this->Sex = 255;
			if (strcmp(value, "male") == 0) {
				this->Sex = 1;
			}
			if (strcmp(value, "female") == 0) {
				this->Sex = 2;
			}
			if (this->Sex == 255) {
				Script.error("unknown constant");
			}
		}

		if (strcmp(Ident, "race") == 0) {
			this->Race = Script.readNumber();
			this->SetSkills(this->Race);
		}

		if (strcmp(Ident, "outfit") == 0) {
			this->Outfit.load(Script);
		}

		if (strcmp(Ident, "home") == 0) {
			Script.readCoordinate(this->posx, this->posy, this->posz);
			Script.getCoordinate(this->startx, this->starty, this->startz);
		}

		if (strcmp(Ident, "radius") == 0) {
			this->Radius = Script.readNumber();
		}

		if (strcmp(Ident, "gostrength") == 0) {
			if (this->Race == 0) {
				throw "gostrength before race in npc-script-file";
			}
			int GoStrength = Script.readNumber();
			this->Skills[4]->SetAct(GoStrength);
			this->Skills[4]->SetMax(GoStrength);
		}

		if (strcmp(Ident, "behaviour") == 0) {
			if (this->Behaviour != nullptr) {
				throw "behaviour database specified twice for NPC";
			}
			this->Behaviour = new TBehaviourDatabase(Script);
		}
	}
	Script.close();

	if (this->Name[0] == 0) {
		throw "no name specified for NPC";
	}

	if (this->posz == 255) {
		throw "no startpoint specified for NPC";
	}

	if (this->Behaviour == nullptr) {
		throw "no behaviour database specified for NPC";
	}

	if (!IsOnMap(this->posx, this->posy, this->posz)) {
		print(1, "WARNUNG: NPC '%s' steht au\xdf" "erhalb der Karte.\n", this->Name);
		return;
	}

	this->OrgOutfit = this->Outfit;
	this->SetID(0);
	this->SetInList();
	if (!this->SetOnMap()) {
		print(1, "WARNUNG: Kann NPC '%s' nicht setzen.\n", this->Name);
		return;
	}

	this->ToDoYield();
}

TNPC::~TNPC() {
	delete this->Behaviour;
}

void TNPC::TalkStimulus(uint32_t Speaker, char *Text) {
	if (Speaker == this->ID) {
		return;
	}

	if (Text == nullptr) {
		error("TNPC::TalkStimulus: \xdc" "bergebener Text existiert nicht.\n");
		return;
	}

	if (this->State == TALKING || this->QueueLength != 0) {
		if (Speaker == this->Interlocutor) {
			this->LastTalk = RoundNr;
			this->Behaviour->react(this, Text, DEFAULT);
		} else {
			uint32_t Help = this->Interlocutor;
			this->Interlocutor = Speaker;
			this->Behaviour->react(this, Text, BUSY);
			this->Interlocutor = Help;
		}
	} else {
		this->ToDoClear();
		this->State = TALKING;
		this->Interlocutor = Speaker;
		this->LastTalk = RoundNr;
		this->Behaviour->react(this, Text, ADDRESS);
		if (this->State == TALKING) {
			this->TurnToInterlocutor();
		}
	}
}

void TNPC::IdleStimulus() {
	if (this->State == TALKING) {
		if (RoundNr <= this->LastTalk + 30) {
			this->ToDoWait(2000);
			this->ToDoStart();
			return;
		}
		this->Behaviour->react(this, "", VANISH);
		this->State = IDLE;
	}

	if (this->State != IDLE) {
		return;
	}

Again:
	if (this->QueueLength > 0) {
		this->State = TALKING;
		this->ToDoClear();
		this->Interlocutor = this->QueuedPlayers(0);
		if (GetCreature(this->Interlocutor) == nullptr) {
			error("TNPC::IdleStimulus: Gespr\xe4" "chspartner existiert nicht.\n");
			this->State = IDLE;
		} else {
			this->Behaviour->react(this, GetDynamicString(this->QueuedAddresses(0)), ADDRESSQUEUE);
		}

		DeleteDynamicString(this->QueuedAddresses(0));
		for (int i = 1; i < this->QueueLength; i++) {
			this->QueuedPlayers(i - 1) = this->QueuedPlayers(i);
			this->QueuedAddresses(i - 1) = this->QueuedAddresses(i);
		}
		this->QueueLength--;

		if (this->State == TALKING) {
			this->TurnToInterlocutor();
			this->LastTalk = RoundNr;
			return;
		}
		goto Again;
	}

	if (this->LockToDo) {
		return;
	}

	TFindCreatures Search(10, 10, this->ID, 1);
	if (Search.getNext() == 0) {
		this->State = SLEEPING;
		return;
	}

	int Try = 0;
	int destX;
	int destY;
	do {
		Try++;
		destX = this->posx;
		destY = this->posy;
		if (Try > 10) {
			this->ToDoWait(2000);
			this->ToDoStart();
			return;
		}

		int dir = rand() % 4;
		switch (dir) {
			case 0: destX--; break;
			case 1: destX++; break;
			case 2: destY--; break;
			case 3: destY++; break;
		}
	} while (!this->MovePossible(destX, destY, this->posz, true, false));

	try {
		this->ToDoGo(destX, destY, this->posz, true, INT_MAX);
		this->ToDoWait(2000);
		this->ToDoStart();
	} catch (RESULT r) {
		error("TNPC::IdleStimulus: Exception %d!\n", r);
	}
}

void TNPC::CreatureMoveStimulus(uint32_t CreatureID, int Type) {
	for (int i = 0; i < this->QueueLength; i++) {
		if (CreatureID == this->QueuedPlayers(i) || CreatureID == this->ID) {
			TCreature *cr = GetCreature(this->QueuedPlayers(i));
			// cr is not checked for NULL, as in the original.
			if (Type == 0
					|| abs(cr->posx - this->posx) >= 5
					|| abs(cr->posy - this->posy) >= 4
					|| cr->posz != this->posz) {
				DeleteDynamicString(this->QueuedAddresses(i));
				for (int j = i + 1; j < this->QueueLength; j++) {
					this->QueuedPlayers(j - 1) = this->QueuedPlayers(j);
					this->QueuedAddresses(j - 1) = this->QueuedAddresses(j);
				}
				this->QueueLength--;
				i--;
			}
		}
	}

	if (this->State == TALKING || this->State == LEAVING) {
		if (CreatureID == this->Interlocutor || CreatureID == this->ID) {
			TCreature *cr = GetCreature(this->Interlocutor);
			if (cr == nullptr) {
				error("TNPC::CreatureMoveStimulus: Gespr\xe4" "chspartner existiert nicht.\n");
				this->State = IDLE;
				this->ToDoYield();
				return;
			}

			if (Type != 0) {
				this->TurnToInterlocutor();
			}

			if (this->State != LEAVING) {
				if (Type == 0
						|| abs(cr->posx - this->posx) >= 5
						|| abs(cr->posy - this->posy) >= 4
						|| cr->posz != this->posz) {
					this->Behaviour->react(this, "", VANISH);
					this->State = IDLE;
					this->ToDoYield();
				}
			}
		}
	}

	if (CreatureID == this->ID) {
		return;
	}

	if (this->State == SLEEPING && Type != 0) {
		this->State = IDLE;
		this->ToDoYield();
	}
}

void TNPC::DamageStimulus(uint32_t, int, int) {
}

bool TNPC::MovePossible(int x, int y, int z, bool, bool) {
	return ::MovePossible(x, y, z)
		&& !CoordinateAvoid(x, y, z)
		&& abs(x - this->startx) <= this->Radius
		&& abs(y - this->starty) <= this->Radius
		&& z == this->startz
		&& !IsHouse(x, y, z);
}

void TNPC::GiveTo(ObjectType Type, int Amount) {
	if (Amount == 0) {
		return;
	}

	if (Type == ObjectType(0) || Type.getName(1) == nullptr) {
		error("TNPC::GiveTo: %s will Objekte vom Typ %d erschaffen.\n", this->Name, Type.getTypeID());
		return;
	}

	Log("npc", "%s -> %ld: %d %s\n", this->Name, (long)this->Interlocutor, Amount, Type.getName(1));
	try {
		if (Type.getFlag(CUMULATIVE)) {
			while (Amount > 0) {
				CreateAtCreature(this->Interlocutor, Type, std::min(Amount, 100));
				Amount -= std::min(Amount, 100);
			}
		} else {
			for (int i = 0; i < Amount; i++) {
				CreateAtCreature(this->Interlocutor, Type, this->Data);
			}
		}
	} catch (RESULT r) {
		error("TNPC::GiveTo: NPC %s, Exception %d, Objekt %d.\n", this->Name, r, Type.getTypeID());
	}
}

void TNPC::GetFrom(ObjectType Type, int Amount) {
	if (Amount == 0) {
		return;
	}

	Log("npc", "%s <- %ld: %d %s\n", this->Name, (long)this->Interlocutor, Amount, Type.getName(1));
	try {
		DeleteAtCreature(this->Interlocutor, Type, Amount, this->Data);
	} catch (RESULT r) {
		error("TNPC::GetFrom: NPC %s, Exception %d, Objekt %d.\n", this->Name, r, Type.getTypeID());
	}
}

void TNPC::GiveMoney(int Amount) {
	int Crystal = Amount / 10000;
	int Platinum = (Amount % 10000) / 100;
	int Gold = Amount % 100;
	this->GiveTo(GetSpecialObject(MONEY_TENTHOUSAND), Crystal);
	this->GiveTo(GetSpecialObject(MONEY_HUNDRED), Platinum);
	this->GiveTo(GetSpecialObject(MONEY_ONE), Gold);
}

void TNPC::GetMoney(int Amount) {
	int Crystal = CountInventoryObjects(this->Interlocutor, GetSpecialObject(MONEY_TENTHOUSAND), 0);
	int Platinum = CountInventoryObjects(this->Interlocutor, GetSpecialObject(MONEY_HUNDRED), 0);
	int Gold = CountInventoryObjects(this->Interlocutor, GetSpecialObject(MONEY_ONE), 0);
	CalculateChange(Amount, Gold, Platinum, Crystal);
	if (Gold > 0) {
		this->GetFrom(GetSpecialObject(MONEY_ONE), Gold);
	}
	if (Platinum > 0) {
		this->GetFrom(GetSpecialObject(MONEY_HUNDRED), Platinum);
	}
	if (Crystal > 0) {
		this->GetFrom(GetSpecialObject(MONEY_TENTHOUSAND), Crystal);
	}
	if (Gold < 0) {
		this->GiveTo(GetSpecialObject(MONEY_ONE), -Gold);
	}
	if (Platinum < 0) {
		this->GiveTo(GetSpecialObject(MONEY_HUNDRED), -Platinum);
	}
}

void TNPC::Enqueue(uint32_t Interlocutor, char *Text) {
	if (Interlocutor == 0) {
		error("TNPC::Enqueue: Gespr\xe4" "chspartner ist Null.\n");
		return;
	}

	if (Text == nullptr) {
		error("TNPC::Enqueue: Text ist NULL.\n");
		return;
	}

	for (int i = 0; i < this->QueueLength; i++) {
		if (this->QueuedPlayers(i) == Interlocutor) {
			return;
		}
	}

	this->QueuedPlayers(this->QueueLength) = Interlocutor;
	this->QueuedAddresses(this->QueueLength) = AddDynamicString(Text);
	this->QueueLength++;
}

void TNPC::TurnToInterlocutor() {
	TCreature *Opp = GetCreature(this->Interlocutor);
	if (Opp == nullptr) {
		error("TNPC::TurnToInterlocutor: Gespr\xe4" "chspartner existiert nicht.\n");
		return;
	}

	int dx = Opp->posx - this->posx;
	int dy = Opp->posy - this->posy;
	int Dir;
	if (abs(dx) < abs(dy)) {
		if (dy < 0) {
			Dir = 0;
		} else {
			Dir = 2;
		}
	} else {
		if (dx < 0) {
			Dir = 3;
		} else {
			Dir = 1;
		}
	}
	this->Rotate((unsigned char)Dir);
}

void TNPC::ChangeState(STATE NewState, bool Stimulus) {
	this->State = NewState;
	if (Stimulus) {
		this->ToDoYield();
	}
}

void ChangeNPCState(TCreature *npc, int NewState, bool Stimulus) {
	if (npc == nullptr) {
		error("ChangeNPCState: npc ist NULL.\n");
		return;
	}

	if (npc->Type != NPC) {
		error("ChangeNPCState: npc ist kein NPC.\n");
		return;
	}

	((TNPC *)npc)->ChangeState((STATE)NewState, Stimulus);
}

TMonster::TMonster(int Race, int x, int y, int z, int Home, uint32_t Father) {
	this->Type = MONSTER;
	this->State = IDLE;
	this->Race = Race;
	this->startx = x;
	this->posx = x;
	this->starty = y;
	this->posy = y;
	this->startz = z;
	this->posz = z;
	this->Home = Home;
	this->Master = Father;
	this->Target = 0;

	while (this->Master != 0) {
		TCreature *master = GetCreature(this->Master);
		if (master == nullptr) {
			// The constructor ends here and leaves the monster off the lists
			// and off the map, as in the original.
			error("TMonster::TMonster: Master existiert nicht.\n");
			return;
		}

		if (master->Type == MONSTER && ((TMonster *)master)->Master != 0) {
			error("TMonster::TMonster: Kinder d\xfcrfen keine eigenen Kinder erschaffen.\n");
			this->Master = ((TMonster *)master)->Master;
			continue;
		}

		master->SummonedCreatures++;
		this->LifeEndRound = master->LifeEndRound;
		break;
	}

	if (RaceData[Race].Article[0] == 0) {
		strcpy(this->Name, RaceData[Race].Name);
	} else {
		snprintf(this->Name, 30, "%s %s", RaceData[Race].Article, RaceData[Race].Name);
	}
	this->SetSkills(Race);
	this->Outfit = RaceData[Race].Outfit;
	this->OrgOutfit = RaceData[Race].Outfit;
	this->SetID(0);
	this->SetInList();
	if (!this->SetOnMap()) {
		error("TMonster::TMonster: Kann Monster nicht auf die Karte setzen.\n");
		return;
	}

	if (this->Master == 0 && RaceData[Race].Items > 0) {
		try {
			Object Bag = Create(GetBodyContainer(this->ID, 3), GetSpecialObject(INVENTORY_CONTAINER), 0);
			for (int i = 1; i <= RaceData[Race].Items; i++) {
				TItemData &I = RaceData[Race].Item(i);
				if (random(0, 999) > I.Probability) {
					continue;
				}

				int Amount = random(1, I.Maximum);
				int Repeat = 1;
				if (!I.Type.getFlag(CUMULATIVE)) {
					Repeat = Amount;
					Amount = 0;
				}

				for (int j = 0; j < Repeat; j++) {
					Object Obj;
					if (I.Type.getFlag(WEAPON)
							|| I.Type.getFlag(SHIELD)
							|| I.Type.getFlag(BOW)
							|| I.Type.getFlag(THROW)
							|| I.Type.getFlag(WAND)
							|| I.Type.getFlag(WEAROUT)
							|| I.Type.getFlag(EXPIRE)
							|| I.Type.getFlag(EXPIRESTOP)) {
						try {
							Obj = Create(Bag, I.Type, 0);
						} catch (RESULT r) {
							error("TMonster::TMonster: Exception %d bei Rasse %d, ggf. CarryStrength erh\xf6hen.\n", r, Race);
							break;
						}
					} else {
						Obj = CreateAtCreature(this->ID, I.Type, Amount);
					}

					if (Obj.getContainer().isMapContainer()) {
						error("TMonster::TMonster: Objekt f\xe4llt auf die Karte. CarryStrength f\xfcr Rasse %d erh\xf6hen.\n", Race);
						Delete(Obj, -1);
					}
				}
			}

			if (GetFirstContainerObject(Bag) == NONE) {
				Delete(Bag, -1);
			}
		} catch (RESULT r) {
			error("TMonster::TMonster: Exception %d bei Rasse %d.\n", r, Race);
		}
	}

	this->Combat.CheckCombatValues();
	this->Combat.SetAttackMode(2);
	this->ToDoWait(0);
	this->ToDoStart();
}

TMonster::~TMonster() {
	if (!this->IsDead) {
		GraphicalEffect(this->posx, this->posy, this->posz, 3);
	} else if (this->Master == 0) {
		this->Combat.DistributeExperiencePoints(RaceData[this->Race].ExperiencePoints);
	}

	if (this->Home != 0) {
		NotifyMonsterhomeOfDeath(this->Home);
	}

	if (this->Master != 0) {
		TCreature *master = GetCreature(this->Master);
		if (master != nullptr && master->SummonedCreatures > 0) {
			master->SummonedCreatures--;
		}
	}
}

void TMonster::IdleStimulus() {
	if (this->LockToDo) {
		return;
	}

	if (this->LoggingOut) {
		return;
	}

	if (this->LifeEndRound != 0 && RoundNr >= this->LifeEndRound) {
		print(3, "Lebenszeit f\xfcr %s abgelaufen.\n", this->Name);
		this->StartLogout(true, true);
		this->State = SLEEPING;
		return;
	}

	if (this->Master != 0) {
		TCreature *master = GetCreature(this->Master);
		if (master == nullptr) {
			if (this->Master >= 0x40000000) {
				print(3, "Kreatur %s verliert ihr Mutter-Monster.\n", this->Name);
				this->Skills[2]->Set(0);
				this->Death();
			} else {
				print(3, "Kreatur %s verliert ihren Spieler-Master.\n", this->Name);
				this->StartLogout(true, true);
			}
			this->State = SLEEPING;
			return;
		}

		if (this->Master < 0x40000000 && master->SummonedCreatures == 0) {
			print(3, "Spieler-Master hat sich eben wieder neu eingeloggt.\n");
			this->StartLogout(true, true);
			this->State = SLEEPING;
			return;
		}

		if ((master != nullptr
					&& (abs(this->posz - master->posz) > 1
						|| abs(this->posx - master->posx) > 30
						|| abs(this->posy - master->posy) > 30))
				|| (this->posz != master->posz && this->Master >= 0x40000000)) {
			if (this->Master < 0x40000000) {
				print(3, "Spieler-Master ist zu weit entfernt.\n");
				this->StartLogout(true, true);
			} else {
				this->Skills[2]->Set(0);
				this->Death();
				print(3, "Mutter-Monster ist zu weit entfernt.\n");
			}
			this->State = SLEEPING;
			return;
		}

		if (master != nullptr && !master->Combat.IsFollowing()) {
			this->Target = master->Combat.GetAttackDest();
		} else {
			this->Target = 0;
		}
		if (this->Target == 0 || this->Target == this->ID) {
			this->Target = this->Master;
		}
	} else if (!MonsterhomeInRange(this->Home, this->posx, this->posy, this->posz)) {
		print(3, "%s an [%d,%d,%d] zu weit von [%d,%d,%d] entfernt.\n",
				this->Name, this->posx, this->posy, this->posz,
				Monsterhome(this->Home).x, Monsterhome(this->Home).y, Monsterhome(this->Home).z);
		this->StartLogout(true, true);
		this->State = SLEEPING;
		return;
	}

	if (this->Target != 0) {
		TCreature *target = GetCreature(this->Target);
		if (target == nullptr
				|| abs(this->posx - target->posx) > 10
				|| abs(this->posy - target->posy) > 10
				|| this->posz != target->posz) {
			this->Target = 0;
		}

		if (target != nullptr) {
			if (IsProtectionZone(target->posx, target->posy, target->posz)) {
				this->Target = 0;
			}
			if (IsHouse(target->posx, target->posy, target->posz)) {
				this->Target = 0;
			}
			if (!RaceData[this->Race].SeeInvisible && target->IsInvisible()) {
				this->Target = 0;
			}
		}

		if (this->Master == 0 && random(0, 99) < RaceData[this->Race].LoseTarget) {
			this->Target = 0;
		}
	}

	if (this->State != PANIC && this->State != UNDERATTACK) {
		this->State = IDLE;
	}

	if (RaceData[this->Race].Talks > 0 && rand() % 50 == 0) {
		int Nr = rand() % RaceData[this->Race].Talks + 1;
		char *Text = GetDynamicString(RaceData[this->Race].Talk(Nr));
		try {
			if (Text[0] == '#') {
				Talk(this->ID, TALK_ANIMAL_LOUD, nullptr, Text + 3, false);
			} else {
				Talk(this->ID, TALK_ANIMAL_LOW, nullptr, Text, false);
			}
		} catch (RESULT r) {
			error("TMonster::IdleStimulus: Exception %d bei Talk.\n", r);
		}
	}

	if (this->Target == 0) {
		bool Sleep = true;
		if (this->Master == 0) {
			int Goodness = INT_MIN;
			int TieBreaker = 0;
			int Strategy = 0;
			int StrategySelection = random(0, 99);
			for (int i = 0; i < 3; i++) {
				if (StrategySelection < RaceData[this->Race].Strategy[i]) {
					break;
				}
				Strategy++;
				StrategySelection -= RaceData[this->Race].Strategy[i];
			}

			TFindCreatures Search(12, 12, this->ID, 5);
			while (true) {
				uint32_t ID = Search.getNext();
				if (ID == 0) {
					break;
				}

				TCreature *cr = GetCreature(ID);
				if (cr == nullptr) {
					error("TMonster::IdleStimulus: Kreatur existiert nicht.\n");
					break;
				}

				if (cr->Type == MONSTER) {
					uint32_t master = ((TMonster *)cr)->Master;
					if (master == 0) {
						continue;
					}
					TCreature *help = GetCreature(master);
					if (help == nullptr || help->Type != PLAYER) {
						continue;
					}
				}

				if ((cr->posz <= 7 && this->posz <= 7) || (cr->posz > 7 && abs(this->posz - cr->posz) <= 2)) {
					Sleep = false;
				}

				if (this->posz != cr->posz) {
					continue;
				}

				if (cr->Type == PLAYER && CheckRight(cr->ID, IGNORED_BY_MONSTERS)) {
					continue;
				}

				if (!RaceData[this->Race].SeeInvisible && cr->IsInvisible()) {
					continue;
				}

				if (IsProtectionZone(cr->posx, cr->posy, cr->posz)) {
					continue;
				}

				if (IsHouse(cr->posx, cr->posy, cr->posz)) {
					continue;
				}

				int dx = abs(this->posx - cr->posx);
				int dy = abs(this->posy - cr->posy);
				if (dx > 10 || dy > 10) {
					continue;
				}

				int NewGoodness;
				switch (Strategy) {
					case 0:
						NewGoodness = -(dx + dy);
						break;
					case 1:
						NewGoodness = -cr->Skills[2]->Get();
						break;
					case 2:
						NewGoodness = (int)this->Combat.GetDamageByCreature(cr->ID);
						break;
					case 3:
						NewGoodness = 0;
						break;
					default:
						NewGoodness = 0;
						error("TMonster::IdleStimulus: Unbekannte Strategie %d.\n", Strategy);
						break;
				}

				int NewTieBreaker = random(0, 99);
				if (NewGoodness > Goodness || (NewGoodness == Goodness && NewTieBreaker > TieBreaker)) {
					this->Target = ID;
					Goodness = NewGoodness;
					TieBreaker = NewTieBreaker;
				}
			}
		}

		if (this->Target == 0 && Sleep && this->State != UNDERATTACK && this->State != PANIC) {
			if (this->Master == 0) {
				this->State = SLEEPING;
				return;
			}
			this->ToDoWait(1000);
			this->ToDoStart();
			return;
		}

		if (this->Target == 0 && this->State == PANIC) {
			this->State = IDLE;
		}

		if (this->State == UNDERATTACK) {
			this->State = IDLE;
		}
	}

	if (RaceData[this->Race].Spells > 0) {
		TCreature *target = nullptr;
		if (this->Target != 0) {
			target = GetCreature(this->Target);
		}

		for (int i = 1; i <= RaceData[this->Race].Spells; i++) {
			TSpellData &S = RaceData[this->Race].Spell(i);
			if (rand() % S.Delay != 0) {
				continue;
			}

			if (this->Master == 0
					&& this->Skills[2]->Get() <= RaceData[this->Race].FleeThreshold
					&& random(1, 3) != 1) {
				continue;
			}

			TImpact *Impact = nullptr;
			switch (S.Impact) {
				case IMPACT_DAMAGE:
					Impact = new TDamageImpact(this, S.ImpactParam1,
							ComputeDamage(this, 0, S.ImpactParam2, S.ImpactParam3), true);
					break;
				case IMPACT_FIELD:
					Impact = new TFieldImpact(this, S.ImpactParam1);
					break;
				case IMPACT_HEALING:
					Impact = new THealingImpact(this,
							ComputeDamage(this, 0, S.ImpactParam1, S.ImpactParam2));
					break;
				case IMPACT_SPEED:
					Impact = new TSpeedImpact(this,
							ComputeDamage(this, 0, S.ImpactParam1, S.ImpactParam2), S.ImpactParam3);
					break;
				case IMPACT_DRUNKEN:
					Impact = new TDrunkenImpact(this,
							ComputeDamage(this, 0, S.ImpactParam1, S.ImpactParam2), S.ImpactParam3);
					break;
				case IMPACT_STRENGTH:
					Impact = new TStrengthImpact(this, S.ImpactParam1,
							ComputeDamage(this, 0, S.ImpactParam2, S.ImpactParam3), S.ImpactParam4);
					break;
				case IMPACT_OUTFIT: {
					TOutfit HelpOutfit;
					HelpOutfit.OutfitID = S.ImpactParam1;
					HelpOutfit.Colors[0] = (unsigned char)S.ImpactParam2;
					HelpOutfit.Colors[1] = (unsigned char)(S.ImpactParam2 / 256);
					HelpOutfit.Colors[2] = (unsigned char)S.ImpactParam3;
					HelpOutfit.Colors[3] = (unsigned char)(S.ImpactParam3 / 256);
					Impact = new TOutfitImpact(this, HelpOutfit, S.ImpactParam4);
					break;
				}
				case IMPACT_SUMMON:
					if (this->Master == 0) {
						Impact = new TSummonImpact(this, S.ImpactParam1, S.ImpactParam2);
					}
					break;
			}

			if (Impact == nullptr) {
				continue;
			}

			if (!Impact->isAggressive() || (this->Target != 0 && this->Target != this->Master)) {
				switch (S.Shape) {
					case SHAPE_ACTOR:
						ActorShapeSpell(this, Impact, S.ShapeParam1);
						break;
					case SHAPE_VICTIM:
						if (target != nullptr) {
							this->Rotate(target);
							VictimShapeSpell(this, target, S.ShapeParam1, S.ShapeParam2, Impact, S.ShapeParam3);
						}
						break;
					case SHAPE_ORIGIN:
						OriginShapeSpell(this, S.ShapeParam1, Impact, S.ShapeParam2);
						break;
					case SHAPE_DESTINATION:
						if (target != nullptr) {
							this->Rotate(target);
							DestinationShapeSpell(this, target, S.ShapeParam1, S.ShapeParam2, S.ShapeParam3,
									Impact, S.ShapeParam4);
						}
						break;
					case SHAPE_ANGLE:
						if (target != nullptr) {
							this->Rotate(target);
						}
						AngleShapeSpell(this, S.ShapeParam1, S.ShapeParam2, Impact, S.ShapeParam3);
						break;
				}
			}

			// TImpact has no virtual destructor and all impacts have trivial
			// ones: the original only frees the memory, as this does.
			::operator delete(Impact);
		}
	}

	if (this->Target != 0) {
		TCreature *target = GetCreature(this->Target);
		if (target == nullptr) {
			error("TMonster::IdleStimulus: Kreatur existiert nicht.\n");
			this->Target = 0;
			goto RandomMove;
		}

		try {
			if (this->Master == 0 && this->Skills[2]->Get() <= RaceData[this->Race].FleeThreshold) {
				int x;
				int y;
				int z;
				if (!SearchFlightField(this->ID, this->Target, x, y, z)) {
					goto RandomMove;
				}
				this->ToDoGo(x, y, z, true, INT_MAX);
			} else if (this->Target == this->Master) {
				int dx = target->posx - this->posx;
				int dy = target->posy - this->posy;
				if (abs(dx) + abs(dy) <= 1) {
					goto RandomMove;
				}

				if (abs(dx) + abs(dy) == 2) {
					this->ToDoWait(1000);
				} else {
					if (abs(dx) + abs(dy) == 3) {
						this->ToDoWait(1000);
					}
					this->ToDoGo(target->posx, target->posy, target->posz, false, 3);
				}
				this->ToDoStart();
				return;
			} else {
				if (this->Skills[11]->Get() > 0 && this->State != PANIC) {
					this->State = ATTACKING;
				}

				if (this->State == ATTACKING || this->State == PANIC) {
					this->Combat.SetAttackDest(this->Target, false);
					this->Combat.SetChaseMode(0);
				}

				if (RaceData[this->Race].DistanceFighting
						&& this->Target != this->Master
						&& ThrowPossible(this->posx, this->posy, this->posz,
							target->posx, target->posy, target->posz, 0)) {
					int Distance = ObjectDistance(this->GetCreatureObject(), target->GetCreatureObject());
					if (Distance <= 3) {
						int x;
						int y;
						int z;
						if (SearchFlightField(this->ID, this->Target, x, y, z)) {
							this->ToDoGo(x, y, z, true, INT_MAX);
						} else {
							this->ToDoWait(1000);
						}
					} else if (Distance == 4) {
						int destX = this->posx;
						int destY = this->posy;
						int dir = rand() % 5;
						switch (dir) {
							case 0: destX--; break;
							case 1: destX++; break;
							case 2: destY--; break;
							case 3: destY++; break;
						}
						if (dir <= 3
								&& this->MovePossible(destX, destY, this->posz, true, false)
								&& std::max(abs(destX - target->posx), abs(destY - target->posy)) == 4) {
							this->ToDoGo(destX, destY, this->posz, true, INT_MAX);
						}
						this->ToDoWait(1000);
					} else {
						this->ToDoGo(target->posx, target->posy, target->posz, false, Distance - 4);
					}
				} else {
					this->Combat.SetChaseMode(1);
					if (abs(this->posx - target->posx) <= 1 && abs(this->posy - target->posy) <= 1) {
						int destX = this->posx;
						int destY = this->posy;
						int dir = rand() % 5;
						switch (dir) {
							case 0: destX--; break;
							case 1: destX++; break;
							case 2: destY--; break;
							case 3: destY++; break;
						}
						if (dir <= 3
								&& this->MovePossible(destX, destY, this->posz, true, false)
								&& abs(destX - target->posx) <= 1
								&& abs(destY - target->posy) <= 1) {
							this->ToDoGo(destX, destY, this->posz, true, INT_MAX);
						}
						if (this->State == PANIC) {
							this->State = ATTACKING;
						}
					} else if (this->State != ATTACKING && this->State != PANIC) {
						this->ToDoGo(target->posx, target->posy, target->posz, false, 3);
					}
				}

				if (this->State == ATTACKING || this->State == PANIC) {
					this->Rotate(GetCreature(this->Target));
					if (this->Master == 0 || this->Combat.GetAttackDest() != this->Master) {
						this->ToDoAttack();
					} else {
						error("TMonster::IdleStimulus: %s greift Master %lu an (St=%d, T: %lu).\n",
								this->Name, (unsigned long)this->Combat.GetAttackDest(), this->State,
								(unsigned long)this->Target);
					}
				} else {
					this->ToDoWait(1000);
				}
			}
		} catch (RESULT r) {
			this->Target = 0;
			this->ToDoClear();
			if (r != EXHAUSTED) {
				goto RandomMove;
			}
			this->ToDoWait(1000);
			this->ToDoStart();
			return;
		}

		this->ToDoStart();
		return;
	}

RandomMove:
	{
		int Try = 0;
		int destX = this->posx;
		int destY = this->posy;
	Again:
		Try++;
		if (Try > 10) {
			this->ToDoWait(1000);
			this->ToDoStart();
			return;
		}

		int dir = rand() % 4;
		switch (dir) {
			case 0: destX--; break;
			case 1: destX++; break;
			case 2: destY--; break;
			case 3: destY++; break;
		}

		try {
			if (!this->MovePossible(destX, destY, this->posz, true, false)) {
				destX = this->posx;
				destY = this->posy;
				goto Again;
			}
		} catch (RESULT r) {
			destX = this->posx;
			destY = this->posy;
			goto Again;
		}

		try {
			this->ToDoGo(destX, destY, this->posz, true, INT_MAX);
			this->ToDoWait(1000);
			this->ToDoStart();
		} catch (RESULT r) {
			error("TMonster::IdleStimulus: Exception %d.\n", r);
		}
	}
}

void TMonster::CreatureMoveStimulus(uint32_t CreatureID, int Type) {
	if (CreatureID == this->ID) {
		if (this->State == SLEEPING && Type != 0) {
			this->State = IDLE;
			this->ToDoYield();
		}
		return;
	}

	if (this->State == SLEEPING && Type != 0) {
		TCreature *cr = GetCreature(CreatureID);
		if (cr == nullptr) {
			error("TMonster::CreatureMoveStimulus: Kreatur %d existiert nicht.\n", CreatureID);
			return;
		}

		if (cr->Type == NPC) {
			return;
		}

		if (cr->Type == MONSTER) {
			if (((TMonster *)cr)->Master == 0) {
				return;
			}
			TCreature *master = GetCreature(((TMonster *)cr)->Master);
			if (master == nullptr || master->Type != PLAYER) {
				return;
			}
		}

		this->State = IDLE;
		this->ToDoYield();
	}

	TCreature::CreatureMoveStimulus(CreatureID, Type);
}

void TMonster::DamageStimulus(uint32_t Attacker, int Damage, int) {
	if (Attacker == 0 || Damage == 0) {
		return;
	}

	if (this->State == SLEEPING) {
		this->State = (this->Target == 0 ? PANIC : UNDERATTACK);
		this->ToDoYield();
		return;
	}

	if (this->State == IDLE) {
		this->State = UNDERATTACK;
	}

	if (this->Target == 0) {
		this->State = PANIC;
	}
}

bool TMonster::MovePossible(int x, int y, int z, bool Execute, bool) {
	if (this->posz != z) {
		error("TMonster::MovePossible: Pr\xfc" "fung \xfc" "ber Ebenen hinweg ([%d,%d,%d] -> [%d,%d,%d]).\n",
				this->posx, this->posy, this->posz, x, y, z);
		return false;
	}

	if (this->State != ATTACKING && this->State != PANIC && !MonsterhomeInRange(this->Home, x, y, z)) {
		return false;
	}

	if (this->State != ATTACKING && this->State != PANIC
			&& std::max(abs(x - this->startx), abs(y - this->starty)) > this->Radius) {
		return false;
	}

	if (this->Skills[4]->GetAct() < 0) {
		error("TMonster::MovePossible: Monster %s an [%d,%d,%d] darf sich nicht bewegen.\n",
				this->Name, this->posx, this->posy, this->posz);
		return false;
	}

	if (IsProtectionZone(x, y, z)) {
		return false;
	}

	if (IsHouse(x, y, z)) {
		return false;
	}

	if (Execute && this->Master != 0 && this->State != ATTACKING && this->State != PANIC) {
		TCreature *master = GetCreature(this->Master);
		if (master != nullptr
				&& master->posz == this->posz
				&& abs(master->posx - this->posx) + abs(master->posy - this->posy) > 1
				&& abs(master->posx - x) + abs(master->posy - y) <= 1) {
			return false;
		}
	}

	int Try = 0;
Again:
	Try++;
	if (Try > 100) {
		error("TMonster::MovePossible: Endlosschleife vermutet f\xfcr %s an [%d,%d,%d].\n", this->Name, x, y, z);
		throw EXHAUSTED;
	}

	Object Obj = GetFirstObject(x, y, z);
	if (Obj == NONE || !Obj.getFlag(BANK)) {
		return false;
	}

	while (Obj != NONE) {
		if (Obj.isCreature()) {
			if (this->State != ATTACKING && this->State != PANIC) {
				return false;
			}

			if (this->Target == 0) {
				return false;
			}

			if (!RaceData[this->Race].KickCreatures) {
				return false;
			}

			TCreature *Obstacle = GetCreature(Obj);
			if (Obstacle == nullptr) {
				error("TMonster::MovePossible: Kann Hindernis-Kreatur nicht identifizieren.\n");
				return false;
			}

			if (RaceData[Obstacle->Race].Unpushable) {
				return false;
			}

			if (Obstacle->Type == NPC) {
				return false;
			}

			if (Obstacle->Type == PLAYER) {
				if (this->Master != 0) {
					return false;
				}
				if (CheckRight(Obstacle->ID, IGNORED_BY_MONSTERS)) {
					return false;
				}
			}

			if (!RaceData[this->Race].SeeInvisible && Obstacle->IsInvisible()) {
				return false;
			}

			if (Obstacle->ID == this->Master) {
				return false;
			}

			if (Obstacle->ID == this->Target) {
				return false;
			}

			if (Execute) {
				if (Obstacle->Type == PLAYER) {
					this->Target = 0;
					throw EXHAUSTED;
				}
				if (!this->KickCreature(Obstacle)) {
					throw EXHAUSTED;
				}
				goto Again;
			}
		} else {
			if (Obj.getFlag(UNPASS)) {
				if (Obj.getFlag(UNMOVE)) {
					return false;
				}
				if (!this->CanKickBoxes()) {
					return false;
				}
				if (Execute) {
					this->KickBoxes(Obj);
					goto Again;
				}
			}

			if (Obj.getFlag(AVOID)
					&& !(this->State == PANIC && Obj.getAttribute(AVOIDDAMAGETYPES) != 0)
					&& !(RaceData[this->Race].NoBurning && Obj.getAttribute(AVOIDDAMAGETYPES) == 4)
					&& !(RaceData[this->Race].NoPoison && Obj.getAttribute(AVOIDDAMAGETYPES) == 2)
					&& !(RaceData[this->Race].NoEnergy && Obj.getAttribute(AVOIDDAMAGETYPES) == 8)) {
				if (Obj.getFlag(UNMOVE)) {
					return false;
				}
				if (!this->CanKickBoxes()) {
					return false;
				}
				if (Execute) {
					this->KickBoxes(Obj);
					goto Again;
				}
			}
		}

		Obj++;
	}

	return true;
}

bool TMonster::IsPeaceful() {
	return this->Master != 0 && this->Master < 0x40000000;
}

bool TMonster::CanKickBoxes() {
	if (RaceData[this->Race].KickBoxes) {
		return true;
	}

	if (this->Master == 0) {
		return false;
	}

	TCreature *master = GetCreature(this->Master);
	if (master == nullptr || master->Type != MONSTER) {
		return false;
	}

	return ((TMonster *)master)->CanKickBoxes();
}

void TMonster::KickBoxes(Object Obj) {
	if (!Obj.exists()) {
		error("TMonster::KickBoxes: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	int x;
	int y;
	int z;
	GetObjectCoordinates(Obj, x, y, z);
	try {
		if (y - 1 != this->posy && ::MovePossible(x, y - 1, z)) {
			::Move(this->ID, Obj, GetMapContainer(x, y - 1, z), -1, false, NONE);
		} else if (y + 1 != this->posy && ::MovePossible(x, y + 1, z)) {
			::Move(this->ID, Obj, GetMapContainer(x, y + 1, z), -1, false, NONE);
		} else if (x - 1 != this->posx && ::MovePossible(x - 1, y, z)) {
			::Move(this->ID, Obj, GetMapContainer(x - 1, y, z), -1, false, NONE);
		} else if (x + 1 != this->posx && ::MovePossible(x + 1, y, z)) {
			::Move(this->ID, Obj, GetMapContainer(x + 1, y, z), -1, false, NONE);
		} else {
			GraphicalEffect(Obj, 4);
			Delete(Obj, -1);
		}
	} catch (RESULT r) {
		error("TMonster::KickBoxes: Exception %d, Objekt %d.\n", r, Obj.getObjectType().getTypeID());
		error("# eigene Position: [%d,%d,%d] - Objektposition: [%d,%d,%d]\n",
				this->posx, this->posy, this->posz, x, y, z);
	}
}

bool TMonster::KickCreature(TCreature *Obstacle) {
	if (Obstacle == nullptr) {
		error("TMonster::KickCreature: \xdc" "bergebene Kreatur existiert nicht.\n");
		return false;
	}

	if (Obstacle->Type != MONSTER) {
		error("TMonster::KickCreature: Zu verschiebende Kreatur ist kein Monster.\n");
		return false;
	}

	print(3, "%s verschiebt %s.\n", this->Name, Obstacle->Name);
	unsigned short HelpX = 0;
	unsigned short HelpY = 0;
	try {
		int x = Obstacle->posx;
		int y = Obstacle->posy;
		int z = Obstacle->posz;
		if (y - 1 != this->posy
				&& Obstacle->MovePossible(x, y - 1, z, true, false)
				&& !CoordinateAvoid(x, y - 1, z)) {
			HelpX = (unsigned short)x;
			HelpY = (unsigned short)(y - 1);
			::Move(this->ID, Obstacle->GetCreatureObject(), GetMapContainer(x, y - 1, z), -1, false, NONE);
			return true;
		}

		if (y + 1 != this->posy
				&& Obstacle->MovePossible(x, y + 1, z, true, false)
				&& !CoordinateAvoid(x, y + 1, z)) {
			HelpX = (unsigned short)x;
			HelpY = (unsigned short)(y + 1);
			::Move(this->ID, Obstacle->GetCreatureObject(), GetMapContainer(x, y + 1, z), -1, false, NONE);
			return true;
		}

		if (x - 1 != this->posx
				&& Obstacle->MovePossible(x - 1, y, z, true, false)
				&& !CoordinateAvoid(x - 1, y, z)) {
			HelpX = (unsigned short)(x - 1);
			HelpY = (unsigned short)y;
			::Move(this->ID, Obstacle->GetCreatureObject(), GetMapContainer(x - 1, y, z), -1, false, NONE);
			return true;
		}

		if (x + 1 != this->posx
				&& Obstacle->MovePossible(x + 1, y, z, true, false)
				&& !CoordinateAvoid(x + 1, y, z)) {
			HelpX = (unsigned short)(x + 1);
			HelpY = (unsigned short)y;
			::Move(this->ID, Obstacle->GetCreatureObject(), GetMapContainer(x + 1, y, z), -1, false, NONE);
			return true;
		}

		print(3, "Kein Platz zum Verschieben => T\xf6ten.\n");
		GraphicalEffect(Obstacle->GetCreatureObject(), 4);
		Obstacle->Combat.AddDamageToCombatList(this->ID, (unsigned short)Obstacle->Skills[2]->Get());
		Obstacle->Skills[2]->Set(0);
		Obstacle->Death();
	} catch (RESULT r) {
		error("TMonster::KickCreature: Exception %d, Kreatur %s.\n", r, Obstacle->Name);
		error("# eigene Position: [%d,%d,%d] - Hindernisposition: [%d,%d,%d]\n",
				this->posx, this->posy, this->posz, Obstacle->posx, Obstacle->posy, Obstacle->posz);
		error("# Objekte auf Zielfeld [%d,%d,%d]:\n", HelpX, HelpY, Obstacle->posz);
		Object Obj = GetFirstObject(HelpX, HelpY, Obstacle->posz);
		while (Obj != NONE) {
			error("# %d\n", Obj.getObjectType().getTypeID());
			Obj++;
		}
	}

	return false;
}

void TMonster::Convince(TCreature *newMaster) {
	if (this->Home != 0) {
		NotifyMonsterhomeOfDeath(this->Home);
	}
	this->Home = 0;

	if (this->Master != 0) {
		TCreature *oldMaster = GetCreature(this->Master);
		if (oldMaster != nullptr) {
			oldMaster->SummonedCreatures--;
		}
	}

	this->Master = newMaster->ID;
	newMaster->SummonedCreatures++;
	this->ToDoClear();
	this->ToDoWait(100);
	this->ToDoStart();
}

void TMonster::SetTarget(TCreature *NewTarget) {
	if (NewTarget == nullptr) {
		error("TMonster::SetTarget: NewTarget ist NULL.\n");
		return;
	}

	if (this->Master != 0) {
		return;
	}

	this->Rotate(NewTarget);
	this->Target = NewTarget->ID;
	this->ToDoYield();
}

TCreature *CreateMonster(int Race, int x, int y, int z, int Home, uint32_t Father, bool ShowEffect) {
	if (Race < 1 || Race > 511) {
		error("CreateMonster: Ung\xfcltige Rassennummer %d.\n", Race);
		return nullptr;
	}

	if (RaceData[Race].Name[0] == 0) {
		error("CreateMonster: Daten f\xfcr Rasse %d nicht definiert.\n", Race);
		return nullptr;
	}

	SearchFreeField(x, y, z, 2, 0, false);
	TMonster *cr = new TMonster(Race, x, y, z, Home, Father);
	if (ShowEffect) {
		GraphicalEffect(x, y, z, 11);
	}
	return cr;
}

void ConvinceMonster(TCreature *Master, TCreature *Slave) {
	if (Master == nullptr) {
		error("ConvinceMonster: Master existiert nicht.\n");
		return;
	}

	if (Slave == nullptr) {
		error("ConvinceMonster: Slave existiert nicht.\n");
		return;
	}

	if (Slave->Type != MONSTER) {
		error("ConvinceMonster: Slave ist kein Monster.\n");
		return;
	}

	((TMonster *)Slave)->Convince(Master);
}

void ChallengeMonster(TCreature *Challenger, TCreature *Monster) {
	if (Challenger == nullptr) {
		error("ChallengeMonster: Herausforderer existiert nicht.\n");
		return;
	}

	if (Monster == nullptr) {
		error("ChallengeMonster: Monster existiert nicht.\n");
		return;
	}

	if (Monster->Type != MONSTER) {
		error("ChallengeMonster: Monster ist kein Monster.\n");
		return;
	}

	((TMonster *)Monster)->SetTarget(Challenger);
}

void InitNPCs() {
	DIR *WorkDir = opendir(NPCPATH);
	if (WorkDir == nullptr) {
		error("InitNPCs: Unterverzeichnis %s nicht gefunden\n", NPCPATH);
		throw "Cannot init NPCs";
	}

	while (true) {
		struct dirent *DirEntry = readdir(WorkDir);
		if (DirEntry == nullptr) {
			break;
		}

		char *s = DirEntry->d_name;
		if (strlen(s) > 4 && strcmp(s + strlen(s) - 4, ".npc") == 0) {
			char FileName[4096];
			sprintf(FileName, "%s/%s", NPCPATH, s);
			new TNPC(FileName);
		}
	}

	closedir(WorkDir);
}

void InitNonplayer() {
	print(1, "Initializing Nonplayers ...\n");
	FirstFreeNonplayer = 0;
	CurrentNonplayer = 0;
	NodeTable = new store<TNode, 256>;
	InitNPCs();
	LoadMonsterhomes();
}

void ExitNonplayer() {
	// A NULL entry at position 0 would loop forever, as in the original.
	while (FirstFreeNonplayer > 0) {
		TNonplayer *np = NonplayerList(0);
		if (np != nullptr) {
			delete np;
		}
	}

	delete NodeTable;
}
