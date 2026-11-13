#ifndef PATELLA_CONTAINR_HH
#define PATELLA_CONTAINR_HH

#include <cstdlib>

template<class T> struct matrix;
template<class T> struct matrix3d;
template<class T> struct vector;
template<class T> struct listnode;
template<class T> struct listIterator;
template<class T> struct list;
template<class T> union storeitem;
template<class T, int N> struct storeunit;
template<class T, int N> struct store;
template<class T> struct fifoIterator;
template<class T> struct fifo;
template<class K, class T> struct priority_queue_entry;
template<class K, class T> struct priority_queue;

template<class T>
struct matrix {
	matrix(int xmin, int xmax, int ymin, int ymax, T init);
	matrix(int xmin, int xmax, int ymin, int ymax);
	~matrix();
	T &operator()(int x, int y);
private:
	int xmin;
	int ymin;
	int dx;
	int dy;
	T *entry;
};

template<class T>
struct matrix3d {
	matrix3d(int xmin, int xmax, int ymin, int ymax, int zmin, int zmax, T init);
	matrix3d(int, int, int, int, int, int);
	~matrix3d();
	T &operator()(int x, int y, int z);
private:
	int xmin;
	int ymin;
	int zmin;
	int dx;
	int dy;
	int dz;
	T *entry;
};

template<class T>
struct vector {
	vector(int min, int max, int block, T init);
	vector(int min, int max, int block);
	~vector();
	T &operator()(int i);
	// inline  containr.h:53  (vector<TDepotInfo>)
	int low() { abort(); /* STUB */ }
	// inline  containr.h:54  (vector<TObjectType>)
	int high() { abort(); /* STUB */ }
	int size();
private:
	int min;
	int max;
	int start;
	int space;
	int block;
	bool initialized;
	T init;
	T *entry;
};

template<class T>
struct listnode {
	listnode<T> *next;
	listnode<T> *prev;
	T data;
};

template<class T>
struct listIterator {
	listIterator();
	// inline  containr.h:74  (listIterator<TDynamicStringTableBlock>)
	listIterator(listnode<T> * /*p*/) { abort(); /* STUB */ }
	// inline  containr.h:75  (listIterator<TDynamicStringTableBlock>)
	listnode<T> *operator()() { abort(); /* STUB */ }
	// inline  containr.h:76  (listIterator<TDynamicStringTableBlock>)
	listnode<T> *operator=(listnode<T> * /*p*/) { abort(); /* STUB */ }
	listnode<T> *operator++(int);
	listnode<T> *operator--(int);
	T *operator*();
private:
	listnode<T> *actNode;
};

template<class T>
struct list {
	list();
	~list();
	listnode<T> *append();
	void remove(listnode<T> *node);
	// inline  containr.h:89  (list<TDynamicStringTableBlock>)
	listnode<T> *first() { abort(); /* STUB */ }
	listnode<T> *last();
private:
	listnode<T> *firstNode;
	listnode<T> *lastNode;
};

template<class T>
union storeitem {
	storeitem<T> *next;
	T data;
};

template<class T, int N>
struct storeunit {
	storeitem<T> item[N];
};

template<class T, int N>
struct store {
	store();
	~store();
	T *getFreeItem();
	void deleteItem(T *Item);
private:
	list<storeunit<T, N>> *Units;
	storeitem<T> *firstFreeItem;
};

template<class T>
struct fifoIterator {
	// inline  containr.h:125  (fifoIterator<TListener>)
	fifoIterator(fifo<T> * /*_Fifo*/, int /*_Position*/) { abort(); /* STUB */ }
	// inline  containr.h:126  (fifoIterator<TListener>)
	T *operator()() { abort(); /* STUB */ }
	// inline  containr.h:127  (fifoIterator<TListener>)
	T *operator++(int) { abort(); /* STUB */ }
	// inline  containr.h:128  (fifoIterator<TStatement>)
	T *operator--(int) { abort(); /* STUB */ }
private:
	fifo<T> *Fifo;
	int Position;
};

template<class T>
struct fifo {
	fifo(int InitialSize);
	~fifo();
	// inline  containr.h:137  (fifo<TStatement>)
	fifoIterator<T> head() { abort(); /* STUB */ }
	// inline  containr.h:138  (fifo<TListener>)
	fifoIterator<T> tail() { abort(); /* STUB */ }
	T *append();
	void remove();
private:
	T *Entry;
	int Size;
	int Head;
	int Tail;
	// inline  containr.h:144  (fifo<TListener>)
	T *getEntry(int /*Position*/) { abort(); /* STUB */ }
};

template<class K, class T>
struct priority_queue_entry {
	K Key;
	T Data;
};

template<class K, class T>
struct priority_queue {
private:
	vector<priority_queue_entry<K, T>> *Entry;
	int Entries;
public:
	// inline  containr.h:601  (priority_queue<long unsigned int,TAttackWave*>)
	priority_queue(int /*Size*/, int /*Block*/) { abort(); /* STUB */ }
	// inline  containr.h:608  (priority_queue<long unsigned int,TAttackWave*>)
	~priority_queue() { abort(); /* STUB */ }
	// inline  containr.h:614  (priority_queue<long unsigned int,TAttackWave*>)
	bool getMin(K & /*Key*/, T & /*Data*/) { abort(); /* STUB */ }
	// 0x080a55e0  containr.h:622  (priority_queue<long unsigned int,long unsigned int>)
	void deleteMin() { abort(); /* STUB */ }
	// 0x080a67d0  containr.h:645  (priority_queue<long unsigned int,TAttackWave*>)
	void insert(K /*Key*/, T & /*Data*/) { abort(); /* STUB */ }
};

// 0x080a6a40  containr.h:220  (matrix<long unsigned int>)
template<class T>
matrix<T>::matrix(int /*xmin*/, int /*xmax*/, int /*ymin*/, int /*ymax*/, T /*init*/) { abort(); /* STUB */ }

// 0x08072100  containr.h:236  (matrix<int>)
template<class T>
matrix<T>::matrix(int /*xmin*/, int /*xmax*/, int /*ymin*/, int /*ymax*/) { abort(); /* STUB */ }

// inline  containr.h:251  (matrix<TShortwayPoint>)
template<class T>
matrix<T>::~matrix() { abort(); /* STUB */ }

// inline  containr.h:257  (matrix<TShortwayPoint>)
template<class T>
T &matrix<T>::operator()(int /*x*/, int /*y*/) { abort(); /* STUB */ }

// 0x080608e0  containr.h:269  (matrix3d<TSector*>)
template<class T>
matrix3d<T>::matrix3d(int /*xmin*/, int /*xmax*/, int /*ymin*/, int /*ymax*/, int /*zmin*/, int /*zmax*/, T /*init*/) { abort(); /* STUB */ }

// inline  containr.h:306  (matrix3d<TSector*>)
template<class T>
matrix3d<T>::~matrix3d() { abort(); /* STUB */ }

// inline  containr.h:312  (matrix3d<TSector*>)
template<class T>
T &matrix3d<T>::operator()(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x080a5880  containr.h:329  (vector<long unsigned int>)
// inline  containr.h:329  (vector<TCreature*>)
template<class T>
vector<T>::vector(int /*min*/, int /*max*/, int /*block*/, T /*init*/) { abort(); /* STUB */ }

// 0x08067140  containr.h:352  (vector<THouseGuest>)
// inline  containr.h:352  (vector<TChannel>)
template<class T>
vector<T>::vector(int /*min*/, int /*max*/, int /*block*/) { abort(); /* STUB */ }

// inline  containr.h:373  (vector<TAction>)
template<class T>
vector<T>::~vector() { abort(); /* STUB */ }

// 0x080561a0  containr.h:379  (vector<TObjectType>)
template<class T>
T &vector<T>::operator()(int /*i*/) { abort(); /* STUB */ }

// inline  containr.h:433  (listIterator<TDynamicStringTableBlock>)
template<class T>
listnode<T> *listIterator<T>::operator++(int) { abort(); /* STUB */ }

// inline  containr.h:453  (listIterator<TDynamicStringTableBlock>)
template<class T>
T *listIterator<T>::operator*() { abort(); /* STUB */ }

// inline  containr.h:463  (list<TDynamicStringTableBlock>)
template<class T>
list<T>::list() { abort(); /* STUB */ }

// inline  containr.h:469  (list<TDynamicStringTableBlock>)
template<class T>
list<T>::~list() { abort(); /* STUB */ }

// 0x08052b10  containr.h:474  (list<TStaticStringTableBlock>)
template<class T>
listnode<T> *list<T>::append() { abort(); /* STUB */ }

// 0x080bde40  containr.h:491  (list<storeunit<TNode, 256> >)
// inline  containr.h:491  (list<TDynamicStringTableBlock>)
template<class T>
void list<T>::remove(listnode<T> * /*node*/) { abort(); /* STUB */ }

// inline  containr.h:511  (store<TNode,256>)
template<class T, int N>
store<T, N>::store() { abort(); /* STUB */ }

// inline  containr.h:518  (store<TNode,256>)
template<class T, int N>
store<T, N>::~store() { abort(); /* STUB */ }

// 0x080afba0  containr.h:524  (store<TPlayerIndexLeafNode,100>)
template<class T, int N>
T *store<T, N>::getFreeItem() { abort(); /* STUB */ }

// inline  containr.h:538  (store<TNode,256>)
template<class T, int N>
void store<T, N>::deleteItem(T * /*Item*/) { abort(); /* STUB */ }

// inline  containr.h:551  (fifo<TListener>)
template<class T>
fifo<T>::fifo(int /*InitialSize*/) { abort(); /* STUB */ }

// inline  containr.h:564  (fifo<TListener>)
template<class T>
fifo<T>::~fifo() { abort(); /* STUB */ }

// 0x0809d4f0  containr.h:570  (fifo<TStatement>)
template<class T>
T *fifo<T>::append() { abort(); /* STUB */ }

// inline  containr.h:586  (fifo<TListener>)
template<class T>
void fifo<T>::remove() { abort(); /* STUB */ }

#endif
