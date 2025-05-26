#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

Token *tokens;	// single linked list of tokens
Token *lastTk;		// the last token in list

int line=1;		// the current line in the input file

// adds a token to the end of the tokens list and returns it
// sets its code and line
Token *addTk(int code){
	Token *tk=safeAlloc(sizeof(Token));
	tk->code=code;
	tk->line=line;
	tk->next=NULL;
	if(lastTk){
		lastTk->next=tk;
		}else{
		tokens=tk;
		}
	lastTk=tk;
	return tk;
	}

char *extract(const char *begin,const char *end){
	if(begin > end){
		err("Begin > end: %p > %p", begin, end);
	}
	char *substring = safeAlloc(sizeof(char) * (end-begin + 1));
	int i = 0;
	for(i = 0; i < end - begin; i++){
		substring[i] = begin[i];
	}
	substring[i] = '\0';
	return substring;
	}

Token *tokenize(const char *pch){
	const char *start;
	Token *tk;
	for(;;){
		switch(*pch){
			//1 element
			case ' ':case '\t':pch++;break;
			case '\r':		// handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
				if(pch[1]=='\n')pch++;
				// fallthrough to \n
			case '\n':
				line++;
				pch++;
				break;
			case '\0':addTk(END);return tokens;
			case ',':addTk(COMMA);pch++;break;
			case '.':addTk(DOT);pch++;break;
			case ';':addTk(SEMICOLON);pch++;break;
			case '(':addTk(LPAR);pch++;break;
			case ')':addTk(RPAR);pch++;break;
			case '[':addTk(LBRACKET);pch++;break;
			case ']':addTk(RBRACKET);pch++;break;
			case '{':addTk(LACC);pch++;break;
			case '}':addTk(RACC);pch++;break;
			case '+':addTk(ADD);pch++;break;
			case '-':addTk(SUB);pch++;break;
			case '*':addTk(MUL);pch++;break;

			//2 elements
			case '/':
				if(pch[1] == '/'){
					pch+=2;
					while(pch[0] != '\0' && pch[0] != '\r' && pch[0] != '\n'){
						pch++;
					}
				}else{
					addTk(DIV);pch++;
				}
				break;

			case '=':
				if(pch[1]=='='){
					addTk(EQUAL);
					pch+=2;
					}else{
					addTk(ASSIGN);
					pch++;
					}
				break;
			case '&':
				if(pch[1] == '&'){
					addTk(AND);
					pch+=2;
				}else{
					pch++;
				}
				break;
			case '|':
				if(pch[1] == '|'){
					addTk(OR);
					pch+=2;
				}else{
					pch++;
				}
				break;
			case '!':
				if(pch[1] == '='){
					addTk(NOTEQ);
					pch+=2;
				}else{
					addTk(NOT);
					pch++;
				}
				break;
			case '<':
				if(pch[1] == '='){
					addTk(LESSEQ);
					pch+=2;
				}else{
					addTk(LESS);
					pch++;
				}
				break;
			case '>':
				if(pch[1] == '='){
					addTk(GREATEREQ);
					pch+=2;
				}else{
					addTk(GREATER);
					pch++;
				}
				break;
			//special
			default:
				if(isdigit(pch[0])){
					int pointer=1;
					while(isdigit(pch[pointer])){
						pointer++;
					}
					if(pch[pointer] == '.'){
						pointer++;
						if(isdigit(pch[pointer])){
							pointer++;
							while(isdigit(pch[pointer])){
								pointer++;
							}
							if(pch[pointer] == 'e' || pch[pointer] == 'E'){
								pointer++;
								if(pch[pointer] == '+' || pch[pointer] == '-'){
									pointer++;
								}
								if(isdigit(pch[pointer])){
									while(isdigit(pch[pointer])){
										pointer++;
									}
									tk = addTk(DOUBLE);
									sscanf(pch, "%lf",&tk->d);
									pch+=pointer;
								}else{
									err("Invalid double value missing exponent after e/E at line %d: %s", line, extract(pch, pch+pointer));
								}
							}else{
								tk = addTk(DOUBLE);
								sscanf(pch, "%lf",&tk->d);
								pch+=pointer;
							}
						}else{
							err("Invalid double value missing digits after . at line %d: %s",line, extract(pch, pch+pointer));
						}
					}else if(pch[pointer] == 'e' || pch[pointer] == 'E'){
						pointer++;
						if(pch[pointer] == '+' || pch[pointer] == '-'){
							pointer++;
						}
						if(isdigit(pch[pointer])){
							while(isdigit(pch[pointer])){
								pointer++;
							}
							tk = addTk(DOUBLE);
							sscanf(pch, "%lf",&tk->d);
							pch+=pointer;
						}else{
							err("Invalid double value missing exponent after e/E at line %d: %s", line, extract(pch, pch+pointer));
						}
					}else{
						tk = addTk(INT);
						sscanf(pch,"%d",&tk->i);
						pch+=pointer;
					}
				}else if(pch[0] == '\''){
					if(pch[1] != '\''){
						tk = addTk(CHAR);
						tk->c=pch[1];
						pch+=2;
					}else{
						err("Missing character at line %d", line);
					}
					if(pch[0] != '\''){
						err("Did not close char at line %d", line);
					}
					pch++;
				}else if(pch[0] == '"'){
					for(start=pch+=1;pch[0] != '"';pch++){
						if(pch[0] == '\0'){
							err("Did not close string at line %d",line);
						} 
					}
					char *text = extract(start, pch);
					tk = addTk(STRING);
					tk->text = text;
					pch++;
				}else if(isalpha(*pch)||*pch=='_'){
					for(start=pch++;isalnum(*pch)||*pch=='_';pch++){}
					char *text=extract(start,pch);
					if(strcmp(text,"char")==0){
						addTk(TYPE_CHAR);
					}else if(strcmp(text,"double") == 0){
						addTk(TYPE_DOUBLE);
					}else if(strcmp(text,"else")== 0){
						addTk(ELSE);
					}else if(strcmp(text,"if")== 0){
						addTk(IF);
					}else if(strcmp(text,"int")== 0){
						addTk(TYPE_INT);
					}else if(strcmp(text,"return")== 0){
						addTk(RETURN);
					}else if(strcmp(text,"struct")== 0){
						addTk(STRUCT);
					}else if(strcmp(text,"void")== 0){
						addTk(VOID);
					}else if(strcmp(text,"while")== 0){
						addTk(WHILE);
					}else{
						tk=addTk(ID);
						tk->text=text;
					}
				}else err("invalid char: %c (%d)",*pch,*pch);
				break;
			}
		}
	}

void showTokens(const Token *tokens, FILE *f){
	for(const Token *tk=tokens;tk;tk=tk->next){
		switch(tk->code) {
			case ID:
				fprintf(f,"%d	ID: %s\n",tk->line, tk->text);
				break;
			case TYPE_CHAR: 
				fprintf(f,"%d	%s\n",tk->line, "TYPE_CHAR");
				break;
			case TYPE_DOUBLE:
				fprintf(f,"%d	%s\n",tk->line, "TYPE_DOUBLE");
				break;
			case ELSE:
				fprintf(f,"%d	%s\n",tk->line, "ELSE");
				break;
			case IF:
				fprintf(f,"%d	%s\n",tk->line, "IF");
				break;
			case TYPE_INT:
				fprintf(f,"%d	%s\n",tk->line, "TYPE_INT");
				break;
			case RETURN:
				fprintf(f,"%d	%s\n",tk->line, "RETURN");
				break;
			case STRUCT:
				fprintf(f,"%d	%s\n",tk->line, "STRUCT");
				break;
			case VOID:
				fprintf(f,"%d	%s\n",tk->line, "VOID");
				break;
			case WHILE:
				fprintf(f,"%d	%s\n",tk->line, "WHILE");
				break;
			case INT:
				fprintf(f,"%d	INT: %d\n",tk->line, tk->i);
				break;
			case DOUBLE:
				fprintf(f,"%d	DOUBLE: %f\n",tk->line, tk->d);
				break;
			case CHAR:
				fprintf(f,"%d	CHAR: %c\n",tk->line, tk->c);
				break;
			case STRING:
				fprintf(f,"%d	STRING: %s\n",tk->line, tk->text);
				break;
			case COMMA:
				fprintf(f,"%d	%s\n",tk->line, "COMMA");
				break;
			case SEMICOLON:
				fprintf(f,"%d	%s\n",tk->line, "SEMICOLON");
				break;
			case LPAR:
				fprintf(f,"%d	%s\n",tk->line, "LPAR");
				break;
			case RPAR:
				fprintf(f,"%d	%s\n",tk->line, "RPAR");
				break;
			case LBRACKET:
				fprintf(f,"%d	%s\n",tk->line, "LBRACKET");
				break;
			case RBRACKET:
				fprintf(f,"%d	%s\n",tk->line, "RBRACKET");
				break;
			case LACC:
				fprintf(f,"%d	%s\n",tk->line, "LACC");
				break;
			case RACC:
				fprintf(f,"%d	%s\n",tk->line, "RACC");
				break;
			case END:
				fprintf(f,"%d	%s\n",tk->line, "END");
				break;
			case ASSIGN:
				fprintf(f,"%d	%s\n",tk->line, "ASSIGN");
				break;
			case EQUAL:
				fprintf(f,"%d	%s\n",tk->line, "EQUAL");
				break;
			case NOTEQ:
				fprintf(f,"%d	%s\n",tk->line, "NOTEQ");
				break;
			case LESS:
				fprintf(f,"%d	%s\n",tk->line, "LESS");
				break;
			case LESSEQ:
				fprintf(f,"%d	%s\n",tk->line, "LESSEQ");
				break;
			case GREATER:
				fprintf(f,"%d	%s\n",tk->line, "GREATER");
				break;
			case GREATEREQ:
				fprintf(f,"%d	%s\n",tk->line, "GREATEREQ");
				break;
			case ADD:
				fprintf(f,"%d	%s\n",tk->line, "ADD");
				break;
			case SUB:
				fprintf(f,"%d	%s\n",tk->line, "SUB");
				break;
			case MUL:
				fprintf(f,"%d	%s\n",tk->line, "MUL");
				break;
			case DIV:
				fprintf(f,"%d	%s\n",tk->line, "DIV");
				break;
			case DOT:
				fprintf(f,"%d	%s\n",tk->line, "DOT");
				break;
			case AND:
				fprintf(f,"%d	%s\n",tk->line, "AND");
				break;
			case OR:
				fprintf(f,"%d	%s\n",tk->line, "OR");
				break;
			case NOT:
				fprintf(f,"%d	%s\n",tk->line, "NOT");
				break;
			default:
				fprintf(f,"~~~~~~~~!!!!!Unknown token!!!!!~~~~~~~~\n");
				break;
		}
		
	}
}


char *getTokenString(char *s,const Token *tk){
	switch(tk->code) {
		case ID:
			sprintf(s,"%d	ID: %s",tk->line, tk->text);
			break;
		case TYPE_CHAR: 
			sprintf(s,"%d	%s",tk->line, "TYPE_CHAR");
			break;
		case TYPE_DOUBLE:
			sprintf(s,"%d	%s",tk->line, "TYPE_DOUBLE");
			break;
		case ELSE:
			sprintf(s,"%d	%s",tk->line, "ELSE");
			break;
		case IF:
			sprintf(s,"%d	%s",tk->line, "IF");
			break;
		case TYPE_INT:
			sprintf(s,"%d	%s",tk->line, "TYPE_INT");
			break;
		case RETURN:
			sprintf(s,"%d	%s",tk->line, "RETURN");
			break;
		case STRUCT:
			sprintf(s,"%d	%s",tk->line, "STRUCT");
			break;
		case VOID:
			sprintf(s,"%d	%s",tk->line, "VOID");
			break;
		case WHILE:
			sprintf(s,"%d	%s",tk->line, "WHILE");
			break;
		case INT:
			sprintf(s,"%d	INT: %d",tk->line, tk->i);
			break;
		case DOUBLE:
			sprintf(s,"%d	DOUBLE: %f",tk->line, tk->d);
			break;
		case CHAR:
			sprintf(s,"%d	CHAR: %c",tk->line, tk->c);
			break;
		case STRING:
			sprintf(s,"%d	STRING: %s",tk->line, tk->text);
			break;
		case COMMA:
			sprintf(s,"%d	%s",tk->line, "COMMA");
			break;
		case SEMICOLON:
			sprintf(s,"%d	%s",tk->line, "SEMICOLON");
			break;
		case LPAR:
			sprintf(s,"%d	%s",tk->line, "LPAR");
			break;
		case RPAR:
			sprintf(s,"%d	%s",tk->line, "RPAR");
			break;
		case LBRACKET:
			sprintf(s,"%d	%s",tk->line, "LBRACKET");
			break;
		case RBRACKET:
			sprintf(s,"%d	%s",tk->line, "RBRACKET");
			break;
		case LACC:
			sprintf(s,"%d	%s",tk->line, "LACC");
			break;
		case RACC:
			sprintf(s,"%d	%s",tk->line, "RACC");
			break;
		case END:
			sprintf(s,"%d	%s",tk->line, "END");
			break;
		case ASSIGN:
			sprintf(s,"%d	%s",tk->line, "ASSIGN");
			break;
		case EQUAL:
			sprintf(s,"%d	%s",tk->line, "EQUAL");
			break;
		case NOTEQ:
			sprintf(s,"%d	%s",tk->line, "NOTEQ");
			break;
		case LESS:
			sprintf(s,"%d	%s",tk->line, "LESS");
			break;
		case LESSEQ:
			sprintf(s,"%d	%s",tk->line, "LESSEQ");
			break;
		case GREATER:
			sprintf(s,"%d	%s",tk->line, "GREATER");
			break;
		case GREATEREQ:
			sprintf(s,"%d	%s",tk->line, "GREATEREQ");
			break;
		case ADD:
			sprintf(s,"%d	%s",tk->line, "ADD");
			break;
		case SUB:
			sprintf(s,"%d	%s",tk->line, "SUB");
			break;
		case MUL:
			sprintf(s,"%d	%s",tk->line, "MUL");
			break;
		case DIV:
			sprintf(s,"%d	%s",tk->line, "DIV");
			break;
		case DOT:
			sprintf(s,"%d	%s",tk->line, "DOT");
			break;
		case AND:
			sprintf(s,"%d	%s",tk->line, "AND");
			break;
		case OR:
			sprintf(s,"%d	%s",tk->line, "OR");
			break;
		case NOT:
			sprintf(s,"%d	%s",tk->line, "NOT");
			break;
		default:
			sprintf(s,"~~~~~~~~!!!!!Unknown token!!!!!~~~~~~~~");
			break;
	}
	return s;
	
}

const char *getTokenName(const Token *tk) {
    switch(tk->code) {
        case ID:          return "variable";
        case TYPE_CHAR:   return "char";
        case TYPE_DOUBLE: return "double";
        case ELSE:        return "else";
        case IF:          return "if";
        case TYPE_INT:    return "int";
        case RETURN:      return "return";
        case STRUCT:      return "struct";
        case VOID:        return "void";
        case WHILE:       return "while";
        case INT: {
			static char buffer[32];
			snprintf(buffer, sizeof(buffer), "%d", tk->i);
			return buffer;
			}
        case DOUBLE:{
			static char buffer[64];
			snprintf(buffer, sizeof(buffer), "%f", tk->d);
			return buffer;
			}
        case CHAR: {
			static char buffer[2];
			snprintf(buffer, sizeof(buffer), "%c", tk->c);
			buffer[1] = '\0';
			return buffer;
			}
        case STRING: {
			static char buffer[256];
			snprintf(buffer, sizeof(buffer), "%s", tk->text);
			buffer[1] = '\0';
			return buffer;
			}
        case COMMA:       return ",";
        case SEMICOLON:   return ";";
        case LPAR:        return "(";
        case RPAR:        return ")";
        case LBRACKET:    return "[";
        case RBRACKET:    return "]";
        case LACC:        return "{";
        case RACC:        return "}";
        case END:         return "end";
        case ASSIGN:      return "=";
        case EQUAL:       return "==";
        case NOTEQ:       return "!=";
        case LESS:        return "<";
        case LESSEQ:      return "<=";
        case GREATER:     return ">";
        case GREATEREQ:   return ">=";
        case ADD:         return "+";
        case SUB:         return "-";
        case MUL:         return "*";
        case DIV:         return "/";
        case DOT:         return ".";
        case AND:         return "&&";
        case OR:          return "||";
        case NOT:         return "!";
        default:          return "unknown";
    }
}
