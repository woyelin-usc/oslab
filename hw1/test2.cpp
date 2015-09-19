#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

int lineNum=0;
int offset=0;


bool isDigit(string& str)
{
	for(int i=0;i<str.length();i++) {
		if(str[i]>'9' || str[i]<'0') return false;
	}
	return true;
}

int main(int argc, char** argv)
{
	string str="12345a";
	cout<<isDigit(str)<<endl;
	// string word;
	// ifstream ifile(argv[1]);
	// while(!ifile.eof()) {
	// 	char ch = ifile.peek();
	// 	if(ch=='\n') { ifile.get();lineNum++; offset=0; }
	// 	else if(ch==' '|| ch=='\t')  { ifile.get(); offset++; }
	// 	else {
	// 		ifile>>word;
	// 		cout<<word<<" @ Line "<<lineNum<<" offset "<<offset<<endl;
	// 		offset+=word.length();
	// 	}
	// }
}


// int main(int argc, char** argv)
// {
// 	string line;
// 	ifstream ifile(argv[1]);
// 	while(getline(ifile, line)) {
// 		lineNum++;
// 		istringstream buf(line);
// 		string word;
// 		while(buf>>word) {
// 			cout<<word<< "@ Line " << lineNum<<" offset ";
// 			if(buf.eof()) cout<<line.length()-word.length()<<endl;
// 			else cout<<buf.tellg()-word.length()<<endl;
// 		}

// 	}

// 	return 0;
// }

