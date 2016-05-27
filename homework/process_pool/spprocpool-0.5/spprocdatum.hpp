/*
 * Copyright 2007 Stephen Liu
 * For license terms, see the file COPYING along with this library.
 */

#ifndef __spprocdatum_hpp__
#define __spprocdatum_hpp__

#include <unistd.h>
#include <pthread.h>
#include <sys/poll.h>

class SP_ProcPool;
class SP_ProcManager;
class SP_ProcDatumServiceFactory;
class SP_ProcDataBlock;
class SP_ProcInfo;

class SP_ProcDatumHandler {
public:
	virtual ~SP_ProcDatumHandler();
	virtual void onReply( pid_t pid, const SP_ProcDataBlock * reply ) = 0;
	virtual void onError( pid_t pid ) = 0;
};

class SP_ProcInfoListEx;

class SP_ProcDatumDispatcher {
public:
	SP_ProcDatumDispatcher( SP_ProcDatumServiceFactory * factory,
			SP_ProcDatumHandler * handler );
	~SP_ProcDatumDispatcher();

	// get the proc pool object to set parameters
	SP_ProcPool * getProcPool();

	// default is 128
	void setMaxProc( int maxProc );

	// > 0 : Success, < 0 : fail, reach MaxProc limit or cannot get a process
	pid_t dispatch( const void * request, size_t len );

	void dump() const;

private:
	// manager side
	SP_ProcManager * mManager;

	// app side
	SP_ProcPool * mPool;
	SP_ProcDatumHandler * mHandler;

	int mIsStop;
	pthread_mutex_t mMutex;
	pthread_cond_t mCond;

	SP_ProcInfoListEx * mBusyList;

	int mMaxProc;

	static void * checkReply( void * );
};

//-------------------------------------------------------------------

class SP_ProcDatumService {
public:
	virtual ~SP_ProcDatumService();
	virtual void handle( const SP_ProcDataBlock * request, SP_ProcDataBlock * reply ) = 0;
};

class SP_ProcDatumServiceFactory {
public:
	virtual ~SP_ProcDatumServiceFactory();

	virtual SP_ProcDatumService * create() const = 0;

	virtual void workerInit( const SP_ProcInfo * procInfo );

	virtual void workerEnd( const SP_ProcInfo * procInfo );
};

#endif

