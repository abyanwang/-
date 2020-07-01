#include <iostream>
#include <fstream>
#include <vector>
#include "core/Scanner.cpp"
#include "core/Utils.cpp"
#include "core/Log.h"

using namespace std;

int main(int argc,char* argv[]) {
	if(argc == 1){
    	puts("����ļ�");
    	return 0;
	}
    if(argc > 2){
    	puts("һ��ֻ�ܽ���һ���ļ�");
		return 0; 
	} 
    Log log(cerr);
    ifstream fin(argv[1]);
    if(argv[1][4] != '1')freopen("test_out.txt","w",stdout);
	else freopen("test1_out.txt","w",stdout); 
    try {
        vector<Token> list = Scanner::scan(fin, log);
        // ���
        for (Token i : list){
			cout << "< " << Utils::tokenToName(i.type) << ", " << i.token << " >" << endl;	
		}
    } catch (const string& err) {
        cerr << endl << err << endl;
    }
}
