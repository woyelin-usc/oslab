#include <iostream>
#include <fstream>
#include <sstream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <list> 
#include <vector>
#include <iomanip>

using namespace std;

char alg = 'l';
unsigned int fmsize = 32; 
string option = "";
ifstream *ifile = NULL, *rfile = NULL;
bool Oflag = false, Pflag = false, Fflag = false, Sflag = false, pflag = false, fflag = false, aflag = false;
int pageFaultCount = 0;

struct PTE {
	unsigned int pgidx ,counter;
	unsigned int p:1; // present bit
	unsigned int m:1; // modified bit
	unsigned int r:1; // reference bit
	unsigned int po:1; // pagedout bit
	unsigned int fmidx:6; // index to the physical frame 
	unsigned int unused:22; // 22 bits unused
	PTE (unsigned int idx) { this -> pgidx = idx; this->p = 0; this->m = 0; this->r = 0; this->po = 0; this->fmidx = 0; counter = 0; }
};

struct Frame {
	unsigned int fmidx;
	PTE *pte = NULL;
	Frame (unsigned int idx) { this -> fmidx = idx; }
};

struct Stats {
	unsigned  unmaps, maps, ins, outs, zeros;
	unsigned long long totalcost;
	Stats () { unmaps = 0; maps = 0; ins = 0; outs = 0; zeros = 0; totalcost = 0; }
};

Stats stats;

unsigned int getran ()  {
	string tmp = "";

	while( true ) {
		if ( rfile -> eof() ) {
			rfile -> clear(); 
			rfile -> seekg(0);
			getline( *rfile, tmp );
		}
		getline( *rfile, tmp);
		if ( tmp.length() == 0 ) continue;
		break;
	} 

	stringstream ss (tmp);
	unsigned int num;  ss >> num; 
	return num;
}

PTE* idx_page ( list<PTE*>& pgTable, unsigned pgidx ) {
	PTE *pte = NULL;
	for( list<PTE*>::iterator it = pgTable.begin(); it!=pgTable.end(); ++it) {
		if( (*it)->pgidx == pgidx ) { pte = *it; break; }
	}
	return pte;
}

Frame* idx_frame ( list<Frame*>& fmTable, unsigned fmidx ) {
	Frame *frame = NULL;
	for ( list <Frame*>::iterator it = fmTable.begin(); it!=fmTable.end(); ++it) {
		if( (*it)->fmidx == fmidx ) { frame = *it; break; }
	}
	return frame;
}

class Pager {
public:
	Pager () { 
		for( unsigned int i=0; i<64; i++ ) pgTable.push_back ( new PTE(i) ); 
		for( unsigned int i=0; i<fmsize; i++) fmTable.push_back( new Frame(i) );
	}
	virtual void order_LRU ( PTE* pte ) {}
	virtual Frame* allocate_frame() = 0;
	list < PTE* > pgTable; 
	list < Frame* > fmTable;
};

class NRU: public Pager {
public:
	NRU () : Pager () {}
	// first choose the frame/page int he lowest class, then if 10 pagefault, clear all r bits!
	Frame* allocate_frame() {
		// initialize classify structure
		vector < vector< unsigned > > tmp;
		for(int i=0; i<4; i++) {
			vector<unsigned> row;
			tmp.push_back(row);
		}
		// classify all valid pages 
		for ( list<PTE*>::iterator it = pgTable.begin(); it!=pgTable.end(); ++it) {
			if ( (*it)->p ) {
				unsigned r = (*it)->r, m = (*it) -> m;
				if ( (!r) && (!m) )  tmp[0].push_back( (*it)->pgidx );  // class 0: r=0 && m=0
				else if ( (!r) && m ) tmp[1].push_back( (*it)->pgidx ); // class 1: r=0 && m=1
				else if( r && (!m) ) tmp[2].push_back( (*it)->pgidx );  // class 2: r=1 && m=0
				else if ( r && m ) tmp[3].push_back( (*it)->pgidx );    // class 3: r=1 && m=1
				else;				
			}
		}

		int classidx = -1;
		if( tmp[0].size() ) classidx = 0;
		else if( tmp[1].size() ) classidx = 1;
		else if( tmp[2].size() ) classidx = 2;
		else if( tmp[3].size() ) classidx = 3;
		else;

		unsigned int selidx = getran() % (tmp[classidx].size());

		// @@ lowest_class=2: selidx=3 from 0 1 2 3
		if( aflag ) {
			cout << " @@ lowest_class=" << classidx << ": selidx=" << selidx << " from ";
			for ( unsigned int i=0;i<tmp[classidx].size(); i++) cout << tmp[classidx][i] << " ";
			cout << endl;
		}

		// NRU requires that the REFERENCED-bit be periodically reset for all valid page table entries.  
		if(alg=='N') { 
			pageFaultCount++;
			if ( pageFaultCount == 10 ) { 
				if ( aflag ) cout << " @@ reset NRU refbits while walking PTE" << endl;
				pageFaultCount = 0;
				for ( list<Frame*>::iterator it = fmTable.begin(); it != fmTable.end(); ++it) 
					(*it)->pte->r = 0;
			}
		}    
		
		return idx_frame ( fmTable, idx_page (pgTable, tmp [classidx] [selidx]) -> fmidx);
	}
};

class LRU: public Pager {
public:
	LRU () : Pager () {}
	// front => least recently used;  back => most recently used  ( used NOT arrived! )
	Frame* allocate_frame( ) {
		Frame *frame = fmTable.front();
		fmTable.pop_front();
		fmTable.push_back(frame);
		return frame;
	}

	// Find the frame (with same idx), move it to the back (most recently used)
	void order_LRU ( PTE* pte ) {
		for( list<Frame*>::iterator it = fmTable.begin(); it != fmTable.end(); ++it) {
			if( (*it)->fmidx == pte->fmidx ) {
				Frame *frame = *it;
				fmTable.erase (it);
				fmTable.push_back(frame);
				return;
			}
		}
	}
};

class Random: public Pager {
public:
	Random () : Pager () { }
	Frame* allocate_frame( ) {
	  	return idx_frame( fmTable, getran() % fmsize );
	}
};

class FIFO: public Pager {
public:
	FIFO () : Pager () {}
	// front => old ; back => new
	Frame* allocate_frame() {
		Frame *frame = fmTable.front(); 
		fmTable.pop_front();
		fmTable.push_back(frame);
		return frame;
	}
};

class SecondChance: public Pager {
public:
	SecondChance () : Pager ( ) {}
	// front => old ; back => new
	Frame* allocate_frame( ) {
		Frame *target = NULL;
		while(true) {
			Frame *frame = fmTable.front();
			fmTable.pop_front();
			fmTable.push_back(frame) ;

			// if old but used, clear R bit and move to the back
			if( frame -> pte -> r ) frame -> pte -> r = 0;
			// if old and unused, find it!
			else {
				target = frame;
				break;
			}
		}
		return target;
	}
};

class PhyClock: public Pager {
public:
	PhyClock () : Pager ( ) { it = fmTable.begin(); }
	Frame* allocate_frame( ) {
		Frame *frame = NULL;
		while(true) {
			// if r bit is 1, clear r bit and increment iterator
			if ( (*it) -> pte -> r )  {
				(*it) -> pte -> r = 0;
				++it;
				if ( it == fmTable.end() ) it = fmTable.begin();
			}
			// if r bit is 0, the page is evicted and increment iteraotr
			else {
				frame = *it;
				++it;
				if ( it == fmTable.end() ) it = fmTable.begin();
				break;
			}
		}
		return frame;
	}
	list<Frame*>::iterator it;
};

class VirClock: public Pager {
public:
	VirClock () : Pager ( ) { it = pgTable.begin(); }
	Frame* allocate_frame( ) {
		Frame *frame = NULL;
		while(true) {
			PTE* pte = *it;
			// for all present pte
			if( pte -> p ) {
				// if the r bit is 1, clear the bit and increment iterator
				if ( pte->r ) {
					pte->r = 0;
					++it;
					if(it == pgTable.end()) it = pgTable.begin();
				}
				// if the r bit is 0, evict the page and increment iterator
				else {
					list<Frame*>::iterator it2 = fmTable.begin();
					advance ( it2, (*it) -> fmidx );
					frame = *it2;
					++it;
					if(it == pgTable.end()) it = pgTable.begin();
					break;
				}
			}
			else ++it;
			if(it == pgTable.end()) it = pgTable.begin();
		}
		return frame;
	}
	list<PTE*>::iterator it;
};

class PhyAging: public Pager {
public:
	PhyAging () : Pager ( ) {}
	Frame* allocate_frame() {
		// shift, add r bit, and clean r bits  of  all pager 
		for( list<PTE*>::iterator it = pgTable.begin(); it!=pgTable.end(); ++it) {
			if ( (*it) -> p ) {
				( (*it) -> counter) >>= 1;
				( (*it) -> counter) |= ( ((*it) -> r) << 31 );
				(*it)->r = 0;
			} 
		}

		// make a decision based on physical frame
		Frame *frame = NULL;
		// find the frame associated with the smallest counter
		for ( list<Frame*>::iterator it = fmTable.begin(); it != fmTable.end(); ++it ) {
			if( frame==NULL ) frame = *it;
			else {
				if( (*it)->pte->counter < frame->pte->counter ) frame = *it;
			} 
		}

		frame->pte->counter = 0;
		return frame;
	}
};

class VirAging: public Pager {
public:
	VirAging () : Pager ( ) {}
	Frame* allocate_frame() {
		// shift, add r bit, and clean r bits  of  all pager 
		for( list<PTE*>::iterator it = pgTable.begin(); it!=pgTable.end(); ++it) {
			if ( (*it)->p ) {
				( (*it) -> counter) >>= 1;
				( (*it) -> counter) |= ( ((*it) -> r) << 31 );
				(*it)->r = 0;
			}
		}

		// implement aging info 
		// @@  0:40000000 * * 3:24880000 4:400801 5:20481042 6:400000 7:800000 8:44002000 * 10:a0000000 11:8000000 * * * 15:80000000 16:2000000 17:200000 * 19:2008000 20:40010 * * * * * * 27:10000000 28:6000800 29:20100000 * * 32:40000 * * * * * * * * * * * * 45:4000000 * * * * * * * * * * 56:41000000 * * * * * * *
		if ( aflag ) {
			cout << " @@  ";
			for ( list<PTE*>::iterator it = pgTable.begin(); it!=pgTable.end(); ++it) {
				if  ( (*it)->p)  { cout << (*it)->pgidx << ":" << hex << (*it)->counter << " "; cout << dec; }
				else cout << "* ";
			}
			cout << endl;
		}

		PTE *pte = NULL;
		for ( list<PTE*>::iterator it = pgTable.begin(); it!=pgTable.end(); ++it) {
			if( (*it)->p ) {
				if( pte == NULL ) pte = *it;
				else {
					if( (*it)->counter < pte->counter ) pte = *it;
				}
			}
		}

		// implement aging info
		//  @@ min_pte = 30 age=40000
		if ( aflag )  cout << " @@ min_pte = " << pte->pgidx << " age=" << hex << pte->counter << dec << endl;

		pte->counter = 0;

		return idx_frame ( fmTable, pte->fmidx );
	}
};

Frame * getFrame ( Pager* pager, unsigned instrIdx ) {
	// first find a frame from freelist
	for( list<Frame*>::iterator it = pager -> fmTable.begin(); it != pager -> fmTable.end(); ++it ) {
		if ( (*it) -> pte == NULL ) return *it;
	}

	// Here we don't have a free frame, Page fault happens! (to decide which frame to evict and adjust fmTable structure)

	Frame *theFrame = pager -> allocate_frame();  
	// first unmap the frame and update corresponding bit for page and frame 
	if (Oflag) {
		cout << instrIdx <<": ";
		cout << left << setw(5) << "UNMAP";
		cout << right << setw(4) << theFrame -> pte -> pgidx;
		cout << right << setw(4) << theFrame -> fmidx << endl; 
	}
	stats.unmaps++;
	// when the page has been modified, page it out and set pageout bit
	if( theFrame -> pte -> m ) {
		if (Oflag) {
			cout << instrIdx << ": ";
			cout << left << setw(5) << "OUT";
			cout << right << setw(4) << theFrame -> pte -> pgidx;
			cout << right << setw(4) << theFrame -> fmidx << endl;
		}
		stats.outs++;
		theFrame -> pte -> po = 1;
	}
	theFrame -> pte -> p = 0; // clear PTE's present bit, (indicates page not in the memory)
	theFrame -> pte = NULL; // update inverse mapping (indicates unmap between page and frame)
	return theFrame;
}

void exec( int instrIdx, int operation, unsigned  pgidx, Pager* pager ) {
	// print the instruction first!
	if (Oflag) cout << "==> inst: " << operation << " " << pgidx <<endl;       
	PTE *pte = idx_page ( pager->pgTable, pgidx );
	
	// when the virtual page is not present in the memory ( present bit = 0 ), deal with page fault! 
	if( !(pte->p) ) {   
		// get a clean frame ( already ummap in the process )
		Frame * frame = getFrame( pager, instrIdx ); 
		// if pagedout bit is 1, page in from disk; else zero frame
		if ( pte->po ) { 
			if (Oflag) {
				cout << instrIdx << ": ";
				cout << setw(5) << left << "IN";
				cout << setw(4) << right << pte -> pgidx;
				cout << setw(4) << right << frame -> fmidx << endl; 
			}
			stats.ins++;
		} 
		else { 
			if (Oflag) {
				cout << instrIdx << ": ";
				cout << left << setw(5) << "ZERO";
				cout << setw(4) << "";
				cout << setw(4) << right << frame -> fmidx << endl; 
			}
			stats.zeros++;
		}
		
		// map the new page to the frame and update correspoding bit of page and frame
		if (Oflag)  {
			cout<< instrIdx << ": ";
			cout << left << setw(5) << "MAP";
			cout << right << setw(4) << pte -> pgidx;
			cout << right << setw(4) << frame -> fmidx << endl;  
		}
		stats.maps++;
		pte -> r = 0;  pte -> m = 0; pte -> p = 1;  
		pte -> fmidx = frame -> fmidx;
		frame -> pte = pte;
	}
	// set reference bit in both read / write, set modified bit when write
	pte -> r = 1;
	if ( operation ) pte -> m = 1; 
	if ( alg == 'l' ) pager -> order_LRU (pte);  // maintain LRU order 


	if( pflag ) {
		for (list<PTE*>::iterator it = pager->pgTable.begin(); it!= pager->pgTable.end(); ++it ) {
			if( (*it)->p ) {
				cout << (*it)->pgidx << ":";
				cout << (((*it)->r)? "R":"-");
				cout << (((*it)->m)? "M":"-");
				cout << (((*it)->po)? "S":"-") << " ";
			}
			// Pages that are not valid are represented by a ‘#’ if they have been swapped out (note you don’t have to swap out a page if it was only referenced but not modified), or a ‘*’ if it does not have a swap area associated with
			else cout << (((*it)->po)? "#" : "*") << " ";
		}
		cout << endl;
	}
}

int main(int argc, char** argv)
{
	// First verify command line arguments
	int c;
	while ((c = getopt (argc, argv, "a:f:o:")) != -1) {
		switch(c) {
			case 'a': alg = optarg[0]; break; // alg: only consider FIRST character following -a
			case 'f': fmsize = stoi(optarg); break; // consider the whole string following -o
			case 'o': option = optarg;  break; // need to consider the whole string following -o
			case '?':
			if (optopt == 'a' || optopt == 'f' || optopt == 'o') printf("Usage: ./mmu [-a<algo>] [-o<options>] [–f<num_frames>] inputfile randomfile\n"); 
			else if (isprint(optopt)) printf("Usage: ./mmu [-a<algo>] [-o<options>] [–f<num_frames>] inputfile randomfile\n");
			else printf ("Unknown option character `\\x%x'.\n", optopt);
			default: return 1;
		}
	}

	// Then verify input files
	if ( optind < argc ) {
		ifile = new ifstream(argv[optind]);
		if(ifile -> fail()) { cerr<<"Not a valid inputfile <" << argv[optind] << ">" <<endl;  return 1; }
	}
	else { cerr << "Not a valid inputfile <(null)>"<<endl; return 1; }
	optind++;
	if ( optind < argc ) {
		rfile = new ifstream(argv[optind]);
		if(rfile -> fail()) { cerr<<"Not a valid random file <"<< argv[optind] << ">" <<endl; return 1; }
	}
	else { cerr << "Not a valid random file <(null)>"<< endl; return 1; }

	string tmp = ""; getline( *rfile, tmp ); // skip the first line

	/****************************************************************************************
	 Here I have finished verifing command line arguments and input files, so start execution
	 ****************************************************************************************/
	 
	// check replacement algorithm validity and allocate pager
	 Pager *pager = NULL;
	 switch(alg) {
	 	case 'N': pager = new NRU( ); break;
	 	case 'l': pager = new LRU( ); break;
	 	case 'r': pager = new Random( ); break;
	 	case 'f': pager = new FIFO( ); break;
	 	case 's': pager = new SecondChance( ); break;
	 	case 'c': pager = new PhyClock( ); break;
	 	case 'X': pager = new VirClock( ); break;
	 	case 'a': pager = new PhyAging( ); break;
	 	case 'Y': pager = new VirAging( ); break;
	 	default: cerr << "Unknown Replacement Algorithm: <" << alg << ">" << endl; return 1;
	 }
	 // check options validity adn set corresponding flags
	 for( unsigned int i=0; i<option.length(); i++ ) {
	 	if ( option[i] == 'O' ) Oflag = true; 
	 	else if ( option[i] == 'P' ) Pflag = true;
	 	else if ( option[i] == 'F' ) Fflag = true;
	 	else if ( option[i] == 'S' ) Sflag = true;
	 	else if ( option[i] == 'p' ) pflag = true;
	 	else if ( option[i] == 'f' ) fflag = true;
	 	else if ( option[i] == 'a' ) aflag = true;
	 	else { cerr << "Unknown output option: <" <<option[i] << ">" <<endl; return 1; }
	 }

	 string instr;  // inst: one line of instruction (e.g.: 0 34)
	 int linenum = 0, instrIdx = 0; 
	 while( getline(*ifile, instr) ) {
	 	linenum++;
		if( instr.length()==0 || instr[0] == '#' ) continue; // check whether empty line or line starts with '#' (ignored)
		else {
			int operation, pgidx;
			stringstream ss(instr);
			ss >> operation; ss >> pgidx;
			if( pgidx > 63 || pgidx < 0 ) {       // check whether virtual page is in valid range
				cerr << "address <" << pgidx << "> exceeds max virtual address <63>, line " << linenum <<endl;
				return 1;
			}
			// here we have a valid instruction, execute the instruction
			exec( instrIdx, operation, pgidx, pager );
			instrIdx++;
		}
	}

	// maps and unmaps each cost 400 cycles, page-in/outs each cost 3000 cycles, zeroing a page costs 150 cycles and each access (read or write) costs 1 cycles.
	 stats.totalcost += (unsigned long long )(stats.unmaps + stats.maps)*400 + (unsigned long long) (stats.ins + stats.outs)*3000+ (unsigned long long) stats.zeros * 150 + (unsigned long long) instrIdx;

	// print page info
	if( Pflag ) {
		for (list<PTE*>::iterator it = pager->pgTable.begin(); it!=pager->pgTable.end(); ++it ) {
			if( (*it)->p ) {
				cout << (*it)->pgidx << ":";
				cout << (((*it)->r)? "R":"-");
				cout << (((*it)->m)? "M":"-");
				cout << (((*it)->po)? "S":"-") << " ";
			}
			// Pages that are not valid are represented by a ‘#’ if they have been swapped out (note you don’t have to swap out a page if it was only referenced but not modified), or a ‘*’ if it does not have a swap area associated with
			else cout << (((*it)->po)? "#" : "*") << " ";
		}
		cout << endl;
	}

	// construct and print frame info
	if( Fflag ) {
		string fm_info[fmsize];
		for (list<Frame*>::iterator it = pager->fmTable.begin(); it!= pager->fmTable.end(); ++it) {
			if( (*it)->pte == NULL ) fm_info[ (*it)->fmidx ] = "*" ; 
			else fm_info[ (*it)->fmidx ] = to_string ( (*it)->pte->pgidx ); 
		}
		for(unsigned i=0; i<fmsize; i++) cout << fm_info[i] + " ";
		cout << endl;
	}

	// print summary if -S
	if( Oflag || Sflag ) 
		printf("SUM %d U=%d M=%d I=%d O=%d Z=%d ===> %llu\n",
			instrIdx, stats.unmaps, stats.maps, stats.ins, stats.outs, stats.zeros, stats.totalcost);

	// clean memory and terminate program
	for ( list<PTE*>::iterator it = pager->pgTable.begin(); it!=pager->pgTable.end(); ++it) delete *it;
	for( list<Frame*>::iterator it = pager->fmTable.begin(); it!=pager->fmTable.end(); ++it) delete *it;
	ifile -> close(); rfile -> close();  delete ifile; delete rfile;	
	return 0;
}
