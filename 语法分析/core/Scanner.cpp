#include <string>
#include <sstream>
#include "Scanner.h"
#include "Utils.h"
using namespace std;

vector<Token> Scanner::scan(ifstream& fin, Log& log) {
    Scanner instance(fin, log);
    if (log.hasError()) {
        stringstream msg;
        msg << "You have " << log.getErrorCount() << " errors.";
        throw msg.str();
    }
    return instance.getTokens();
}

void Scanner::newLine() {
    lineNumber ++;
    charNumber = 0;
    type = TokenType::NONE;
    isSign = false;
}


//��ǰ��word�ر�
void Scanner::figureWord() {
    //�½�һ����ʱ��Token
    Token token;
    token.token = word;
    token.type = type;
    token.line = lineNumber;
    token.offset = charNumber - 1;

    //enum
    TokenType t = Utils::nameToToken(word);
    
    //ת��������type 
    if (token.type == TokenType::NONE)token.type = t;
    else if (token.type == TokenType::ID && t != TokenType::NONE)token.type = t;

    //if word is error     
    if (token.type == TokenType::NONE)log.error("unknown token: " + word, lineNumber, charNumber - 1);
    
    //if word is not annotation ����token list
    if (token.type != TokenType::ANNOTATION)list.push_back(token);
    word.clear();
    type = TokenType::NONE;
    isSign = false;
}



void Scanner::analyse(){
	if (!in.is_open()) {
        log.error("can not load file.", 1, 0);
        return;
    }
    char ch;
    lineNumber = 1;
    charNumber = 0;
    isSign = false;
    while(1){
    	in.get(ch);
    	if (in.fail() || in.eof()) {
            if (!word.empty()){
            	figureWord();
			}
            //����while    
            break;
        }
        charNumber++;
        
        //����ǰchΪsign ����û�ж�Ϊsign ����word�����  ����sign 
        if(word.size() != 0&&type != TokenType::ANNOTATION&& type != TokenType::STRING&&
		isSign == false&&Utils::isValidSign(ch)){
			figureWord();
		}
		
		
		//��ǰ��type�Ѿ�Ϊsign(ǰһ���ַ�Ϊsign)  :1 ;  ���ǵ���sign := ; ����һ����ʱ����Ϊ:= 
        if(isSign&&Utils::nameToToken(word) != TokenType::NONE &&
                    Utils::nameToToken(word + ch) == TokenType::NONE){
            figureWord();        	
		}
		
		if(word.size() == 0){
			if (ch == '{'){
				type = TokenType::ANNOTATION;
			}else if (ch == '\''){
            	type = TokenType::STRING;
			}else if (isdigit(ch)){
				type = TokenType::NUMBER;
			}else if (isalpha(ch)){
				type = TokenType::ID;
			}else if (Utils::isValidSign(ch)){
				isSign = true;
			}else if (!Utils::isSeparator(ch)){
				log.error("unknown symbol: " + ch, lineNumber, charNumber);
			}
		}else{
			if (type == TokenType::STRING && ch == '\'') {
                word += ch;
                figureWord();
                continue;
            }
            if (type == TokenType::ANNOTATION && ch == '}') {
                word += ch;
                figureWord();
                continue;
            }
		}
		
		//����ָ�� ����ȥ����ע�Ͷ��е����� 
		if (type != TokenType::ANNOTATION && type != TokenType::STRING) {
            if (Utils::isSeparator(ch)) {
                if (!word.empty())figureWord();
                if (ch == '\n')newLine();
                continue;
            }
        }
		
		
		if (type == TokenType::NUMBER && !isdigit(ch))
            log.error("here must be a digit: " + ch, lineNumber, charNumber);
        if (type == TokenType::ID && !isalpha(ch) && !isdigit(ch))
            log.error("here must be a letter: " + ch, lineNumber, charNumber);
            
        word += ch;
	}
	
	if (!word.empty())figureWord();
}
