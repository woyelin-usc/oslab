#include <iostream>
#include <fstream>
#include <sstream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <list>
#include <iomanip>

using namespace std;

bool vflag = 0;
char algo = 'i';
stringstream ss;

#define traceQ( ss )  do { if (vflag)  cout << ss.str() << endl; ss.clear(); ss.str(""); } while(0)

struct IO {
	unsigned idx;
	int startTime, startTrack, finishTime, finishTrack, issueTime;
	IO(unsigned idx, int startTime, int finishTrack) { 
		this->idx = idx; this->startTime = startTime; this->finishTrack = finishTrack; 
	}
};

struct Stats {
	int total_time, total_movement, max_waittime;
	double avg_turnaround, avg_waittime;
	Stats() { total_time = total_movement = max_waittime = 0; avg_turnaround = avg_waittime = 0; }
};

class IOSched {
public:
	IOSched() { active = new list<IO*>(); wait = new list<IO*>(); curIO = NULL; curTime = 0; curTrack = 0; curDirection = 1; }
	virtual ~IOSched() { delete active; delete wait; }
	virtual IO* getIO() = 0;
	virtual void addIO( IO *io ) = 0;
	virtual void rmIO() = 0;
	void swap() { 
		list<IO*> *tmp = active;
		active = wait;
		wait = tmp;
		curDirection = 1;
	}

	list<IO*> *active, *wait;
	IO *curIO;
	int curTime, curTrack, curDirection;
};

class FIFO: public IOSched {
public:
	FIFO(): IOSched() {}
	// back() is the oldest; front() is the newest
	IO* getIO() {
		return active -> back();
	}
	void addIO(IO* io) {
		active -> push_front(io);
	}

	void rmIO() {
		active -> pop_back();
		curIO = NULL;
	}
};

class SSTF: public IOSched {
public:
	SSTF(): IOSched() {}
	IO* getIO() {
		list<IO*>::iterator target = active->begin(), it = (active->begin())++;
		while(it!=active->end()) {
			if( abs((*it)->finishTrack - curTrack) < abs((*target)->finishTrack - curTrack)  ) {
				target = it;
			}
			it++;
		}
		IO *io = *target;
		active->erase(target);
		active->push_front(io);
		return active->front();
	}

	// new request: push it in the back
	void addIO(IO* io) { 
		active->push_back(io); 
	}

	// remove front() which is the oldest
	void rmIO() { 
		active->pop_front(); 
		curIO = NULL;
	}
};

class SCAN: public IOSched {
public:
	SCAN(): IOSched() {}
	IO* getIO() {
		IO *io = NULL;
		while ( true) {
			// curDirection 1: up    
			if( curDirection ) {
				for( list<IO*>::iterator it = active->begin(); it != active->end(); ++it ) {
					if( (*it)->finishTrack >= curTrack ) {
						io = *it;
						break;
					}
				}
				// if there is no request's track larger , cannot go up, change direction to down and loop one more time
				if ( io == NULL ) curDirection = 0;
				else break;
			}

			// curDirection 0: down
			else {
				for( list<IO*>::reverse_iterator it = active->rbegin(); it != active->rend(); ++it ) {
					if( (*it)->finishTrack < curTrack ) {
						int target = (*it)->finishTrack;
						++it;
						while( it != active->rend() ) {
							if( (*it)->finishTrack == target ) ++it;
							else break;
						}
						--it;
						io = *it;
						break;
					}

					else if( (*it)->finishTrack == curTrack ) {
						++it;
						while( it != active->rend() ) {
							if( (*it)->finishTrack < curTrack ) {
								--it;
								io = *it;
								break;
							}
							else ++it;
						}
						if ( io == NULL ) io = active->front(); 
						break; 
					}
				}

				// if all io_requests's track are larger, then change direction to up
				if( io == NULL ) curDirection = 1;
				else break;
			}	
		}
		return io;
	}


	// maintain a order from smallest to biggest
	void addIO(IO* io) {
		// reverse backward so satisfy FIFO when two io requests have same destination track
		list<IO*>::reverse_iterator it;
		for( it = active->rbegin(); it!=active->rend(); ++it) {
			if( (*it)->finishTrack <= io->finishTrack )  {
				active->insert( it.base(), io );
				break;
			}
		}
		// if all requests's track are larger, insert new io to the front
		if( it == active->rend() ) active->push_front(io);
	}

	void rmIO() {
		for( list<IO*>::iterator it = active->begin(); it!=active->end(); ++it ) {
			if( *it == curIO ) {
				active->erase(it);
				curIO = NULL;
				break;
			}
		}
	}
};

class CSCAN: public IOSched {
public:
	CSCAN(): IOSched() {}
	IO* getIO() {
		IO *io = NULL;
		// first scan upwards
		for( list<IO*>::iterator it = active->begin(); it!=active->end(); ++it ) {
			if( (*it)->finishTrack >= curTrack ) {
				io = *it;
				break;
			}
		}
		// if all requests are smaller, wrap back to smallest and go again
		if( io == NULL ) io = active->front();
		return io;
	}

	// maintain a order from smallest to biggest
	void addIO(IO* io) {
		// reverse backward so satisfy FIFO when two io requests have same destination track
		list<IO*>::reverse_iterator it;
		for ( it = active->rbegin(); it != active->rend(); ++it  ) {
			if ( (*it)->finishTrack <= io->finishTrack ) {
				active->insert ( it.base(), io );
				break;
			} 
		}
		if ( it == active->rend() ) active->push_front( io) ;
	}

	void rmIO() {
		for ( list<IO*>::iterator it = active->begin(); it != active->end(); ++it ) {
			if ( *it == curIO ) {
				active->erase( it);
				curIO = NULL;
				break;
			}
		}
	}
};

class FSCAN: public IOSched {
public:
	FSCAN(): IOSched() {}
	IO* getIO() {
		if( active->empty() ) swap();
		IO *io = NULL;
		while ( true) {
			// curDirection 1: up    
			if( curDirection ) {
				for( list<IO*>::iterator it = active->begin(); it != active->end(); ++it ) {
					if( (*it)->finishTrack >= curTrack ) {
						io = *it;
						break;
					}
				}

				
				// if there is no request's track larger , cannot go up, change direction to down and loop one more time
				if ( io == NULL )  curDirection = 0; 
				else break;
			}

			// curDirection 0: down
			else {
				for( list<IO*>::reverse_iterator it = active->rbegin(); it != active->rend(); ++it ) {
					if( (*it)->finishTrack < curTrack ) {
						int target = (*it)->finishTrack;
						++it;
						while( it != active->rend() ) {
							if( (*it)->finishTrack == target ) ++it;
							else break;
						}
						--it;
						io = *it;
						break;
					}

					else if( (*it)->finishTrack == curTrack ) {
						++it;
						while( it != active->rend() ) {
							if( (*it)->finishTrack < curTrack ) {
								--it;
								io = *it;
								break;
							}
							else ++it;
						}
						if ( io == NULL ) io = active->front(); 
						break; 
					}
				}

				// if all io_requests's track are larger, then change direction to up
				if( io == NULL ) curDirection = 1;
				else break;
			}	
		}
		return io;
	}

	// add new io into the wait list and also main order from smallest to largest
	void addIO(IO* io) {
		// reverse backward so satisfy FIFO when two io requests have same destination track
		list<IO*>::reverse_iterator it;
		for ( it = wait->rbegin(); it != wait->rend(); ++it  ) {
			if ( (*it)->finishTrack <= io->finishTrack ) {
				wait->insert ( it.base(), io );
				break;
			} 
		}
		if ( it == wait->rend() ) wait->push_front( io ) ;
	}

	void rmIO() {
		for ( list<IO*>::iterator it = active->begin(); it != active->end(); ++it ) {
			if ( *it == curIO ) {
				active->erase( it);
				curIO = NULL;
				break;
			}
		}
		if( active->empty() ) swap();
	}
};

void read( ifstream& ifile, list<IO*>& requests ) {
	unsigned idx = 0;
	while( !ifile.eof() ) {
		string tmp;
		getline(ifile, tmp);
		if( !tmp.length() || tmp[0]=='#') continue;
		else {
			stringstream ss(tmp);
			int startTime, finishTrack;
			ss >> startTime; ss >> finishTrack;
			requests.push_back(new IO(idx++, startTime, finishTrack));
		}
	}
}

void add( IOSched* sched, list<IO*>::iterator &it ) {
	sched->addIO(*it);
	sched->curTime = (*it)->startTime;
	// seems that update current track is unecessary, will see later
	//sched->curTrack = (sched->curTime - sched->curIO->issueTime) + ...
	ss << sched->curTime << ":" << setw(6) << right << (*it)->idx << " add " << (*it)->finishTrack; traceQ(ss);
	++it;	
}

void issue(IOSched* sched, Stats & stats) {
	sched->curIO = sched->getIO();
	sched->curIO->issueTime = sched->curTime;
	sched->curIO->startTrack = sched->curTrack;
	sched->curIO->finishTime = sched->curTime + abs( sched->curIO->finishTrack - sched->curTrack );

	// count the track movement
	stats.avg_waittime += sched->curIO->issueTime - sched->curIO->startTime;
	stats.max_waittime = (stats.max_waittime >= sched->curIO->issueTime - sched->curIO->startTime)? stats.max_waittime: sched->curIO->issueTime - sched->curIO->startTime;
	ss << sched->curTime << ":" << setw(6) << right << sched->curIO->idx << " issue " << sched->curIO->finishTrack << " " << sched->curTrack; traceQ(ss);
}

void finish( IOSched* sched, Stats & stats ) {
	sched->curTime = sched->curIO->finishTime;
	sched->curTrack = sched->curIO->finishTrack;

	// do statistics
	stats.total_movement += abs( sched->curIO->finishTrack - sched->curIO->startTrack );
	stats.total_time = sched->curTime;
	stats.avg_turnaround += sched->curIO->finishTime - sched->curIO->startTime;
	ss << sched->curTime << ":" << setw(6) << right << sched->curIO->idx << " finish " << sched->curIO->finishTime - sched->curIO->startTime;  traceQ( ss );

	// shows that io system is free && remove that io request from io_queue
	sched->rmIO();
	// if active list is empty, swap active and wait list, and update active in the scheduler
	// already done above in the scheduler's class (remove function)
}


void simulate ( list<IO*>& requests,  Stats& stats, IOSched * sched ) {
	if(vflag) cout << "TRACE" << endl;
	list<IO*>::iterator it = requests.begin();
	while( it != requests.end() ) {

		// if the io system is free,
		if ( sched->curIO == NULL ) {
			// if active list is empty, read in sth
			if( sched->active->empty() )  add(sched, it);

			// here we make sure that there is sth in the active list to serve, so issue one to serve
			issue( sched, stats);
		}

		// if io is not free && next io request starttime < = targetTime, keep reading in the wait list
		else if ( (*it)->startTime <= sched->curIO->finishTime ) add(sched, it); 

		// if io is not free && next io request starttime > targetTime, should finish an io request from active list
		else if ( (*it)->startTime > sched->curIO->finishTime ) finish( sched, stats );
		else;
	}

	// here we have finish reading all io_requests, so first process active list then process wait list
	while( !sched->active->empty() ) {
		// here io is not free, finish it
		if( sched->curIO != NULL ) finish( sched , stats) ; 

		// here io is free, pick one to issue
		else issue(sched, stats ) ;
	}

	// work on remaining wait list
	sched->swap();

	while( !sched->active->empty() ) {
		if( sched->curIO != NULL ) finish( sched, stats);
		else issue(sched, stats ) ;
	}
}


int main(int argc, char** argv) {
	if( argc==1 ) { cerr << "Not a valid inputfile <(null)>" << endl; return 1; }
	int c;

	while ((c = getopt (argc, argv, "vs:")) != -1) {
		switch(c) {
			case 'v': vflag = 1;  break;
			case 's': 
				algo = optarg[0]; 
				if( algo != 'i' && algo != 'j' && algo != 's' && algo != 'c' && algo != 'f' ) 
					{ cerr << "Unknown Scheduler spec: -s {ijscf}" << endl; return 1; }
				break;
			case '?':
				if (optopt == 's') fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt)) fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return 1;
			default: abort ();
		}
	}

	// first read in all io_requests
	ifstream ifile( argv[optind] );
	if(ifile.fail()) { cerr << "Not a valid inputfile <" << argv[optind] <<  ">" << endl; return 1;}
	list<IO*> requests;
	read( ifile, requests );
	ifile.close();
	if( !requests.size() ) { printf("SUM: %d %d -nan -nan %d\n", 0, 0, 0); return 0; }

	Stats stats; 
	list<IO*> l1, l2;

	// create scheduler
	IOSched * sched = NULL;
	if ( algo == 'i') sched = new FIFO();
	else if ( algo == 'j' ) sched = new SSTF();
	else if ( algo == 's' ) sched = new SCAN();
	else if ( algo == 'c' ) sched = new CSCAN();
	else if ( algo == 'f' ) sched = new FSCAN();
	else { cout << "Unknown Scheduler spec: -s {ijscf}" << endl; return 1; }

	// start simulation
	simulate( requests, stats, sched );

	// calculate statistics
	stats.avg_turnaround /= requests.size();
	stats.avg_waittime /= requests.size();
	// print summary
	printf("SUM: %d %d %.2lf %.2lf %d\n", stats.total_time, stats.total_movement, stats.avg_turnaround, stats.avg_waittime, stats.max_waittime);

	// clean memory
	delete sched;	
	for(auto it: requests) delete it;
	
	return 0;
}