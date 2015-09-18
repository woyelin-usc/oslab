#include <iostream>
#include <string>
#include <vector>

#include <unordered_map>

using namespace std;

struct obj
{
	vector<int*> vec;
	obj(){}
};


int main(int argc, char** argv)
{
	obj* myobj = new obj();
	myobj->vec.push_back(new int(5));
	return 0;
}