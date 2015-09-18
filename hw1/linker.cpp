#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

struct symbol
{
	string name;
	int val;
};

struct instruction 
{
	string type="", buf="", opcode="", address="";
};


struct Module {
	int numSymbolDefine=0;
	vector<sym> defList;

	int numSymbolUse=0;
	vector<symbolUse> useList;

	int numInstructions=0;
	vector<instruction> instructionList;
};

void readFile(ifstream& ifile, vector<string>& vec1, vector<string>& vec2, string tmp)
{
	while(ifile>>tmp)  vec1.push_back(tmp); 
	ifile.clear();
	ifile.seekg(0);
	while(ifile>>tmp) vec2.push_back(tmp); 


	for(unsigned int i=0;i<vec1.size();i++) cout<<vec1[i]<<"_";
	cout<<endl;
	for(unsigned int i=0;i<vec2.size();i++) cout<<vec2[i]<<"_";
	cout<<endl;	
}

int main(int argc, char** argv)
{
	if(argc<2)  { cerr<<"Enter your input file."<<endl; return 1; }

	// open input file
	ifstream ifile(argv[1]);
	if(ifile.fail()) { cerr<<"Unable to open file "<<argv[1]<<endl; return 1; }

	// Here we open input file successfully
	vector<string> vec1, vec2;
	string tmp;

	readFile(ifile, vec1, vec2, tmp);



	ifile.close();
	return 0;
}