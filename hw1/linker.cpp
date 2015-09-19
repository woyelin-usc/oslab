#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctype.h>

using namespace std;

int lineNum=0;
int offset=0;
int totalInstr=0;

struct symbol
{
	string name;
	int val;

	symbol() {this->name=""; this->val=0;}
	symbol(string name, int val) { this->name = name; this->val = val;}
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

	int numSymDef;
	vector<symbol*> defList;

	int numSymUse;
	vector<string> useList;

	int numInstr;
	vector<instruction*> instrList;

	module() {this->base=0; this->numSymDef=0; this->numSymUse=0; this->numInstr=0;}
};

bool isDigit(string&);
bool isSymbol(string&);
bool isType(string&);
void parseErrMsg(int);
string read(ifstream&);
void skipDelimiter(ifstream&);
void readDefList(ifstream& , vector<module*>& , int);
void readUseList(ifstream& , vector<module*>& , int);
void readInstrList(ifstream&,vector<module*>&, int);
void readfile1(ifstream&, vector<module*>&, string&);
void cleanMem(vector<module*>&);
void close(vector<module*>&);


void close(vector<module*>& mList)
{
	cleanMem(mList);
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

void parseErrMsg(int type)
{
	static string errstr[] = {"NUM_EXPECTED", "SYM_EXPECTED", "ADDR_EXPECTED", 
	"SYM_TOLONG", "TO_MANY_DEF_IN_MODULE", "TO_MANY_USE_IN_MODULE", "TO_MANY_INSTR",
	"TYPE_EXPECTED" };

	cout<<"Parse Error line "<<lineNum<<" offset "<<offset<<": "<<errstr[type]<<endl;
}

string read(ifstream& ifile)
{
	string str;
	ifile>>str;
	offset+=str.length();
	return str;
}

void skipDelimiter(ifstream& ifile)
{
	while(!ifile.eof()) {
		char ch=ifile.peek();
		if(ch=='\n'){ ifile.get(); lineNum++; offset=0;}
		else if(ch==' '||ch=='\t') { ifile.get(); offset++; }
		else break;
	}
}

void readDefList(ifstream& ifile, vector<module*>& mList, int idx)
{
	string tmp=read(ifile);
	if(!isDigit(tmp)) {parseErrMsg(0); close(mList);}
	mList[idx]->numSymDef=stoi(tmp);
	if(mList[idx]->numSymDef>16) { parseErrMsg(4); close(mList); }

	for(int i=0;i<mList[idx]->numSymDef;i++) {
		skipDelimiter(ifile);
		if(ifile.eof()) { parseErrMsg(1); close(mList);}
		tmp=read(ifile);
		if(!isSymbol(tmp)) { parseErrMsg(1); close(mList);}
		string symName=tmp;

		skipDelimiter(ifile);
		if(ifile.eof()) {parseErrMsg(0); close(mList);}
		tmp=read(ifile);
		if(!isDigit(tmp)){ parseErrMsg(0); close(mList);}
		int symVal=stoi(tmp)+mList[idx]->base;

		mList[idx]->defList.push_back(new symbol(symName, symVal));
	}
}

void readUseList(ifstream& ifile, vector<module*>& mList, int idx)
{
	string tmp=read(ifile);
	if(!isDigit(tmp)) { parseErrMsg(0); close(mList); }
	mList[idx]->numSymUse=stoi(tmp);
	if(mList[idx]->numSymUse>16) {parseErrMsg(5);close(mList);}

	for(int i=0;i<mList[idx]->numSymUse;i++) {
		skipDelimiter(ifile);
		if(ifile.eof()) { parseErrMsg(1); close(mList);}
		tmp=read(ifile);
		if(!isSymbol(tmp)) { parseErrMsg(1); close(mList);}
		mList[idx]->useList.push_back(tmp);
	}
}

void readInstrList(ifstream& ifile,vector<module*>& mList, int idx)
{
	string tmp=read(ifile);
	if(!isDigit(tmp))  { parseErrMsg(0); close(mList);}
	totalInstr += stoi(tmp);
	if(totalInstr>512) { parseErrMsg(6); close(mList);}
	mList[idx]->numInstr=stoi(tmp);

	for(int i=0;i<mList[idx]->numInstr;i++) {
		skipDelimiter(ifile);
		if(ifile.eof()) { parseErrMsg(7); close(mList); }
		string newType=read(ifile);
		if(!isType(newType)) { parseErrMsg(7); close(mList); }

		skipDelimiter(ifile);
		if(ifile.eof()) {parseErrMsg(2); close(mList);}
		string newBuf = read(ifile);
		if(!isDigit(newBuf)) {parseErrMsg(2); close(mList);}

		mList[idx]->instrList.push_back(new instruction(newType, newBuf));
	}
}

void readfile1(ifstream& ifile, vector<module*>& mList, string& tmp)
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

			readDefList(ifile, mList, i);
			skipDelimiter(ifile);
			if(ifile.eof()) {parseErrMsg(0); close(mList);}

			readUseList(ifile, mList, i);
			skipDelimiter(ifile);
			if(ifile.eof()) {parseErrMsg(0); close(mList);}

			readInstrList(ifile, mList, i);
			skipDelimiter(ifile);
		}
		i++;
	}
}

void output(vector<module*>& mList) 
{
	cout<<"MODULE SIZE: "<<mList.size()<<endl;
	
	cout<<"BASE ADDRESS: ";
	for(unsigned int i=0;i<mList.size();i++) cout<<mList[i]->base<<",";
	cout<<endl;
	
	cout<<"SYMBOL TABLE:";
	for(unsigned int i=0;i<mList.size();i++) {
		vector<symbol*> tmpList = mList[i]->defList;
		for(unsigned int j=0;j<tmpList.size();j++) {
			cout<<" "<<tmpList[j]->name<<"="<<tmpList[j]->val;
		}
	}
	cout<<endl;
}

void cleanMem(vector<module*>& mList)
{
	for(unsigned int i=0;i<mList.size();i++) {
		module* m = mList[i];
		vector<symbol*> defs = m->defList;
		for(unsigned j=0; j<defs.size();j++) delete defs[j];
		vector<instruction*> instrs = m->instrList;
		for(unsigned j=0; j<instrs.size();j++) delete instrs[j];
		delete m;
	}	
}

int main(int argc, char** argv)
{
	if(argc<2)  { cerr<<"Enter your input file."<<endl; return 1; }

	// open input file
	ifstream ifile(argv[1]);
	if(ifile.fail()) { cerr<<"Unable to open file "<<argv[1]<<endl; return 1; }

	// Here we open input file successfully
	vector<module*> mList;
	string tmp;
	readfile1(ifile, mList, tmp);

	output(mList);

	cleanMem(mList);

	ifile.close();
	return 0;
}