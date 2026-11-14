// containr.hh: the server's container templates: matrix and matrix3d (fixed
// index ranges in two and three dimensions), vector (an index range that grows
// at either end on use), list (doubly linked), store (items handed out from
// blocks of N, with a free list), fifo (a ring buffer that doubles when full)
// and priority_queue (a binary heap, smallest key first).

#ifndef PATELLA_CONTAINR_HH
#define PATELLA_CONTAINR_HH

#include <cstdlib>
#include <cstring>

#include "utils.hh"

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
	int low() { return min; }
	int high() { return max; }
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
	listIterator(listnode<T> *p) { actNode = p; }
	listnode<T> *operator()() { return actNode; }
	listnode<T> *operator=(listnode<T> *p) { actNode = p; return actNode; }
	listnode<T> *operator++(int);
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
	listnode<T> *first() { return firstNode; }
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
	fifoIterator(fifo<T> *_Fifo, int _Position) { Fifo = _Fifo; Position = _Position; }
	T *operator()() { return Fifo->getEntry(Position); }
	T *operator++(int) { Position++; return Fifo->getEntry(Position); }
	T *operator--(int) { Position--; return Fifo->getEntry(Position); }
private:
	fifo<T> *Fifo;
	int Position;
};

template<class T>
struct fifo {
	fifo(int InitialSize);
	~fifo();
	fifoIterator<T> head() { return fifoIterator<T>(this, Head); }
	fifoIterator<T> tail() { return fifoIterator<T>(this, Tail); }
	T *append();
	void remove();
private:
	// fifoIterator calls getEntry, so it is a friend.
	friend struct fifoIterator<T>;
	T *Entry;
	int Size;
	int Head;
	int Tail;
	T *getEntry(int Position) {
		if (!in(Position, Tail, Head)) {
			return nullptr;
		}
		return &Entry[Position % Size];
	}
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
	priority_queue(int Size, int Block) {
		Entry = new vector<priority_queue_entry<K, T>>(1, Size, Block);
		Entries = 0;
	}
	~priority_queue() {
		delete Entry;
	}
	bool getMin(K &Key, T &Data) {
		if (Entries == 0) {
			return false;
		}
		Key = (*Entry)(1).Key;
		Data = (*Entry)(1).Data;
		return true;
	}
	void deleteMin() {
		if (Entries == 0) {
			error("priority_queue::deleteMin: Warteschlange ist leer.\n");
			return;
		}

		if (Entries > 1) {
			(*Entry)(1) = (*Entry)(Entries);
			int Position = 1;
			// Entries is only decremented at the end, so the stale last entry
			// still takes part in the comparisons.
			while (true) {
				int SmallerSon = Position * 2;
				if (SmallerSon > Entries) {
					break;
				}
				if (SmallerSon < Entries && (*Entry)(SmallerSon + 1).Key < (*Entry)(SmallerSon).Key) {
					SmallerSon++;
				}
				if ((*Entry)(Position).Key <= (*Entry)(SmallerSon).Key) {
					break;
				}
				priority_queue_entry<K, T> Help = (*Entry)(Position);
				(*Entry)(Position) = (*Entry)(SmallerSon);
				(*Entry)(SmallerSon) = Help;
				Position = SmallerSon;
			}
		}

		Entries--;
	}
	void insert(K Key, T &Data) {
		Entries++;
		(*Entry)(Entries).Key = Key;
		(*Entry)(Entries).Data = Data;

		int Position = Entries;
		while (Position != 1) {
			int Father = Position / 2;
			if ((*Entry)(Position).Key >= (*Entry)(Father).Key) {
				break;
			}
			priority_queue_entry<K, T> Help = (*Entry)(Position);
			(*Entry)(Position) = (*Entry)(Father);
			(*Entry)(Father) = Help;
			Position = Father;
		}
	}
};

template<class T>
matrix<T>::matrix(int xmin, int xmax, int ymin, int ymax, T init) {
	this->xmin = xmin;
	this->ymin = ymin;
	this->dx = xmax - xmin + 1;
	this->dy = ymax - ymin + 1;
	if (this->dx < 1 || this->dy < 1) {
		error("matrix: Ungueltige Feldgroesse %d..%d, %d..%d.\n", xmin, xmax, ymin, ymax);
		this->dx = 1;
		this->dy = 1;
	}

	this->entry = new T[this->dx * this->dy];
	for (int i = 0; i < this->dx * this->dy; i++) {
		this->entry[i] = init;
	}
}

template<class T>
matrix<T>::matrix(int xmin, int xmax, int ymin, int ymax) {
	this->xmin = xmin;
	this->ymin = ymin;
	this->dx = xmax - xmin + 1;
	this->dy = ymax - ymin + 1;
	if (this->dx < 1 || this->dy < 1) {
		error("matrix: Ungueltige Feldgroesse %d..%d, %d..%d.\n", xmin, xmax, ymin, ymax);
		if (this->dx < 1) {
			this->dx = 1;
		}
		if (this->dy < 1) {
			this->dy = 1;
		}
	}

	this->entry = new T[this->dx * this->dy];
}

template<class T>
matrix<T>::~matrix() {
	delete[] entry;
}

template<class T>
T &matrix<T>::operator()(int x, int y) {
	x -= xmin;
	y -= ymin;
	if (x < 0 || x >= dx || y < 0 || y >= dy) {
		error("matrix::operator(): Ungueltiger Index %d/%d.\n", x + xmin, y + ymin);
		return entry[0];
	}

	return entry[y * dx + x];
}

template<class T>
matrix3d<T>::matrix3d(int xmin, int xmax, int ymin, int ymax, int zmin, int zmax, T init) {
	this->xmin = xmin;
	this->ymin = ymin;
	this->zmin = zmin;
	this->dx = xmax - xmin + 1;
	this->dy = ymax - ymin + 1;
	this->dz = zmax - zmin + 1;
	if (this->dx < 1 || this->dy < 1 || this->dz < 1) {
		error("matrix3d: Ungueltige Feldgroesse %d..%d, %d..%d, %d..%d.\n", xmin, xmax, ymin, ymax, zmin, zmax);
		if (this->dx < 1) {
			this->dx = 1;
		}
		if (this->dy < 1) {
			this->dy = 1;
		}
		if (this->dz < 1) {
			this->dz = 1;
		}
	}

	this->entry = new T[this->dx * this->dy * this->dz];
	for (int i = 0; i < this->dx * this->dy * this->dz; i++) {
		this->entry[i] = init;
	}
}

template<class T>
matrix3d<T>::~matrix3d() {
	delete[] entry;
}

template<class T>
T &matrix3d<T>::operator()(int x, int y, int z) {
	x -= xmin;
	y -= ymin;
	z -= zmin;
	if (x < 0 || x >= dx || y < 0 || y >= dy || z < 0 || z >= dz) {
		error("matrix3d::operator(): Ungueltiger Index %d/%d/%d.\n", x + xmin, y + ymin, z + zmin);
		return entry[0];
	}

	return entry[z * dx * dy + y * dx + x];
}

template<class T>
vector<T>::vector(int min, int max, int block, T init) {
	this->min = min;
	this->max = max;
	this->start = min;
	this->space = max - min + 1;
	this->block = block;
	this->initialized = true;
	this->init = init;
	if (this->space < 1) {
		error("vector: Ungueltige Feldgroesse %d bis %d.\n", min, max);
		this->max = max;
		this->space = 1;
	}
	if (block < 0) {
		error("vector: Ungueltige Blockgroesse %d.\n", block);
		this->block = 0;
	}

	this->entry = new T[this->space];
	for (int i = 0; i < this->space; i++) {
		this->entry[i] = init;
	}
}

template<class T>
vector<T>::vector(int min, int max, int block) {
	this->min = min;
	this->max = max;
	this->start = min;
	this->space = max - min + 1;
	this->block = block;
	this->initialized = false;
	if (this->space < 1) {
		error("vector: Ungueltige Feldgroesse %d bis %d.\n", min, max);
		this->max = max;
		this->space = 1;
	}
	if (block < 0) {
		error("vector: Ungueltige Blockgroesse %d.\n", block);
		this->block = 0;
	}

	this->entry = new T[this->space];
}

template<class T>
vector<T>::~vector() {
	delete[] entry;
}

template<class T>
T &vector<T>::operator()(int i) {
	// Growth moves the old elements by swapping their raw bytes with the
	// freshly constructed ones; delete[] then destroys the fresh ones.
	while (i < start) {
		int extend = block;
		if (extend == 0) {
			extend = space;
		}

		T *help = new T[space + extend];
		for (int j = min; j <= max; j++) {
			unsigned char Help[sizeof(T)];
			memcpy(Help, &help[j - start + extend], sizeof(T));
			memcpy((void *)&help[j - start + extend], &entry[j - start], sizeof(T));
			memcpy((void *)&entry[j - start], Help, sizeof(T));
		}

		delete[] entry;
		entry = help;
		start -= extend;
		space += extend;
	}

	while (i >= start + space) {
		int extend = block;
		if (extend == 0) {
			extend = space;
		}

		T *help = new T[space + extend];
		for (int j = min; j <= max; j++) {
			unsigned char Help[sizeof(T)];
			memcpy(Help, &help[j - start], sizeof(T));
			memcpy((void *)&help[j - start], &entry[j - start], sizeof(T));
			memcpy((void *)&entry[j - start], Help, sizeof(T));
		}

		delete[] entry;
		entry = help;
		space += extend;
	}

	while (i < min) {
		min--;
		if (initialized) {
			entry[min - start] = init;
		}
	}

	while (i > max) {
		max++;
		if (initialized) {
			entry[max - start] = init;
		}
	}

	return entry[i - start];
}

// Every use assigns actNode right after the construction.
template<class T>
listIterator<T>::listIterator() {
	actNode = nullptr;
}

template<class T>
listnode<T> *listIterator<T>::operator++(int) {
	if (actNode == nullptr) {
		error("listIterator::operator++: Iterator ist NULL.\n");
		return nullptr;
	}

	actNode = actNode->next;
	return actNode;
}

template<class T>
T *listIterator<T>::operator*() {
	if (actNode == nullptr) {
		error("listIterator::operator*: Iterator ist NULL.\n");
		return nullptr;
	}

	return &actNode->data;
}

template<class T>
list<T>::list() {
	firstNode = nullptr;
	lastNode = nullptr;
}

template<class T>
list<T>::~list() {
	while (firstNode != nullptr) {
		remove(firstNode);
	}
}

template<class T>
listnode<T> *list<T>::append() {
	listnode<T> *newNode = new listnode<T>();
	newNode->prev = nullptr;
	newNode->next = nullptr;
	if (firstNode == nullptr) {
		firstNode = newNode;
	} else {
		lastNode->next = newNode;
		newNode->prev = lastNode;
	}
	lastNode = newNode;
	return lastNode;
}

template<class T>
void list<T>::remove(listnode<T> *node) {
	if (node == nullptr) {
		error("list::remove: node ist NULL.\n");
		return;
	}

	if (node->prev == nullptr) {
		firstNode = node->next;
	} else {
		node->prev->next = node->next;
	}

	if (node->next == nullptr) {
		lastNode = node->prev;
	} else {
		node->next->prev = node->prev;
	}

	delete node;
}

template<class T, int N>
store<T, N>::store() {
	Units = new list<storeunit<T, N>>;
	firstFreeItem = nullptr;
}

template<class T, int N>
store<T, N>::~store() {
	delete Units;
}

template<class T, int N>
T *store<T, N>::getFreeItem() {
	if (firstFreeItem == nullptr) {
		listIterator<storeunit<T, N>> iter;
		iter = Units->append();
		for (int i = 0; i < N - 1; i++) {
			(*iter)->item[i].next = &(*iter)->item[i + 1];
		}
		(*iter)->item[N - 1].next = nullptr;
		firstFreeItem = (*iter)->item;
	}

	T *help = &firstFreeItem->data;
	firstFreeItem = firstFreeItem->next;
	return help;
}

template<class T, int N>
void store<T, N>::deleteItem(T *Item) {
	storeitem<T> *help = (storeitem<T> *)Item;
	help->next = firstFreeItem;
	firstFreeItem = help;
}

template<class T>
fifo<T>::fifo(int InitialSize) {
	Entry = new T[InitialSize];
	Size = InitialSize;
	Head = -1;
	Tail = 0;
}

template<class T>
fifo<T>::~fifo() {
	delete[] Entry;
}

template<class T>
T *fifo<T>::append() {
	if (Head - Tail + 1 == Size) {
		int NewSize = Size * 2;
		T *NewEntry = new T[NewSize];
		for (int i = Tail; i <= Head; i++) {
			NewEntry[i % NewSize] = Entry[i % Size];
		}
		delete[] Entry;
		Entry = NewEntry;
		Size = NewSize;
	}

	Head++;
	return &Entry[Head % Size];
}

template<class T>
void fifo<T>::remove() {
	if (Tail > Head) {
		error("fifo::remove: Fifo ist leer.\n");
		return;
	}

	Tail++;
}

#endif
