#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctype.h>
#include <map>
#include <iomanip>
#include <cstdlib>
#include <sstream>

using namespace std;

int lineNum=1;
int offset=1, lastLineOffset=0;
int totalInstr=0;

vector<string> symOrderRecord;

vector<int> moduleForSymNotUse;
vector<string> symNotUse;


struct symbol
{
	string name;
	int val;
	bool multiDef;
	int moduleIdx;
	bool defNoUse;

	symbol() {this->name=""; this->val=0;}
	symbol(string name, int val) { this->name = name; this->val = val; multiDef=false; moduleIdx=-1; defNoUse=true;}
};

struct instruction 
{
	string type, buf;
	instruction() {type=""; buf="";}
	instruction(string type, string buf) {this->type=type; this->buf=buf;}
};

struct module 
{
	int base;

	//int numSymDef;
	//vector<symbol*> defList;

	//int numSymUse;
	//vector<string> useList;

	int numInstr;
	//vector<instruction*> instrList;

	module() {this->base=0; this->numInstr=0;}
};

bool isDigit(string&);
bool isSymbol(string&);
bool isType(string&);
void parseErrMsg(int, bool);
string read(ifstream&);
void skipDelimiter(ifstream&);
void readDefList1(ifstream& , vector<module*>& , int);
void readUseList1(ifstream& , vector<module*>& , map<string, symbol*>&);
void readInstrList1(ifstream&,vector<module*>&, int, map<string, symbol*>&);
void readfile1(ifstream&, vector<module*>&, string&);
void cleanMem(vector<module*>&, map<string, symbol*>&);
void close(vector<module*>&, map<string, symbol*>&);
void printSymTable(vector<module*>&, map<string, symbol*>&);


int stoi(string str) 
{
	stringstream ss;
	ss<<str;
	int num;
	ss>>num;
	return num;
}

void close(vector<module*>& mList, map<string, symbol*>& symTable)
{
	cleanMem(mList, symTable);
	exit(0);
}

bool isType(string& str)
{
	return str=="I"||str=="R"||str=="A"||str=="E";
}

bool isDigit(string& str)
{
	for(unsigned int i=0;i<str.length();i++) {
		if(!isdigit(str[i])) return false;
	}
	return true;
}

bool isSymbol(string& str)
{
	if(!str.length()) return false;
	for(unsigned int i=0;i<str.length();i++) {
		if(!i) {
			if(!isalpha(str[i])) return false;
		}
		else {
			if(!isalnum(str[i])) return false;
		}
	}
	return true;
}

void parseErrMsg(int type, bool special)
{
	static string errstr[] = {"NUM_EXPECTED", "SYM_EXPECTED", "ADDR_EXPECTED", 
	"SYM_TOLONG", "TO_MANY_DEF_IN_MODULE", "TO_MANY_USE_IN_MODULE", "TO_MANY_INSTR"};

	if(special && offset==1) cout<<"Parse Error line "<<lineNum-1<<" offset "<<lastLineOffset<<": "<<errstr[type]<<endl;
	else cout<<"Parse Error line "<<lineNum<<" offset "<<offset<<": "<<errstr[type]<<endl;
}

string read(ifstream& ifile)
{
	string str;
	ifile>>str;
	return str;
}

void skipDelimiter(ifstream& ifile)
{
	while(!ifile.eof()) {
		char ch=ifile.peek();
		if(ch=='\n'){ 
			lastLineOffset = offset;
			lineNum++;
			offset=1;
			ifile.get();
		}
		else if(ch==' '||ch=='\t') { ifile.get(); offset++; }
		else break;
	}
}

// idx: index of current module
void readDefList1(ifstream& ifile, vector<module*>& mList, int idx, map<string, symbol*>& symTable)
{
	string tmp=read(ifile);
	if(!isDigit(tmp)) {parseErrMsg(0, false); close(mList, symTable);}
	//mList[idx]->numSymDef=stoi(tmp);
	if(stoi(tmp)>16) { parseErrMsg(4, false); close(mList, symTable); }
	offset+=tmp.length();

	for(int i=0;i<stoi(tmp);i++) {
		skipDelimiter(ifile);
		if(ifile.eof()) { parseErrMsg(1, true); close(mList, symTable);}
		string symName=read(ifile);
		if(!isSymbol(symName)) { parseErrMsg(1, false); close(mList, symTable);}
		offset+=symName.length();

		skipDelimiter(ifile);
		if(ifile.eof()) {parseErrMsg(0, true); close(mList, symTable);}
		string symVal=read(ifile);
		if(!isDigit(symVal)){ parseErrMsg(0, false); close(mList, symTable);}
		offset+=symVal.length();

		//symbol* newSym = new symbol(symName, stoi(symVal)+mList[idx]->base );
		
		// check whether has multi defined
		map<string,symbol*>::iterator got=symTable.find(symName);
		if(got==symTable.end()) {
			symbol* newSym = new symbol(symName, stoi(symVal)+mList[idx]->base );
			//mList[idx]->defList.push_back(newSym);
			symTable.insert(pair<string, symbol*>(symName, newSym));
			symOrderRecord.push_back(symName);
			newSym->moduleIdx=idx;
		}
		else  { got->second->multiDef=true;}
		
	}
}

void readUseList1(ifstream& ifile, vector<module*>& mList, map<string, symbol*>& symTable)
{
	string tmp=read(ifile);
	if(!isDigit(tmp)) { parseErrMsg(0, false); close(mList, symTable); }
	//mList[idx]->numSymUse=stoi(tmp);
	if(stoi(tmp)>16) {parseErrMsg(5, false);close(mList, symTable);}
	offset+=tmp.length();

	for(int i=0;i<stoi(tmp);i++) {
		skipDelimiter(ifile);
		if(ifile.eof()) { parseErrMsg(1, true); close(mList, symTable);}
		string symName=read(ifile);
		if(!isSymbol(symName)) { parseErrMsg(1, false); close(mList, symTable);}
		offset+=symName.length();
		//mList[idx]->useList.push_back(symName);
	}

}

// idx: index of current module, to record number of instructions in the current module
void readInstrList1(ifstream& ifile,vector<module*>& mList, int idx, map<string, symbol*>& symTable)
{
	string tmp=read(ifile);
	if(!isDigit(tmp))  { parseErrMsg(0, false); close(mList, symTable);}
	totalInstr += stoi(tmp);
	if(totalInstr>512) { parseErrMsg(6, false); close(mList, symTable);}
	mList[idx]->numInstr=stoi(tmp);
	offset+=tmp.length();

	for(int i=0;i<stoi(tmp);i++) {
		skipDelimiter(ifile);

		if(ifile.eof()) { parseErrMsg(2,true); close(mList, symTable); }
		string newType=read(ifile);
		if(!isType(newType)) { parseErrMsg(2, false); close(mList, symTable); }
		offset+=newType.length();

		skipDelimiter(ifile);
		if(ifile.eof()) {parseErrMsg(2,true); close(mList, symTable);}
		string newBuf = read(ifile);
		if(!isDigit(newBuf)) {parseErrMsg(2, false); close(mList, symTable);}
		offset+=newBuf.length();

		//mList[idx]->instrList.push_back(new instruction(newType, newBuf));
	}
}

void readfile1(ifstream& ifile, vector<module*>& mList, map<string, symbol*>& symTable)
{
	// i: idx of new module in the mList
	int i=0;
	while(!ifile.eof()) {
		skipDelimiter(ifile);
		if(ifile.eof()) break;		
		else {
			mList.push_back(new module());
			
			// configure new module base address
			if(!i) mList[i]->base=0;
			else mList[i]->base=mList[i-1]->base + mList[i-1]->numInstr;

			readDefList1(ifile, mList, i, symTable);
			skipDelimiter(ifile);
			if(ifile.eof()) {parseErrMsg(0, true); close(mList, symTable);}

			readUseList1(ifile, mList, symTable);
			skipDelimiter(ifile);
			if(ifile.eof()) {parseErrMsg(0, true); close(mList,  symTable);}

			readInstrList1(ifile, mList, i, symTable);
			skipDelimiter(ifile);
		}
		i++;
	}
}

void output(vector<module*>& mList) 
{
	// cout<<"MODULE SIZE: "<<mList.size()<<endl;
	
	// cout<<"BASE ADDRESS: ";
	// for(unsigned int i=0;i<mList.size();i++) cout<<mList[i]->base<<",";
	// cout<<endl;
	
	// cout<<"SYMBOL TABLE:";
	// for(unsigned int i=0;i<mList.size();i++) {
	// 	vector<symbol*> tmpList = mList[i]->defList;
	// 	for(unsigned int j=0;j<tmpList.size();j++) {
	// 		cout<<" "<<tmpList[j]->name<<"="<<tmpList[j]->val;
	// 	}
	// }
	// cout<<endl;
}

// clean all symbol* and module* which have been dynamically allocated
void cleanMem(vector<module*>& mList, map<string, symbol*>& symTable)
{
	for(unsigned int i=0;i<mList.size();i++) delete mList[i];

	for(map<string, symbol*>::iterator it=symTable.begin();
		it!=symTable.end();
		++it) {
		delete it->second;
	}
	// for(unsigned int i=0;i<mList.size();i++) {
	// 	module* m = mList[i];
	// 	vector<symbol*> defs = m->defList;
	// 	for(unsigned j=0; j<defs.size();j++) delete defs[j];
	// 	vector<instruction*> instrs = m->instrList;
	// 	for(unsigned j=0; j<instrs.size();j++) delete instrs[j];
	// 	delete m;
	// }	
}

void printSymTable(vector<module*>& mList, map<string, symbol*>& symTable)
{

	for(unsigned int i=0; i<symOrderRecord.size();i++) {
		symbol* sym = symTable.find(symOrderRecord[i])->second;
		if(sym->val>=mList[sym->moduleIdx]->numInstr+mList[sym->moduleIdx]->base) {
			cout<<"Warning: Module "<<sym->moduleIdx+1
			<<": "<<sym->name<<" to big "
			<<sym->val<<" (max="<<mList[sym->moduleIdx]->numInstr-1	
			<<") assume zero relative"<<endl;
			
			sym->val=0;
		}
	}

	cout<<"Symbol Table "<<endl;
	for(unsigned int i=0;i<symOrderRecord.size();i++) {
		symbol* sym = symTable.find(symOrderRecord[i])->second;
		cout<<sym->name<<"="<<sym->val;
		if(sym->multiDef) cout<<" Error: This variable is multiple times defined; first value used";
		cout<<endl;
	}	
}

void printMemMap(vector<module*>& mList)
{
	// cout<<"Memory Map"<<endl;
	// int idx=0;
	// for(int i=0;i<mList.size();i++) {
	// 	module* m=mList[i];
	// 	for(int j=0;j<m->instrList.size();j++) {
	// 		instruction* instr=m->instrList[j];
	// 		cout<<setw(3)<<setfill('0')<<idx++<<": ";

	// 	}
	// }
}

void readDefList2(ifstream& ifile, int numDef)
{
	string word;
	for(int i=0;i<numDef;i++) {
		// read symbol(def) name
		ifile>>word;
		// read symbol address
		ifile>>word;
	}
}

void readUseList2(ifstream& ifile,map<string, symbol*>& symTable, int numUse, string* useList, bool used[])
{
	string word;
	for(int i=0;i<numUse;i++) {
		// read symbol(use) name
		ifile>>word;
		useList[i]=word;
		used[i]=false;
		map<string, symbol*>::iterator it=symTable.find(word);
		// mark whether a symbol is defined AND used
		if(it!=symTable.end()) it->second->defNoUse=false;
	}
}

// bool used[]: check whether a symbol appeared in the uselist but was not actually used
// depends on specific module so cannot make this a member variable of symbol
void readInstrList2(ifstream& ifile, vector<module*>& mList, int idx, map<string, symbol*>& symTable, int numInstr, int& memIdx, string* useList, bool used[], int numUse)
{

	string word;
	for(int i=0;i<numInstr;i++) {
		cout<<setw(3)<<setfill('0')<<memIdx++<<": ";

		string type; ifile>>type;
		string address; ifile>>address;

		int opcode, addcode;
		if(address.length()<4) { opcode=0; addcode=stoi(address); }
		else { opcode=address[0]-'0'; addcode=stoi(address.substr(1)); }

		if(type=="I") {
			if(address.length()>4) {
				cout<<"9999 Error: Illegal immediate value; treated as 9999"<<endl;
			}
			else {
				cout<<setw(4)<<setfill('0')<<address<<endl;
			}
		}
		else if(type=="R") {
			if(address.length()>4) {
				cout<<"9999 Error: Illegal opcode; treated as 9999"<<endl;
			}
			else {
				// Relative address exceeds module size; zero used
				if(addcode>=mList[idx]->numInstr) {
					cout<<opcode;
					cout<<setw(3)<<setfill('0')<<(stoi("000")+mList[idx]->base)
					<<" Error: Relative address exceeds module size; zero used"<<endl;
				}
				else {
					cout<<opcode;
					cout<<setw(3)<<setfill('0')<<addcode+mList[idx]->base<<endl;
				}

			}
		}
		else if(type=="A") {
			if(addcode>=512) {
				cout<<opcode<<"000 Error: Absolute address exceeds machine size; zero used"<<endl;
			}
			else {
				cout<<opcode;
				cout<<setw(3)<<setfill('0')<<addcode<<endl;
			}

		}
		else if(type=="E") {
			if(address.length()>4) {
				cout<<"9999 Error: Illegal opcode; treated as 9999"<<endl;
			}
			else if(addcode>=numUse) {
				cout<<setw(4)<<setfill('0')<<address<<" Error: External address exceeds length of uselist; treated as immediate"<<endl;
			}
			else if(addcode<numUse) {
				used[addcode]=true;
				//cout<<"Check "<<(symTable.find(useList[addcode])==symTable.end())<<endl;
				cout<<opcode;
				map<string ,symbol*>::iterator got=symTable.find(useList[addcode]);
				if(got==symTable.end()) {
					cout<<"000 Error: "<<useList[addcode]<<" is not defined; zero used"<<endl;
				}
				else {
					cout<<setw(3)<<setfill('0')<<symTable.find(useList[addcode])->second->val<<endl;

				}
			}
		}
		else;
	}
	for(int i=0;i<numUse;i++) {
		if(!used[i]) { moduleForSymNotUse.push_back(idx); symNotUse.push_back(useList[i]);}
	}
}

void printWarning(map<string, symbol*>& symTable)
{
	for(unsigned int i=0;i<moduleForSymNotUse.size();i++) {
		cout<<"Warning: Module "<<moduleForSymNotUse[i]+1<<": "
		<<symNotUse[i]<<" appeared in the uselist but was not actually used"<<endl;
	}

	for(unsigned int i=0;i<symOrderRecord.size();i++) {
		symbol* sym = symTable.find(symOrderRecord[i])->second;
		if(sym->defNoUse) {
			cout<<"Warning: Module "<<sym->moduleIdx+1<<" : "
			<<sym->name<<" was defined but never used"<<endl;
		}
	}
}

void readfile2(ifstream& ifile, vector<module*>& mList, map<string, symbol*>& symTable)
{
	// i: idx of current module
	int i=0, memIdx=0;
	string word;
	while(ifile>>word) {
		readDefList2(ifile, stoi(word));

		ifile>>word;
		int numUse = stoi(word);
		string useList[numUse];
		bool used[numUse];
		readUseList2(ifile, symTable, numUse, useList, used);

		ifile>>word;
		readInstrList2(ifile, mList, i, symTable, stoi(word), memIdx, useList, used,numUse);

		i++;
	}
	cout<<endl;
	printWarning(symTable);
}


int main(int argc, char** argv)
{
	if(argc<2)  { cerr<<"Enter your input file."<<endl; return 1; }
	ifstream ifile(argv[1]);
	if(ifile.fail()) { cerr<<"Unable to open file "<<argv[1]<<endl; return 1; }

	// create empty module list and symbol table
	vector<module*> mList;
	map<string, symbol*> symTable;
	
	//First Pass
	readfile1(ifile, mList, symTable);
	printSymTable(mList, symTable);
	cout<<endl;

	// Second Pass
	ifile.clear();
	ifile.seekg(0);
	cout<<"Memory Map"<<endl;
	map<int, vector<string> > use;
	readfile2(ifile, mList, symTable);


	//printMemMap();
	//output(mList);

	cleanMem(mList, symTable);

	ifile.close();
	return 0;
}