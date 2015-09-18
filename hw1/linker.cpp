#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

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

void readDefList(ifstream& ifile, vector<module*>& mList, int idx, string& tmp)
{
	mList[idx]->numSymDef=stoi(tmp);
	for(int i=0;i<mList[idx]->numSymDef;i++) {
		ifile>>tmp;
		string newName=tmp;
		ifile>>tmp;
		int newVal=stoi(tmp)+mList[idx]->base;

		mList[idx]->defList.push_back(new symbol(newName, newVal));
	}
}

void readUseList(ifstream& ifile, vector<module*>& mList, int idx, string& tmp)
{
	mList[idx]->numSymUse=stoi(tmp);
	for(int i=0;i<mList[idx]->numSymUse;i++) {
		ifile>>tmp;
		mList[idx]->useList.push_back(tmp);
	}
}

void readInstrList(ifstream& ifile,vector<module*>& mList, int idx, string& tmp)
{
	mList[idx]->numInstr=stoi(tmp);
	for(int i=0;i<mList[idx]->numInstr;i++) {
		ifile>>tmp;
		string newType=tmp;
		ifile>>tmp;
		string newBuf=tmp;
		mList[idx]->instrList.push_back(new instruction(newType, newBuf));
	}
}

void readfile1(ifstream& ifile, vector<module*>& mList, string& tmp)
{
	// i: idx of new module in the mList
	int i=0;
	while(ifile>>tmp) {
		mList.push_back(new module());

		// configure new module base address
		if(!i) mList[i]->base=0;
		else mList[i]->base=mList[i-1]->base + mList[i-1]->numInstr;
		
		readDefList(ifile, mList,i,tmp);
		
		ifile>>tmp;
		readUseList(ifile, mList,i,tmp);

		ifile>>tmp;
		readInstrList(ifile, mList, i, tmp);

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

	ifile.close();
	return 0;
}