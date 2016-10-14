#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <queue>
#include <stack>
#include <vector>
#include <iomanip>

using namespace std;

typedef int stime_t;

bool vflag = false;
int curTime = 0;
int maxfintime = 0;
int cpuTotalTime = 0;
int ioTotalTime = 0;
int lastiofintime=0;
int turnaroundTotalTime = 0;
int cpuwaitTotalTime = 0;

struct Process 
{
	int idx, at, tc, cb, io, remcb, staPrio, dynPrio, arrivalTime=0, totalCPU=0, ft=-1, it=0, cw=0;
	Process() {}
	Process(int idx, int at, int tc, int cb, int io,int remcb, int staPrio, int dynPrio) { this->idx=idx; this->at=at; this->tc=tc; this->cb=cb; this->io=io; this->remcb=remcb; this->staPrio=staPrio; this->dynPrio=dynPrio; }
};

Process* curProcess=NULL;
vector<Process*> allProcesses;

struct Event 
{
	int timeStamp, createdTime;
	Process* p;
	string oldMode, newMode;
	int oldTime, newTime;
	Event(int timeStamp, Process* p, string oldMode, int oldTime, string newMode, int newTime, int createdTime) 
	{ this->timeStamp=timeStamp; this->p=p; this->oldMode=oldMode; this->oldTime=oldTime; 
	  this->newMode=newMode;  this->newTime=newTime;  this->createdTime = createdTime;   }
};

class Scheduler 
{
public:
	int _quantum;
	string _name;
	vector<Process*> *readyQ = new vector<Process*>(), *readyQ2 = new vector<Process*>();
	virtual void addProcess(Process* p)=0;
	virtual Process* getProcess()=0;
	virtual ~Scheduler() {}
};

class FCFS: public Scheduler 
{
public:
	FCFS(string name) { this->_name = name; this->_quantum=0; }
	~FCFS() {delete readyQ; delete readyQ2; }
	void addProcess(Process* p) { readyQ->push_back(p);  }
	Process* getProcess() { 
		if(readyQ->empty()) return NULL;
		else { Process* theProcess=readyQ->front(); readyQ->erase(readyQ->begin()); return theProcess; }
	}
};

class LCFS: public Scheduler
{
public:
	LCFS(string name) { this->_name = name; this->_quantum=0;}
	~LCFS() { delete readyQ; delete readyQ2; }
	void addProcess(Process* p) { readyQ->push_back(p); } 
	Process* getProcess() {
		if(readyQ->empty()) return NULL;
		else { Process* theProcess=readyQ->back(); readyQ->pop_back(); return theProcess; } 
	}
};

class SJF: public Scheduler
{
public:
	SJF(string name) { this->_name = name; this->_quantum=0; }
	~SJF() { delete readyQ; delete readyQ2; }
	// ordered based on shortest remaining time (tc)
	void addProcess(Process* p) { 
		int l=0, r=readyQ->size();
		while(l<r) {
			int m=l+(r-l)/2;
			if((*readyQ)[m]->tc==p->tc) { 

				// assume processes with same remaining time inserted into runqueue based on FCFS
				// so first move new process to far left
				while(m>0 && readyQ->at(m-1)->tc==p->tc) m--;
				readyQ->insert(readyQ->begin()+m, p);

				// this is the old method: Process with same rem time inserted into runqueue based on idx
				// int idx=m;
				// while((unsigned)idx>0 && p->tc==readyQ->at(idx-1)->tc && readyQ->at(idx-1)->idx < p->idx ) idx--;
				// while((unsigned)idx<readyQ->size() && p->tc==readyQ->at(idx)->tc &&  readyQ->at(idx)->idx > p->idx) idx++;
				// readyQ->insert(readyQ->begin()+idx, p);
				
				return;
			}
			else if((*readyQ)[m]->tc>p->tc) l=m+1;
			else r=m;
		}
		readyQ->insert(readyQ->begin()+l, p);
	}
	Process* getProcess() { 
		if(readyQ->empty()) return NULL;
		else { Process* theProcess=readyQ->back(); readyQ->pop_back(); return theProcess; }
	}
};

class RR: public Scheduler
{
public:
	RR(string name, int quantum)  { this->_name = name; this->_quantum=quantum; }  
	~RR() { delete readyQ; delete readyQ2; }
	void addProcess(Process* p) {
		readyQ->insert(readyQ->begin(), p);
	}
	Process* getProcess() {
		if(readyQ->empty()) return NULL;
		else { Process* theProcess=readyQ->back(); readyQ->pop_back(); return theProcess; }
	}
};

class PRIO: public Scheduler
{
public:
	PRIO(string name, int quantum) { this->_name = name; this->_quantum=quantum; }
	~PRIO() { delete readyQ; delete readyQ2; }
	void addProcess(Process* p) {
		// if p's dynPrio is -1, reset to its staPrio-1 and add to the readyQ2
		if(p->dynPrio==-1)  {
			p->dynPrio=p->staPrio-1;
			int l=0, r=readyQ2->size();
			while(l<r) {
				int m=l+(r-l)/2;
				if((*readyQ2)[m]->dynPrio==p->dynPrio) {
					// right: smaller idx   left: bigger idx
					// go to far left of the same dynPrio (same priority: round robin scheduling)
					while(m>0 && (*readyQ2)[m-1]->dynPrio==p->dynPrio ) m--;
					readyQ2->insert(readyQ2->begin()+m, p);
					break;
				}
				else if((*readyQ2)[m]->dynPrio>p->dynPrio) r=m;
				else l=m+1;
			}
			if(l==r) readyQ2->insert(readyQ2->begin()+l, p);

			// if readyQ is empty now, switch readyQ and readyQ2
			if(readyQ->empty()) {
				vector<Process*> *tmp = readyQ;
				readyQ = readyQ2;
				readyQ2 = tmp;
			}
		}

		// if P's dynPrio hasn't reach -1, just insert into the readyQ
		else {
			int l=0, r=readyQ->size();
			while(l<r) {
				int m=l+(r-l)/2;
				if((*readyQ)[m]->dynPrio==p->dynPrio) {
					// right: smaller idx   left: bigger idx
					// go to far left of the same dynPrio
					while(m>0 && (*readyQ)[m-1]->dynPrio==p->dynPrio ) m--;
					readyQ->insert(readyQ->begin()+m, p);
					break;
				}	
				else if((*readyQ)[m]->dynPrio>p->dynPrio) r=m;
				else l=m+1;
			}
			if(l==r) readyQ->insert(readyQ->begin()+l, p);

		}
	}

	Process* getProcess() {
		if(readyQ->empty()) {
			if(readyQ2->empty()) return NULL;
			else {
				vector<Process*> *tmp = readyQ;
				readyQ = readyQ2;
				readyQ2 = tmp;

			}
		}
		Process* theProcess=readyQ->back(); readyQ->pop_back(); return theProcess; 
	}
};

int getRanNum(ifstream& rfile) {
	string tmp;
	if(rfile.eof()) {
		rfile.clear();
		rfile.seekg(0);
		getline(rfile, tmp);
	}
	getline(rfile, tmp);
	stringstream ss;
	ss<<tmp;
	int num;
	ss>>num;
	return num;
}

int myRandom(int burst, ifstream& rfile)
{
	return 1 + (getRanNum(rfile) % burst); 
}


// put event into eventQ order based on the timestamp (earlier=right, later=left)
void putEvent(vector<Event*>& eventQ, Event *newEvent)
{
	int l=0, r=eventQ.size();
	while(l<r) {
		int m=l+(r-l)/2;
		if(eventQ[m]->timeStamp==newEvent->timeStamp) { 

			// because event with same timestamp is ordered based on time generated, so first move new event the far left then right move gradually
			while(m>0 && eventQ[m-1]->timeStamp==newEvent->timeStamp) m--;

			// Now eventQ[m] is the far left event with same timestamp, gradually right move to find the limit
			while((unsigned)m<eventQ.size() && eventQ[m]->timeStamp==newEvent->timeStamp) {
				if(eventQ[m]->oldMode=="CREATED") break;
				else if(eventQ[m]->createdTime > newEvent->createdTime ) m++;
				else break;
			}
			// now insert before eventQ[m]
			eventQ.insert(eventQ.begin()+m, newEvent);
			return;
		}
		else if(eventQ[m]->timeStamp>newEvent->timeStamp) l=m+1;
		else r=m;
	}
	eventQ.insert(eventQ.begin()+l, newEvent);  
}


void start(ifstream& ifile, vector<Event*>& eventQ, ifstream& rfile)
{
	string line, word;
	int idx=0;
	while(!ifile.eof()) {
	        getline(ifile, line);
		if(!line.size()) continue;
		// call random function to assign static priority(1-4) to each process
		int staPrio = myRandom(4, rfile);
		stringstream ss(line);
		ss>>word; int at=stoi(word); 
		ss>>word; int tc=stoi(word);
		ss>>word; int cb=stoi(word);
		ss>>word; int io=stoi(word);
		Process *p = new Process(idx, at, tc, cb, io, 0, staPrio, staPrio-1);
		p->arrivalTime = at; p->totalCPU=tc;
		allProcesses.push_back(p);

		// new event created from "CREATED" state to "READY" state
		// change created old time to -1 so that same stamp, created alwasy take precedence (to be verified!)
		Event *e = new Event(at, p, "CREATED", at, "READY", at, at);
		eventQ.insert(eventQ.begin(), e);
		idx++;
	}
}

bool isRR(string schealg)
{
	if(schealg[0]!='R') return false;
	for(unsigned int i=1; i<schealg.size();i++) if(!isdigit(schealg[i])) return false;
	return true;
}

bool isPRIO(string schealg)
{
	if(schealg[0]!='P') return false;
	for(unsigned int i=1; i<schealg.size();i++) if(!isdigit(schealg[i])) return false;
	return true;
}


void simulation(ifstream& rfile, Scheduler *sched, vector<Event*>& eventQ)
{
	while(!eventQ.empty()) {

		Event* e = eventQ.back(); eventQ.pop_back();
		curTime = e->timeStamp;
		
		if(e->newMode=="READY") {

			if(vflag /*&& e->oldMode!="CREATED"*/) {
				// 1 0 0: CREATED -> READY ||  76 2 57: BLOCK -> READY
				cout<<curTime<<" "<<e->p->idx<<" "<<curTime-e->oldTime<<": "
				<<e->oldMode<<" -> "<<e->newMode; 
			}

			if(e->oldMode=="CREATED") { if(vflag) cout<<endl; }
			else if(e->oldMode=="BLOCK") {
				e->p->dynPrio=e->p->staPrio-1;
				if(vflag) cout<<endl;

				// accumulate this process's block time
				e->p->it += curTime-e->oldTime;
			}
			else if(e->oldMode=="RUNNG") {
				if(vflag) {
					cout<<"  cb="<<e->p->remcb<<" rem="<<e->p->tc<<" prio=";
					if(sched->_name=="RR") cout<<e->p->staPrio-1<<endl;
					else cout<<e->p->dynPrio<<endl;
				}
				// accumulate cpuRunning time (at least one processing is running)
				cpuTotalTime += curTime - e->oldTime;
				
				curProcess=NULL;
				e->p->dynPrio = e->p->dynPrio-1;
				// if reach -1, reset to static priority here if it's RR scheduling; reset in scheduler when it's PRIO scheduler
				if(e->p->dynPrio==-1 && sched->_name=="RR") e->p->dynPrio=e->p->staPrio;
			}
			else;

			// Since it's a ready event,  put that process into readyQ in the scheduler
			sched->addProcess(e->p); e->p->at=curTime;
			

			// if next event exits and has same timestamp, "continue" to fetch that next event
			if(!eventQ.empty() && eventQ.back()->timeStamp==curTime) continue;

			// here either eventQ is empty, or next event's timestamp is greater than curTime, so process scheduler's readyQ
			// if the scheduler is not running any process, get next process to run
			if(!curProcess) {
				Process *processToRun = sched->getProcess();
				// if readyQ is currently empty, nothing to run, CPU is idle
				if(!processToRun) continue;
				// otherwise, create a run event and put that event into eventQ
				else {
					Event* newEvent = new Event(curTime, processToRun,"READY", processToRun->at, "RUNNG", curTime, curTime);
					putEvent(eventQ, newEvent);
					curProcess=processToRun;
					//eventQ.insert(eventQ.begin(), newEvent);
					//putEvent(eventQ, newEvent);
				}
			}
		}

		else if(e->newMode=="RUNNG") {
			curProcess=e->p;

			// accumulate curProcess's cpu waiting time
			curProcess->cw += curTime - e->oldTime;

			// compute cpu burst
			int cbTime;
			// if already has a remaining cpu burst time, use that and don't calculate new cpu burst time
			if(curProcess->remcb) {  cbTime = curProcess->remcb; }
			else { cbTime = myRandom(curProcess->cb, rfile); }

			// if cpu burst is larger than process remaining time, reduce it to the remaining time
			if(cbTime>curProcess->tc) { cbTime = curProcess->tc; }

			if(vflag) {
				//10 2 9: READY -> RUNNG cb=9 rem=20 prio=1
				cout<<curTime<<" "<<curProcess->idx<<" "<<curTime-e->oldTime<<": "<<e->oldMode<<" -> "<<e->newMode
				<<" cb="<<cbTime<<" rem="<<curProcess->tc<<" prio=";
				if(sched->_name=="RR") cout<<curProcess->staPrio-1<<endl;
				else cout<<curProcess->dynPrio<<endl;
			}

			// if we don't need to consider quantum cpu burst ("FCFS, LCFS, SJS" or quantum>=cbTime)
			if( (sched->_name!="RR" && sched->_name!="PRIO") || sched->_quantum>=cbTime) {
				// reduce the process's remaining CPU time as already runned
				curProcess->tc -= cbTime;

				curProcess->remcb = 0;

				// at the endTime(curTime+cbTime), there will be an event moving current process from RUNNG to BLOCK. create that event and put it into eventQ
				Event *newEvent = new Event(curTime+cbTime, curProcess, "RUNNG", curTime, "BLOCK", curTime+cbTime, curTime);
				putEvent(eventQ, newEvent);
			}

			// quantum time is smaller than cpu burst time, put that into the READY state with remaining cpu burst time
			// here quantum will expire once so decrease the process's dynamic priority
			else {
				curProcess->tc -= sched->_quantum;
				curProcess->remcb=cbTime-sched->_quantum;
				//sched->addProcess(curProcess);
				
				// at the endTime(curTime++quantum), there will be an event moving current process from RUNNING to READY
				Event *newEvent = new Event(curTime+sched->_quantum, curProcess, "RUNNG", curTime, "READY", curTime+sched->_quantum, curTime);
				putEvent(eventQ, newEvent);
				//curProcess=NULL;
			}
			
		}

		else if(e->newMode=="BLOCK") {

			// if the process already completed all CPU time, print out done message
			if(!curProcess->tc) {
				if(vflag) {
					// 201 0 5: Done
					cout<<curTime<<" "<<curProcess->idx<<" "<<curTime-e->oldTime<<": Done"<<endl;
				}
				curProcess->ft = curTime; 
				maxfintime=curTime;
			}

			else {
				// compute io burst  [to verity: here curProcess == e->p]
				int ioTime = myRandom(curProcess->io, rfile);
				if(vflag) {
					//3 0 2: RUNNG -> BLOCK  ib=80 rem=18
					cout<<curTime<<" "<<e->p->idx<<" "<<curTime-e->oldTime<<": "
					<<e->oldMode<<" -> "<<e->newMode<<"  ib="<<ioTime<<" rem="<<e->p->tc<<endl;
				}
				int endTime = curTime + ioTime;
				// at the endTime, move from BLOCK to READY, create that event
				Event *newEvent = new Event(endTime, e->p, "BLOCK", curTime, "READY", endTime, curTime);
				putEvent(eventQ, newEvent);


				// accumulate iototaltime
				if(curTime >= lastiofintime) ioTotalTime += ioTime;
				else if(curTime <lastiofintime && endTime > lastiofintime) ioTotalTime += endTime - lastiofintime; 
				else;
				lastiofintime = max(lastiofintime, curTime + ioTime);
			}

			// accumulate cpuTotalTime
			cpuTotalTime += curTime - e->oldTime;

			// free CPU and get next process to run
			curProcess=NULL;
			Process* processToRun = sched->getProcess();
			// if readyQ is empty, CPU idle
			if(!processToRun) { /*cout<<"DEBUG: "<<endl;*/ continue; }
			// else create that running event
			else {
				Event* newEvent = new Event(curTime, processToRun, "READY", processToRun->at, "RUNNG", curTime, curTime);
				putEvent(eventQ, newEvent);
				curProcess=processToRun;
			}
		}
		else;
		// clean memory (event pointer)
		delete e;
	}
}

void printInfo(int quantum, Scheduler *sched)
{
	// print out scheduler's info
	cout<<sched->_name;
	if(sched->_name=="PRIO"||sched->_name=="RR") cout<<" "<<quantum;
	cout<<endl;

	// print each process's information
	for(unsigned int i=0;i<allProcesses.size();i++) {
		Process *p = allProcesses[i];
		int 	id = p->idx;       // unique id for this process
		stime_t arrival =p->arrivalTime;      // specified arrival time of process
		stime_t totaltime = p->totalCPU;  // specified total exec time of process
		stime_t cpuburst = p->cb;     // specified cpuburst of process
		stime_t ioburst = p->io;      // specified ioburst of process
		int static_prio = p->staPrio ;  // static priority of the process, 0 if not prio scheduler

		stime_t state_ts = p->ft;   // time we entered into this state (finish time)
		stime_t iowaittime = p->it; // total iowaittime of process during sim
		stime_t cpuwaittime= p->cw; // time we were ready to run but did not
		
		printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
		       	id, arrival, totaltime, cpuburst, ioburst, static_prio,
	       		state_ts, state_ts - arrival, iowaittime,  cpuwaittime    );

		turnaroundTotalTime += state_ts - arrival;
		cpuwaitTotalTime += p->cw;
	}

	// print summary

	// compute the following variables based on the simulation and the final state of all the processes 
	//int    maxfintime = ;
	double cpu_util = (double) (cpuTotalTime) * 100 / maxfintime;
	double io_util  = (double) (ioTotalTime) * 100 / maxfintime;
	double avg_turnaround = (double) (turnaroundTotalTime) / allProcesses.size();
	double avg_waittime = (double) (cpuwaitTotalTime) / allProcesses.size();
	double throughput = (double) allProcesses.size() * 100 / maxfintime;
	
	printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n",
	       maxfintime,
	       cpu_util,
	       io_util,
	       avg_turnaround,
	       avg_waittime, 
	       throughput);

}

int main(int argc, char** argv)
{ 
	string schealg="", inputFile, randomFile;
	int c;
	
	while( (c=getopt(argc, argv, "s:v"))!=-1 ) {
		switch(c) {
			case 's':
				schealg=optarg;
				break;
			case 'v':
				vflag=true;
				break;
		}
	}

	ifstream ifile(argv[optind]), rfile(argv[optind+1]);
	// ignore first line of rfile which indicates the number of random numbers in the file
	string tmp; getline(rfile, tmp);

	// Create a process start event for all processes in the input file and enter these events into the event queue
	vector<Event*> eventQ;
	start(ifile, eventQ, rfile);

	// create scheduler based on options specifications
	Scheduler *sched=NULL;
	int quantum=-1;
	if(schealg==""||schealg[0]=='F')  sched = new FCFS("FCFS");
	else if(schealg[0]=='L')  sched = new LCFS("LCFS");   
	else if(schealg[0]=='S') sched = new SJF("SJF"); 
	else if(isRR(schealg)) { stringstream ss(schealg.substr(1)); ss>>quantum; sched = new RR("RR", quantum); } 
	else if(isPRIO(schealg)) { stringstream ss(schealg.substr(1)); ss>>quantum; sched = new PRIO("PRIO", quantum); }
	else { cout<<"Unknown Scheduler spec: -v {FLSR}"<<endl; return 0;}

	// start simulation
	simulation(rfile, sched, eventQ);

	printInfo(quantum, sched);

	// clean memory
	for(int i=0;i<allProcesses.size(); i++) delete allProcesses[i];
	delete sched;

	ifile.close();
	rfile.close();
	return 0;
}
