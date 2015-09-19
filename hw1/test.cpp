#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
	ifstream ifile(argv[1]);
	string word;
	while(ifile>>word) {
		cout<<word<<" ";
	}
	cout<<endl;
	ifile.clear();
	ifile.seekg(0);
	while(ifile>>word) {
		cout<<word<<" ";
	}
	cout<<endl;
	return 0;
}