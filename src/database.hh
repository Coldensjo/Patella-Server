#ifndef PATELLA_DATABASE_HH
#define PATELLA_DATABASE_HH

#include "threads.hh"

struct TDatabaseConnection;

struct TDatabaseSettings {
	char Product[30];
	char Database[30];
	char Login[30];
	char Password[30];
	char Host[30];
	char Port[6];
};

struct TPreparedQuery {
	int Number;
	const char *Query;
	const char *DBTypes;
	const char *CTypes;
	const char *Name;
};

struct TDatabaseConnectionPool {
	TDatabaseConnectionPool(int);
	void init(TDatabaseSettings &);
	void exit();
	TDatabaseConnection *getConnection();
	void releaseConnection(TDatabaseConnection *);
private:
	int NumberOfConnections;
	TDatabaseSettings DatabaseSettings;
	TDatabaseConnection **DatabaseConnection;
	bool *DatabaseConnectionFree;
	Semaphore FreeDatabaseConnections;
	Semaphore DatabaseConnectionMutex;
};

struct TDatabasePoolConnection {
	TDatabasePoolConnection(TDatabaseConnectionPool *);
	~TDatabasePoolConnection();
	TDatabaseConnection *operator->();
	void beginTransaction();
	void abortTransaction();
	void endTransaction();
	void release();
	void reconnect();
private:
	TDatabaseConnectionPool *DatabaseConnectionPool;
	TDatabaseConnection *DatabaseConnection;
	bool TransactionRunning;
};

#endif
