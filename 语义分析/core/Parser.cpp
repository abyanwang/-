#include "Parser.h"
#include "Utils.h"

void Parser::parse() {
	//�����﷨�� 
    root = program();
    // ���ڵ�����������Ƿ���ȷ 
    checkType(root);
    
    //check statement
    checkStatementType(root);
    
    //����д�throw  
    if (log.hasError()) {
        std::stringstream msg;
        msg << "You have " << log.getErrorCount() << " errors.";
        throw msg.str();
    }
}

bool Parser::match(TokenType expected, bool flag) {
	//token���Լ��� 
    if (token.type == expected) {
        last_token = token;
        token = Stream.nextToken();
        
        
        return true;
    } else {
        if (flag) {
            if (token.type == TokenType::NO_MORE_TOKEN) {
                log.parseError("expected token " + Utils::tokenToName(expected)
                    + ", but there is no more token", last_token.line, last_token.offset);
            } else {
                log.parseError("unexpected token: " + token.token + ", "
                    + Utils::tokenToName(expected) + " expected", token.line, token.offset);
            }
        }
        return false;
    }
}

TreeNode* Parser::program() {
	//�����ʱ���õ�һ��token 
    token = Stream.nextToken();
    
    //�������� 
    declarations();
    
    //�����﷨���Ĺ��� 
    TreeNode* body = stmt_sequence();
    if (token.type != TokenType::NO_MORE_TOKEN)
        log.parseError("��Ч�ķ��ţ�֮������뽫������", token.line, token.offset);
    
    //���������﷨�� 
    return body;
}


//�������� 
void Parser::declarations() {
	//��ʱ��type 
    VarType type = VarType::VT_VOID;
    while (match(TokenType::KEY_INT) || match(TokenType::KEY_BOOL) || match(TokenType::KEY_STRING)) {
        switch (last_token.type) {
            case TokenType::KEY_INT:
                type = VarType::VT_INT;
                break;
            case TokenType::KEY_BOOL:
                type = VarType::VT_BOOL;
                break;
            case TokenType::KEY_STRING:
                type = VarType::VT_STRING;
                break;
            default:
                log.parseError("the token can not be parsed to a type: " + last_token.token, last_token.line, last_token.offset);
                break;
        }
        do {
            // ������ȡһ����ʶ��  int A �ж��ǲ���A����id 
            if (!match(TokenType::ID, true))
                break;

            // ������ű�
            //���� �Ƿ����ظ������ı��� 
            
            if(table.count(last_token.token)){
            	stringstream msg;
            	msg << "the variable " << last_token.token << " has already declared in line "<<table[last_token.token].lines[0];
				log.parseError(msg.str(), last_token.line, last_token.offset);
			}else{
				Symbol symbol;
				symbol.type = type;
				symbol.lines.push_back(last_token.line);
				table[last_token.token] = symbol;
			}
			
        } while(match(TokenType::OP_COMMA));
        match(TokenType::OP_SEMICOLON); // �ֺſ��п���  ��ǰ������������ 
    }	
}

// stmt-sequence -> statement {; stmt-sequence } 
TreeNode* Parser::stmt_sequence() {
    TreeNode* node = TreeNode::createNode(NodeType::STMT_SEQUENCE);
    node->children[0] = statement();  //��ڵ� 
    if (node->children[0] == nullptr) {
        delete node;
        return nullptr;
    }
    match(TokenType::OP_SEMICOLON); // �ֺſ��п���
    node->children[1] = stmt_sequence();
    if (node->children[1] == nullptr) {
        TreeNode* tmp = node->children[0];
        node->children[0] = nullptr;
        delete node;
        node = tmp;
    }
    return node;
}


// statement -> if-stmt | repeat-stmt | assign-stmt | read-stmt | write-stmt | while-stmt
TreeNode* Parser::statement() {
    TreeNode* node = nullptr;
    switch (token.type) {
        case TokenType::KEY_IF:
            node = if_stmt();
            break;
        case TokenType::KEY_READ:
            node = read_stmt();
            break;
        case TokenType::KEY_REPEAT:
            node = repeat_stmt();
            break;
        case TokenType::KEY_WRITE:
            node = write_stmt();
            break;
        case TokenType::KEY_WHILE:
            node = while_stmt();
            break;
        case TokenType::ID:
            node = assign_stmt();
            break;
        default:
            break;
    }
    return node;
}


//if-stmt -> if  logical-or-exp then stmt-sequence [else stmt-sequence] end 
TreeNode* Parser::if_stmt() {
    TreeNode* node = TreeNode::createNode(NodeType::IF_STMT);
    match(TokenType::KEY_IF, true);
    node->children[0] = or_exp();
    match(TokenType::KEY_THEN, true);
    node->children[1] = stmt_sequence();
    if (match(TokenType::KEY_ELSE))
        node->children[2] = stmt_sequence();
    match(TokenType::KEY_END, true);
    return node;
}


//repeat-stmt -> repeat stmt-sequence until logical-or-exp 
TreeNode* Parser::repeat_stmt() {
    TreeNode* node = TreeNode::createNode(NodeType::REPEAT_STMT);
    match(TokenType::KEY_REPEAT, true);
    node->children[0] = stmt_sequence();
    match(TokenType::KEY_UNTIL, true);
    node->children[1] = or_exp();
    return node;
}

//assign-stmt -> identifier := logical-or-exp 
TreeNode* Parser::assign_stmt() {
    TreeNode* node = TreeNode::createNode(NodeType::ASSIGN_STMT);
    //���������������� 
    if (token.type == TokenType::ID)
        node->children[0] = factor();
    match(TokenType::OP_ASSIGN, true);
    node->token = new Token(last_token);
    node->children[1] = or_exp();
    return node;
}

// read-stmt -> read identifier 
TreeNode* Parser::read_stmt() {
    TreeNode* node = TreeNode::createNode(NodeType::READ_STMT);
    match(TokenType::KEY_READ, true);
    node->token = new Token(last_token);
    if (token.type == TokenType::ID)
        node->children[0] = factor();
    else if (token.type == TokenType::NO_MORE_TOKEN)
        log.parseError("unexpected token: " + token.token, last_token.line, last_token.offset);
    else
        log.parseError("unexpected token: " + token.token, token.line, token.offset);
    return node;
}

// write-stmt -> write logical-or-exp
TreeNode* Parser::write_stmt() {
    TreeNode* node = TreeNode::createNode(NodeType::WRITE_STMT);
    match(TokenType::KEY_WRITE, true);
    node->token = new Token(last_token);
    node->children[0] = or_exp();
    return node;
}

// while-stmt -> while logical-or-exp do stmt-sequence end 
TreeNode* Parser::while_stmt() {
    TreeNode* node = TreeNode::createNode(NodeType::WHILE_STMT);
    match(TokenType::KEY_WHILE, true);
    node->children[0] = or_exp();
    match(TokenType::KEY_DO, true);
    node->children[1] = stmt_sequence();
    match(TokenType::KEY_END, true);
    return node;
}


// logical-or-exp -> logical-and-exp [ or logical-or-exp ] 
TreeNode* Parser::or_exp() {
    TreeNode* node = TreeNode::createNode(NodeType::OR_EXP);
    node->children[0] = and_exp();
    if (match(TokenType::KEY_OR)) {
        node->token = new Token(last_token);
        node->children[1] = or_exp();
    }
    else {
    	//ֻ��ǰһ��and_exp��������ڵ���ӽڵ�͸ýڵ�ϲ� 
        TreeNode* tmp = node->children[0];
        node->children[0] = nullptr;
        delete node;
        node = tmp;
    }
    return node;
}

//logical-and-exp -> comparison-exp [ and logical-and-exp] 
TreeNode* Parser::and_exp() {
    TreeNode* node = TreeNode::createNode(NodeType::AND_EXP);
    node->children[0] = comparison_exp();
    if (match(TokenType::KEY_AND)) {
        node->token = new Token(last_token);
        node->children[1] = and_exp();
    }
    else {
    	//ֻ��ǰһ��and_exp��������ڵ���ӽڵ�͸ýڵ�ϲ�
        TreeNode* tmp = node->children[0];
        node->children[0] = nullptr;
        delete node;
        node = tmp;
    }
    return node;
}


// comparison-exp -> add-exp [ comparison-op comparison-exp ]
TreeNode* Parser::comparison_exp() {
    TreeNode* node = TreeNode::createNode(NodeType::NONE);
    node->children[0] = add_sub_exp();
    
    //comparison-op  ->  < | = | > | >= | <=  
    switch (token.type) {
    	// < 
        case TokenType::OP_LSS:
            match(TokenType::OP_LSS, true);
            node->token = new Token(last_token);
            // comparison-exp
            node->type = NodeType::LT_EXP;
            node->children[1] = comparison_exp();
            break;
        // <=    
        case TokenType::OP_LEQ:
            match(TokenType::OP_LEQ, true);
            node->token = new Token(last_token);
            // comparison-exp
            node->type = NodeType::LE_EXP;
            node->children[1] = comparison_exp();
            break;
        // >
        case TokenType::OP_GTR:
            match(TokenType::OP_GTR, true);
            node->token = new Token(last_token);
            // comparison-exp
            node->type = NodeType::GT_EXP;
            node->children[1] = comparison_exp();
            break;
        // >=    
        case TokenType::OP_GEQ:
            match(TokenType::OP_GEQ, true);
            node->token = new Token(last_token);
            
            // comparison-exp
            node->type = NodeType::GE_EXP;
            node->children[1] = comparison_exp();
            break;
        // =         
        case TokenType::OP_EQU:
            match(TokenType::OP_EQU, true);
            node->token = new Token(last_token);
            
            // comparison-exp
            node->type = NodeType::EQ_EXP;
            node->children[1] = comparison_exp();
            break;
        default: {
            TreeNode* tmp = node->children[0];
            node->children[0] = nullptr;
            delete node;
            node = tmp;
            break;
        }
    }
    return node;
}


// add-exp	-> mul-exp [ addop add-exp ]
TreeNode* Parser::add_sub_exp() {
    TreeNode* node = TreeNode::createNode(NodeType::NONE);
    node->children[0] = mul_div_exp();
    switch (token.type) {
        case TokenType::OP_ADD:
            match(TokenType::OP_ADD, true);
            node->token = new Token(last_token);
            node->type = NodeType::PLUS_EXP;
            node->children[1] = add_sub_exp();
            break;
        case TokenType::OP_SUB:
            match(TokenType::OP_SUB, true);
            node->token = new Token(last_token);
            node->type = NodeType::SUB_EXP;
            node->children[1] = add_sub_exp();
            break;
        default: {
            TreeNode* tmp = node->children[0];
            node->children[0] = nullptr;
            delete node;
            node = tmp;
            break;
        }
    }
    return node;
}


// mul-exp	-> factor [ mulop mul-exp ]
TreeNode* Parser::mul_div_exp() {
    TreeNode* node = TreeNode::createNode(NodeType::NONE);
    node->children[0] = factor();
    switch (token.type) {
        case TokenType::OP_MUL:
            match(TokenType::OP_MUL, true);
            node->token = new Token(last_token);
            node->type = NodeType::MUL_EXP;
            node->children[1] = mul_div_exp();
            break;
        case TokenType::OP_DIV:
            match(TokenType::OP_DIV, true);
            node->token = new Token(last_token);
            node->type = NodeType::DIV_EXP;
            node->children[1] = mul_div_exp();
            break;
        default: {
            TreeNode* tmp = node->children[0];
            node->children[0] = nullptr;
            delete node;
            node = tmp;
            break;
        }
    }
    return node;
}


// factor  -> number | string | identifier | true | false| ( logical-or-exp )
TreeNode* Parser::factor() {
    TreeNode* node = TreeNode::createNode(NodeType::FACTOR);
    switch (token.type) {
    	
    	//identifier
        case TokenType::ID: {
            node->token = new Token(token);
            
            //������� 
            if(table.count(token.token)){
            	node->varType = table[token.token].type;
            	table[token.token].lines.push_back(token.line);
			}else{
				log.parseError("the symbol " + token.token + " is not declared", token.line, token.offset);
			}
			
			
            match(TokenType::ID, true);
            break;
        }
        
        //number 
        case TokenType::NUMBER:
            node->token = new Token(token);
            node->varType = VarType::VT_INT;
            match(TokenType::NUMBER, true);
            break;
        //string    
        case TokenType::STRING:
            node->token = new Token(token);
            node->varType = VarType::VT_STRING;
            match(TokenType::STRING, true);
            break;
        //true    
        case TokenType::KEY_TRUE:
            node->token = new Token(token);
            node->varType = VarType::VT_BOOL;
            match(TokenType::KEY_TRUE, true);
            break;
        //false    
        case TokenType::KEY_FALSE:
            node->token = new Token(token);
            node->varType = VarType::VT_BOOL;
            match(TokenType::KEY_FALSE, true);
            break;
        // ���� 
        case TokenType::OP_LP:
        	//logical-or-exp 
            node->type = NodeType::OR_EXP;
            match(TokenType::OP_LP, true);
            node = or_exp();
            match(TokenType::OP_RP, true);
            break;
        //not ����������    
        case TokenType::KEY_NOT:
            node->type = NodeType::NOT_EXP;
            match(TokenType::KEY_NOT, true);
            node->token = new Token(last_token);
            node->children[0] = factor();
            break;
        default:
            if (token.type == TokenType::NO_MORE_TOKEN)
                log.parseError("unexpected token: " + token.token, last_token.line, last_token.offset);
            else
                log.parseError("unexpected token: " + token.token, token.line, token.offset);
            delete node;
            node = nullptr;
            break;
    }
    return node;
}


//dfs
void Parser::checkType(TreeNode *node) {
    if (node == NULL)return;
    for (TreeNode* child : node->children){
    	if (child != NULL)checkType(child);
	}
    
	//Ҷ�ڵ�    
    switch (node->type) {
    	//��С�Ƚ� boolֵ ���������� 
        case NodeType::LT_EXP:
        case NodeType::LE_EXP:
        case NodeType::GT_EXP:
        case NodeType::GE_EXP:
            if (node->children[0] == nullptr || node->children[1] == nullptr){
            	log.typeError("ȱ�ٲ�����", node->token->line, node->token->offset);
			}else if (node->children[0]->varType != VarType::VT_INT){
            	log.typeError("�Ƚ�������Ĳ���������int����", node->children[0]->token->line, node->children[0]->token->offset);
			}else if (node->children[1]->varType != VarType::VT_INT){
            	log.typeError("�Ƚ�������Ĳ���������int����", node->children[1]->token->line, node->children[1]->token->offset);
			}
            node->varType = VarType::VT_BOOL;
            if (!node->token) {
                node->token = new Token;
                node->token->line = node->children[1]->token->line;
                node->token->offset = node->children[1]->token->offset;
            }
            break;
        case NodeType::EQ_EXP:
            if (node->children[0] == nullptr || node->children[1] == nullptr){
            	log.typeError("ȱ�ٲ�����", node->token->line, node->token->offset);
			} else if (node->children[0]->varType != node->children[1]->varType){
            	log.typeError("����������Ͳ�һ��", node->children[1]->token->line, node->children[1]->token->offset);
			}
			
            node->varType = VarType::VT_BOOL;
            if (!node->token) {
                node->token = new Token;
                node->token->line = node->children[1]->token->line;
                node->token->offset = node->children[1]->token->offset;
            }
            break;
        //bool ֵ����  ����������    
        case NodeType::OR_EXP:
        case NodeType::AND_EXP:
            if (node->children[0] == nullptr || node->children[1] == nullptr){
            	log.typeError("ȱ�ٲ�����", node->token->line, node->token->offset);
			}else if (node->children[0]->varType != VarType::VT_BOOL){
				log.typeError("�߼�������Ĳ���������bool����", node->children[0]->token->line, node->children[0]->token->offset);
			}else if (node->children[1]->varType != VarType::VT_BOOL){
				log.typeError("�߼�������Ĳ���������bool����", node->children[1]->token->line, node->children[1]->token->offset);
			}
                
            node->varType = VarType::VT_BOOL;
            if (!node->token) {
                node->token = new Token;
                node->token->line = node->children[1]->token->line;
                node->token->offset = node->children[1]->token->offset;
            }
            break;
        //boolֵ����  ����������    
        case NodeType::NOT_EXP:
            if (node->children[0] == nullptr){
            	log.typeError("ȱ�ٲ�����", node->token->line, node->token->offset);
			}else if (node->children[0]->varType != VarType::VT_BOOL){
				log.typeError("�߼�������Ĳ���������bool����", node->children[0]->token->line, node->children[0]->token->offset);
			}
            node->varType = VarType::VT_BOOL;
            if (!node->token) {
                node->token = new Token;
                node->token->line = node->children[0]->token->line;
                node->token->offset = node->children[0]->token->offset;
            }
            break;
        //�Ӽ��˳������ +-*/    
        case NodeType::PLUS_EXP:
        case NodeType::SUB_EXP:
        case NodeType::MUL_EXP:
        case NodeType::DIV_EXP:
            if (node->children[0] == nullptr || node->children[1] == nullptr){
            	log.typeError("ȱ�ٲ�����", node->token->line, node->token->offset);
			}else if (node->children[0]->varType != VarType::VT_INT){
				log.typeError("����������Ĳ���������int����", node->children[0]->token->line, node->children[0]->token->offset);
			}else if (node->children[1]->varType != VarType::VT_INT){
				log.typeError("����������Ĳ���������int����", node->children[1]->token->line, node->children[1]->token->offset);
			}
			
            node->varType = VarType::VT_INT;
            if (!node->token) {
                node->token = new Token;
                node->token->line = node->children[1]->token->line;
                node->token->offset = node->children[1]->token->offset;
            }
            break;
        //boolֵ�����ж�    
        case NodeType::IF_STMT:
        case NodeType::WHILE_STMT:
            if (node->children[0]->varType != VarType::VT_BOOL){
            	log.typeError("IF/WHILE�����б�ʽ����bool����", node->children[0]->token->line, node->children[0]->token->offset);
			}
            node->varType = VarType::VT_BOOL;
            break;
        //��ֵ���    
        case NodeType::ASSIGN_STMT:
            if (node->children[0] == nullptr || node->children[1] == nullptr){
            	log.typeError("ȱ�ٲ�����", node->token->line, node->token->offset);
			}
            else if (node->children[0]->varType != node->children[1]->varType){
            	log.typeError("��ֵ�������Ͳ�һ��", node->children[1]->token->line, node->children[1]->token->offset);
			}
            node->varType = node->children[0]->varType;
            break;
        default:
            break;
    }
}

void Parser::checkStatementType(TreeNode *node, StatementType parentStmtType) {
    if (!node)return;
    switch (node->type) {
        case NodeType::ASSIGN_STMT:
            node->stmtType = StatementType::ASSIGN;
            break;
        case NodeType::IF_STMT:
        case NodeType::WHILE_STMT:
            node->stmtType = StatementType::CONDITION;
            break;
        default:
            node->stmtType = parentStmtType;
            break;
    }
    for (TreeNode* child : node->children){
    	if (child)checkStatementType(child, node->stmtType);
	}
}
