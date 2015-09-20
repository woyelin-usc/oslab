#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>

using namespace std;

void pass(string* strs)
{
	for(int i=0;i<3;i++) cout<<strs[i]<<endl;

}

int main(int argc, char** argv)
{
	string strs[3];
	strs[0]="aaa";
	strs[1]="bbb";
	strs[2]="ccc";

	pass(strs);
}