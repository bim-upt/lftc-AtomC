#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "ad.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"
#include "at.h"
#include "gc.h"

Token *iTk = NULL;		// the iterator in the tokens list
Token *consumedTk = NULL;		// the last consumed token
Symbol *owner = NULL;

bool unit();
bool structDef();
bool varDef();
bool fnDef();
bool typeBase(Type *t);
bool arrayDecl(Type *t);
bool fnParam();
bool stm();
bool stmCompound(bool);
bool expr(Ret *r);
bool exprAssign(Ret *r);
bool exprOr(Ret *r);
bool exprOrPrim(Ret *r);
bool exprAnd(Ret *r);
bool exprAndPrim(Ret *r);
bool exprEq(Ret *r);
bool exprEqPrim(Ret *r);
bool exprRel(Ret *r);
bool exprRelPrim(Ret *r);
bool exprAdd(Ret *r);
bool exprAddPrim(Ret *r);
bool exprMul(Ret *r);
bool exprMulPrim(Ret *r);
bool exprCast(Ret *r);
bool exprUnary(Ret *r);
bool exprPostfix(Ret *r);
bool exprPostfixPrim(Ret *r);
bool exprPrimary(Ret *r);



#define MAX_LEN 50

typedef struct Node {
    char data[MAX_LEN];
    struct Node* next;
}Node;

Node* parserPath = NULL;

void push(const char* str) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (!newNode) {
        printf("Memory allocation failed\n");
        return;
    }
    strncpy(newNode->data, str, MAX_LEN - 1);
    newNode->data[MAX_LEN - 1] = '\0';  // Ensure null-termination
    newNode->next = parserPath;
    parserPath = newNode;
}

void pop() {
    if (parserPath == NULL) {
        printf("List is empty\n");
        return;
    }
    Node* temp = parserPath;
    parserPath = parserPath->next;
    free(temp);
}


FILE *logFile = NULL;
void parserSetLog(FILE *f){
	logFile = f;
}

void logPath(FILE *logfile, Token *tk);

void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	
	//logPath(stderr,iTk);
	
	exit(EXIT_FAILURE);
	}

char s[200];
void logPath(FILE *logFile, Token *tk){
	if(consumedTk != NULL){
		fprintf(logFile, "%-20s",getTokenString(s,consumedTk));
	}
	Node* current = parserPath;
    while (current != NULL) {
        fprintf(logFile, "%s <- ", current->data);
        current = current->next;
    }
	fprintf(logFile,"\n");
	
}


void restore(Token *start, Token *lastConsumed, FILE *logFile, Instr *startInstr){
	iTk = start;
	if(owner)delInstrAfter(startInstr);
	//consumedTk = lastConsumed;
	//fprintf(stderr,"%f %f\n\n", iTk, consumedTk);
	if(iTk != NULL){
		fprintf(logFile, "\nRESTORED to:");
		logPath(logFile, iTk);
		fprintf(logFile,"\n");
	}else{
		fprintf(logFile, "<- unit");
	}
}


bool consume(int code){
	if(iTk->code==code){
		consumedTk=iTk;
		iTk=iTk->next;
		//fprintf(logFile, "%s:	%s",branch,getTokenString(s,consumedTk));		
		logPath(logFile, consumedTk);
		
		return true;
		}
	return false;
	}

//exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(Ret *r){ char *br = "exprPrimary";push(br);
	Token *start = iTk; Instr
*startInstr=owner?lastInstr(owner->fn.instr):NULL;
	if(consume(ID)){
		Token *tkName = consumedTk;
		Symbol *s=findSymbol(tkName->text);
		if(!s)tkerr("undefined id: %s",tkName->text);
		if(consume(LPAR)){
			if(s->kind!=SK_FN)tkerr("only a function can be called");
			Ret rArg;
			Symbol *param=s->fn.params;
			if(expr(&rArg)){
				if(!param)tkerr("too many arguments in function call");
				if(!convTo(&rArg.type,&param->type))tkerr("in call, cannot convert the argument type to the parameter type");

				addRVal(&owner->fn.instr,rArg.lval,&rArg.type);
				insertConvIfNeeded(lastInstr(owner->fn.instr),&rArg.type,&param->type);

				param=param->next;
				while(consume(COMMA)){
					if(!expr(&rArg)){
						tkerr("Missing expression after , in function call");
					}
					if(!param)tkerr("too many arguments in function call");
					if(!convTo(&rArg.type,&param->type))tkerr("in call, cannot convert the argument type to the parameter type");

					addRVal(&owner->fn.instr,rArg.lval,&rArg.type);
					insertConvIfNeeded(lastInstr(owner->fn.instr),&rArg.type,&param->type);

					param=param->next;
				}
			}
			if(consume(RPAR)){
				if(param)tkerr("too few arguments in function call");
				*r=(Ret){s->type,false,true};

				if(s->fn.extFnPtr){
					addInstr(&owner->fn.instr,OP_CALL_EXT)->arg.extFnPtr=s->fn.extFnPtr;
				}else{
					addInstr(&owner->fn.instr,OP_CALL)->arg.instr=s->fn.instr;
				}

				pop();return true;
			}else{

				


				tkerr("Missing closing ) or invalid parameters in function call");
			}
		}else{
			if(s->kind==SK_FN)tkerr("a function can only be called");
			*r=(Ret){s->type,true,s->type.n>=0};

			//unsure
				if(s->kind==SK_VAR){
					if(s->owner==NULL){ // global variables
						addInstr(&owner->fn.instr,OP_ADDR)->arg.p=s->varMem;
					}else{ // local variables
						switch(s->type.tb){
							case TB_INT:addInstrWithInt(&owner->fn.instr,OP_FPADDR_I,s->varIdx+1);break;
							case TB_DOUBLE:addInstrWithInt(&owner->fn.instr,OP_FPADDR_F,s->varIdx+1);break;
						}
					}
				}
				if(s->kind==SK_PARAM){
					switch(s->type.tb){
						case TB_INT:
							addInstrWithInt(&owner->fn.instr,OP_FPADDR_I,s->paramIdx-symbolsLen(s->owner->fn.params)-1); break;
						case TB_DOUBLE:
							addInstrWithInt(&owner->fn.instr,OP_FPADDR_F,s->paramIdx-symbolsLen(s->owner->fn.params)-1); break;
					}
				}
		}
		pop();return true;
	}
	if(consume(INT)){
		Token *ct = consumedTk;

		*r=(Ret){{TB_INT,NULL,-1},false,true};

		addInstrWithInt(&owner->fn.instr,OP_PUSH_I,ct->i);
		pop();return true;
	}
	if(consume(DOUBLE)){
		Token *ct = consumedTk;
		*r=(Ret){{TB_DOUBLE,NULL,-1},false,true};

		addInstrWithDouble(&owner->fn.instr,OP_PUSH_F,ct->d);
		pop();return true;
	}
	if(consume(CHAR)){
		Token *ct = consumedTk;
		*r=(Ret){{TB_CHAR,NULL,-1},false,true};
		pop();return true;
	}
	if(consume(STRING)){
		Token *ct = consumedTk;
		*r=(Ret){{TB_CHAR,NULL,0},false,true};
		pop();return true;
	}
	if(consume(LPAR)){
		if(expr(r)){
			if(consume(RPAR)){
				pop();return true;
			}else{
				tkerr("Missing closing ) or invalid parameters");
			}
		}else{
			restore(start, consumedTk, logFile, startInstr);
			return false;
			//tkerr("Missing expression after ( in function call");
		}
	}
	pop();return false;
}


// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
//prefix unic: tot
bool typeBase(Type *t){ char *br = "typeBase";push(br);
	t->n = -1;
	if(consume(TYPE_INT)){
		t->tb = TB_INT;
		pop();return true;
		}
	if(consume(TYPE_DOUBLE)){
		t->tb = TB_DOUBLE;
		pop();return true;
		}
	if(consume(TYPE_CHAR)){
		t->tb = TB_CHAR;
		pop();return true;
		}
	if(consume(STRUCT)){
		if(consume(ID)){
			Token *tkName = consumedTk;
			t->tb =TB_STRUCT;
			t->s=findSymbol(tkName->text);
			if(!t->s)tkerr("structura nedefinita: %s",tkName->text);
			pop();return true;
		}else{
			tkerr("Must specify the struct name");
		}
	}
	pop();return false;
	}

//arrayDecl: LBRACKET INT? RBRACKET
//prefix unic: [
bool arrayDecl(Type *t){ char *br = "arrayDecl";push(br);
	if(consume(LBRACKET)){
		if(consume(INT)){
			Token *tkSize=consumedTk;
			t->n=tkSize->i;
		}else{
			t->n=0; // array fara dimensiune: int v[]
		}
		if(consume(RBRACKET)){
			pop();return true;
		}else{
			tkerr("Missing ] in array declaration or invalid expression inside [...]");
		}
	}
	pop();return false;
}




//exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID | exprPrimary =>
//
//exprPostfix: exprPrimary exprPostfixPrim
//exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | ε
bool exprPostfix(Ret *r){ char *br = "exprPostfix";push(br);
	if(exprPrimary(r)){
		if(exprPostfixPrim(r)){
			pop();return true;
		}
	}
	pop();return false;
}

bool exprPostfixPrim(Ret *r){ char *br = "exprPostfixPrim";push(br);
	Token *start = iTk; Instr
*startInstr=owner?lastInstr(owner->fn.instr):NULL;
	Token *lastConsumed = consumedTk;


	if(consume(LBRACKET)){
		Ret idx;
		if(expr(&idx)){
			if(consume(RBRACKET)){
				if(r->type.n<0)tkerr("only an array can be indexed");
				Type tInt={TB_INT,NULL,-1};
				if(!convTo(&idx.type,&tInt))tkerr("the index is not convertible to int");
				r->type.n=-1;
				r->lval=true;
				r->ct=false;
				if(exprPostfixPrim(r)){
					pop();return true;
				}
			}else{
				restore(start, lastConsumed, logFile, startInstr);
				pop();return true;
			}
		}else{
			restore(start, lastConsumed, logFile, startInstr);
			pop();return true;
		}
	}

	if(consume(DOT)){
		if(consume(ID)){
			Token *tkName = consumedTk;
			if(r->type.tb!=TB_STRUCT)tkerr("a field can only be selected from a struct");
			Symbol *s=findSymbolInList(r->type.s->structMembers,tkName->text);
			if(!s)tkerr("the structure %s does not have a field %s",r->type.s->name,tkName->text);
			*r=(Ret){s->type,true,s->type.n>=0};
			if(exprPostfixPrim(r)){
				pop();return true;
			}
		}else{
			restore(start, lastConsumed, logFile, startInstr);
			pop();return true;
		}
	}

	pop();return true;
}


//exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(Ret *r){ char *br = "exprUnary";push(br);
	
	if(consume(SUB)){
		if(exprUnary(r)){
			if(!canBeScalar(r))tkerr("unary - must have a scalar operand");
			r->lval=false;
			r->ct=true;
			pop();return true;
		}else{
			tkerr("Unary expression missing after -");	
		}
	}

	if(consume(NOT)){
		if(exprUnary(r)){
			if(!canBeScalar(r))tkerr("unary ! must have a scalar operand");
			r->lval=false;
			r->ct=true;
			pop();return true;
		}else{
			tkerr("Unary expression missing after !");	
		}
	}

	if(exprPostfix(r)){
		pop();return true;
	}

	pop();return false;
}

//exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(Ret *r){ char *br = "exprCast";push(br);
	if(consume(LPAR)){
		Type t;Ret op;
		if(typeBase(&t)){
			if(arrayDecl(&t)){}
			if(consume(RPAR)){
				if(exprCast(&op)){
					if(t.tb==TB_STRUCT)tkerr("cannot convert to a struct type");
					if(op.type.tb==TB_STRUCT)tkerr("cannot convert a struct");
					if(op.type.n>=0&&t.n<0)tkerr("an array can be converted only to another array");
					if(op.type.n<0&&t.n>=0)tkerr("a scalar can be converted only to another scalar");
					*r=(Ret){t,false,true};
					pop();return true;
				}else{
					tkerr("Cast must continue with unary expression or another cast");
				}
			}else{
				tkerr("Did not close ) in cast");
			}
		}else{
			tkerr("Missing type for cast");
		}
	}

	if(exprUnary(r)){
		pop();return true;
	}

	pop();return false;
}


//varDef: typeBase ID arrayDecl? SEMICOLON
//prefix unic: typeBase ID (SEMICOLON | LBRAKET)
bool varDef(){ char *br = "varDef";push(br);
	Token *start = iTk; Instr
*startInstr=owner?lastInstr(owner->fn.instr):NULL;
	Token *lastConsumed = consumedTk;
	
	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName = consumedTk;

			if(arrayDecl(&t)){
				{if(t.n==0)tkerr("a vector variable must have a specified dimension");}
			}
			if(consume(SEMICOLON)){

				Symbol *var=findSymbolInDomain(symTable,tkName->text);
				if(var)tkerr("symbol redefinition: %s",tkName->text);
				var=newSymbol(tkName->text,SK_VAR);
				var->type=t;
				var->owner=owner;
				addSymbolToDomain(symTable,var);
				if(owner){
					switch(owner->kind){
						case SK_FN:
							var->varIdx=symbolsLen(owner->fn.locals);
							addSymbolToList(&owner->fn.locals,dupSymbol(var));
							break;
						case SK_STRUCT:
							var->varIdx=typeSize(&owner->type);
							addSymbolToList(&owner->structMembers,dupSymbol(var));
							break;
					}
				}else{
					var->varMem=safeAlloc(typeSize(&t));
				}

				pop();return true;
			}else{
				tkerr("Missing ; after variable declaration");
			}
		}else{
			//look-ahead
			//if(iTk->code == SEMICOLON || iTk->code == LBRACKET){
				//tkerr("Variable declaration missing name");
			//}
			if(consumedTk->code == ID){
				tkerr("Struct variable missing name");
			}
			tkerr("Variable missing name");
			restore(start, lastConsumed, logFile, startInstr);
			pop();return false;
		}
	}
	pop();return false;
}

//fnParam: typeBase ID arrayDecl?
//prefix unic: typeBase
bool fnParam(){ char *br = "fnParam";push(br);
	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName = consumedTk;

			if(arrayDecl(&t)){t.n = 0;}

			Symbol *param=findSymbolInDomain(symTable,tkName->text);
			if(param)tkerr("symbol redefinition: %s",tkName->text);
			param=newSymbol(tkName->text,SK_PARAM);
			param->type=t;
			param->owner=owner;
			param->paramIdx=symbolsLen(owner->fn.params);
			// parametrul este adaugat atat la domeniul curent, cat si la parametrii fn
			addSymbolToDomain(symTable,param);
			addSymbolToList(&owner->fn.params,dupSymbol(param));

			pop();return true;
		}else{
			tkerr("Missing parameter name");
		}
	}
	pop();return false;
}



//stmCompound: LACC ( varDef | stm )* RACC
//prefix unic: LACC
bool stmCompound(bool newDomain){ char *br = "stmCompound";push(br);
	if(consume(LACC)){
		if(newDomain)pushDomain();

		while(varDef() || stm()){};
		if(consume(RACC)){
			if(newDomain)dropDomain();

			pop();return true;
		}else{
			tkerr("Invalid statement, need to close with } or end with ; found instead: %s", getTokenName(iTk));
		}
	}
	pop();return false;
}

//exprOr: exprOr OR exprAnd | exprAnd =>
//
//exprOr: exprAnd exprOrPrim
//exprOrPrim: OR exprAnd exprOrPrim | ε
bool exprOr(Ret *r){ char *br = "exprOr";push(br);
	if(exprAnd(r)){
		if(exprOrPrim(r)){
			pop();return true;
		}
	}
	pop();return false;
}

bool exprOrPrim(Ret *r){ char *br = "exprOrPrim";push(br);
	//Token *start = iTk;
	//Token *lastConsumed = consumedTk;
	if(consume(OR)){
		Ret right;
		if(exprAnd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for ||");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if(exprOrPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after || ");
			pop();return true;
		}
	}
	pop();return true;
}

//exprAnd: exprAnd AND exprEq | exprEq =>
//
//exprAnd: exprEq exprAndPrim
//exprAndPrim: AND exprEq exprAndPrim | ε
//
bool exprAnd(Ret *r){ char *br = "exprAnd";push(br);
	if(exprEq(r)){
		if(exprAndPrim(r)){
			pop();return true;
		}
	}
	pop();return false;
}

bool exprAndPrim(Ret *r){ char *br = "exprAndPrim";push(br);
	Token *start = iTk; Instr
*startInstr=owner?lastInstr(owner->fn.instr):NULL;
	Token *lastConsumed = consumedTk;
	if(consume(AND)){
		Ret right;
		if(exprEq(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for &&");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if(exprAndPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after &&");
			restore(start, lastConsumed, logFile, startInstr);
			pop();return true;
		}
	}
	pop();return true;
}

//exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel =>
//
//exprEq: exprRel exprEqPrim
//exprEqPrim: (EQUAL | NOTEQ) exprRel exprEqPrim | ε
bool exprEq( Ret *r){ char *br = "exprEq";push(br);
	if(exprRel(r)){
		if(exprEqPrim(r)){
			pop();return true;
		}
	}
	pop();return false;
}

bool exprEqPrim( Ret *r){ char *br = "exprEqPrim";push(br);
	//Token *start = iTk;
	//Token *lastConsumed = consumedTk;


	if(consume(EQUAL)){
		Ret right;
		if(exprRel(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for ==");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if(exprEqPrim(r)){
				pop();return true;
			}
		}else{
			
			tkerr("Invalid expression after =");
		}
	}

	if(consume(NOTEQ)){
		Ret right;
		if(exprRel(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for !=");
			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if(exprEqPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after !=");
		}
	}


	/*
	if(consume(EQUAL) || consume(NOTEQ)){
		if(exprRel()){
			if(exprEqPrim()){
				pop();return true;
			}
		}else{
			restore(start, lastConsumed, logFile, startInstr);
			pop();return true;
		}
	}*/

	
	pop();return true;
}

//exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd =>
//
//exprRel: exprAdd exprRelPrim
//exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | ε
bool exprRel(Ret *r){ char *br = "exprRel";push(br);
	
	if(exprAdd(r)){
		if(exprRelPrim(r)){
			pop();return true;
		}
	}
	pop();return false;
}

bool exprRelPrim(Ret *r){ char *br = "exprRelPrim";push(br);
	//Token *start = iTk;
	//Token *lastConsumed = consumedTk;

	Token *op;
	if(consume(LESS)){
		op = consumedTk;
		Ret right;
		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);
		if(exprAdd(&right)){
			Type tDst;	
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for <");

			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op->code){
				case LESS:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_LESS_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_LESS_F);break;
					}
				break;
			}


			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if(exprRelPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after <");
		}
	}

	if(consume(LESSEQ)){
		op = consumedTk;
		Ret right;
		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);
		if(exprAdd(&right)){
			Type tDst;	
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for <=");

			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op->code){
				case LESS:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_LESS_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_LESS_F);break;
					}
				break;
			}


			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if(exprRelPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after <=");
		}
	}

	if(consume(GREATER)){
		op = consumedTk;
		Ret right;
		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);
		if(exprAdd(&right)){
			Type tDst;	
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for >");


			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op->code){
				case LESS:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_LESS_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_LESS_F);break;
					}
				break;
			}


			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if(exprRelPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after >");
		}
	}


	if(consume(GREATEREQ)){
		op = consumedTk;
		Ret right;
		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);
		if(exprAdd(&right)){
			Type tDst;	
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for >=");

			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op->code){
				case LESS:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_LESS_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_LESS_F);break;
					}
				break;
			}


			*r=(Ret){{TB_INT,NULL,-1},false,true};
			if(exprRelPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after >=");
		}
	}


	/*	
	if(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)){
		if(exprAdd()){
			if(exprRelPrim()){
				pop();return true;
			}
		}else{
			restore(start, lastConsumed, logFile, startInstr);
			pop();return true;
		}
	}*/
	pop();return true;
}

//exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul => 
//
//exprAdd: exprMul exprAddPrim
//exprAddPrim:( ADD | SUB ) exprMul exprAddPrim | ε

bool exprAdd(Ret *r){ char *br = "exprAdd";push(br);
	if(exprMul(r)){
		if(exprAddPrim(r)){
			pop();return true;
		}
	}
	pop();return false;
}

bool exprAddPrim(Ret *r){ char *br = "exprAddPrim";push(br);
	//Token *start = iTk;
	//Token *lastConsumed = consumedTk;
	Token *op;
	if(consume(ADD)){
		op = consumedTk;
		Ret right;
		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);
		if(exprMul(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for +");


			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op->code){
				case ADD:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_ADD_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_ADD_F);break;
					}
				break;
				case SUB:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_SUB_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_SUB_F);break;
					}
				break;
			}


			*r=(Ret){tDst,false,true};
			if(exprAddPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after +");
		}
	}

	if(consume(SUB)){
		Ret right;
		op = consumedTk;
		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);
		if(exprMul(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for -");

			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op->code){
				case ADD:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_ADD_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_ADD_F);break;
					}
				break;
				case SUB:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_SUB_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_SUB_F);break;
					}
				break;
			}

			*r=(Ret){tDst,false,true};
			if(exprAddPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after -");
		}
	}

	/*
	if(consume(ADD) || consume(SUB)){
		if(exprMul()){
			if(exprAddPrim()){
				pop();return true;
			}
		}else{
			restore(start, lastConsumed, logFile, startInstr);
			pop();return true;
		}
	}
	*/
	pop();return true;
}

//exprMul: exprMul ( MUL | DIV ) exprCast | exprCast => 
//
//exprMul: exprCast exprMulPrim
//exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | ε
bool exprMul(Ret *r){ char *br = "exprMul";push(br);
	if(exprCast(r)){
		if(exprMulPrim(r)){
			pop();return true;
		}
	}
	pop();return false;
}

bool exprMulPrim(Ret *r){ char *br = "exprMulPrim";push(br);
	Token *op;
	if(consume(MUL)){
		op = consumedTk;
		Ret right;

		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);

		if(exprCast(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for *");


			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op->code){
				case MUL:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_MUL_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_MUL_F);break;
					}
				break;
				case DIV:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_DIV_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_DIV_F);break;
					}
				break;
			}

			*r=(Ret){tDst,false,true};
			if(exprMulPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after *");
		}
	}

	if(consume(DIV)){
		op = consumedTk;
		Ret right;

		Instr *lastLeft=lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr,r->lval,&r->type);

		if(exprCast(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type,&right.type,&tDst))tkerr("invalid operand type for /");


			addRVal(&owner->fn.instr,right.lval,&right.type);
			insertConvIfNeeded(lastLeft,&r->type,&tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&right.type,&tDst);
			switch(op->code){
				case MUL:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_MUL_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_MUL_F);break;
					}
				break;
				case DIV:
					switch(tDst.tb){
						case TB_INT:addInstr(&owner->fn.instr,OP_DIV_I);break;
						case TB_DOUBLE:addInstr(&owner->fn.instr,OP_DIV_F);break;
					}
				break;
			}


			*r=(Ret){tDst,false,true};
			if(exprMulPrim(r)){
				pop();return true;
			}
		}else{
			tkerr("Invalid expression after /");
		}
	}

	/*
	Token *start = iTk;
	Token *lastConsumed = consumedTk;
	if(consume(MUL) || consume(DIV)){
		if(exprCast()){
			if(exprMulPrim()){
				pop();return true;
			}
		}else{
			restore(start, lastConsumed, logFile, startInstr);
			pop();return true;
		}
	}*/
	pop();return true;
}






//exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign(Ret *r){ char *br = "exprAssign";push(br);
	Token *start = iTk; Instr
*startInstr=owner?lastInstr(owner->fn.instr):NULL;
	Token *lastConsumed = consumedTk;
	Ret rDst;
	if(exprUnary(&rDst)){
		if(consume(ASSIGN)){
			if(exprAssign(r)){
				if(!rDst.lval)tkerr("the assign destination must be a left-value");
				if(rDst.ct)tkerr("the assign destination cannot be constant");
				if(!canBeScalar(&rDst))tkerr("the assign destination must be scalar");
				if(!canBeScalar(r))tkerr("the assign source must be scalar");
				if(!convTo(&r->type,&rDst.type))tkerr("the assign source cannot be converted to destination");
				r->lval=false;
				r->ct=true;


				addRVal(&owner->fn.instr,r->lval,&r->type);
				insertConvIfNeeded(lastInstr(owner->fn.instr),&r->type,&rDst.type);
				switch(rDst.type.tb){
					case TB_INT:addInstr(&owner->fn.instr,OP_STORE_I);break;
					case TB_DOUBLE:addInstr(&owner->fn.instr,OP_STORE_F);break;
				}


				pop();return true;
			}else{
				tkerr("Assignment needs to continue with another assignment or with an expression");
			}
		}else{
			restore(start, lastConsumed, logFile, startInstr);
		}
	}

	if(exprOr(r)){
		pop();return true;
	}

	pop();return false;
}


//expr: exprAssign
bool expr(Ret *r){ char *br = "expr";push(br);
	if(exprAssign(r)){
		pop();return true;
	}
	pop();return false;
}


//stm: stmCompound | IF LPAR expr RPAR stm ( ELSE stm )? | WHILE LPAR expr RPAR stm | RETURN expr? SEMICOLON | expr? SEMICOLON
bool stm(){ char *br = "stm";push(br);
	Token *start = iTk; Instr
*startInstr=owner?lastInstr(owner->fn.instr):NULL;
	Token *lastConsumed = consumedTk;
	Ret rCond,rExpr;
	if(stmCompound(true)){
		pop();return true;
	}


	if(consume(IF)){
		if(consume(LPAR)){
			if(expr(&rCond)){

				if(!canBeScalar(&rCond))tkerr("the if condition must be a scalar value");

				if(consume(RPAR)){
					addRVal(&owner->fn.instr,rCond.lval,&rCond.type);
					Type intType={TB_INT,NULL,-1};
					insertConvIfNeeded(lastInstr(owner->fn.instr),&rCond.type,&intType);
					Instr *ifJF=addInstr(&owner->fn.instr,OP_JF);
					if(stm()){
						if(consume(ELSE)){
							Instr *ifJMP=addInstr(&owner->fn.instr,OP_JMP);
							ifJF->arg.instr=addInstr(&owner->fn.instr,OP_NOP);
							if(stm()){
								ifJMP->arg.instr=addInstr(&owner->fn.instr,OP_NOP);
							}else{
								tkerr("Missing body after else");
							}
						}else{
							ifJF->arg.instr=addInstr(&owner->fn.instr,OP_NOP); //unsure
						}
						
						pop();return true;
					}else{
						tkerr("Missing body from if statement");
					}
				}else{
					tkerr("Missing ) from if statement or invalid expression");
				}
			}else{
				tkerr("Missing expression from if statement");
			}
		}else{
			tkerr("Missing ( from if statement");
		}
	}

	if(consume(WHILE)){
		Instr *beforeWhileCond=lastInstr(owner->fn.instr);
		if(consume(LPAR)){
			if(expr(&rCond)){

				if(!canBeScalar(&rCond))tkerr("the while condition must be a scalar value");

				if(consume(RPAR)){
					addRVal(&owner->fn.instr,rCond.lval,&rCond.type);
					Type intType={TB_INT,NULL,-1};
					insertConvIfNeeded(lastInstr(owner->fn.instr),&rCond.type,&intType);
					Instr *whileJF=addInstr(&owner->fn.instr,OP_JF);
					if(stm()){
						addInstr(&owner->fn.instr,OP_JMP)->arg.instr=beforeWhileCond->next;
						whileJF->arg.instr=addInstr(&owner->fn.instr,OP_NOP);
						pop();return true;
					}else{
						tkerr("Missing body from while");
					}
				}else{
					tkerr("Missing ) from while or invalid expression");
				}
			}else{
				tkerr("Missing expression from while");
			}
		}else{
			tkerr("Missing ( form while");
		}
	}

	if(consume(RETURN)){
		if(expr(&rExpr)){
			if(owner->type.tb==TB_VOID)tkerr("a void function cannot return a value");
			if(!canBeScalar(&rExpr))tkerr("the return value must be a scalar value");
			if(!convTo(&rExpr.type,&owner->type))tkerr("cannot convert the return expression type to the function return type");


			addRVal(&owner->fn.instr,rExpr.lval,&rExpr.type);
			insertConvIfNeeded(lastInstr(owner->fn.instr),&rExpr.type,&owner->type);
			addInstrWithInt(&owner->fn.instr,OP_RET,symbolsLen(owner->fn.params));
		}else{
			if(owner->type.tb!=TB_VOID)tkerr("a non-void function must return a value");

			addInstr(&owner->fn.instr,OP_RET_VOID);
		}
		
		
		

		if(consume(SEMICOLON)){
			pop();return true;
		}else{
			tkerr("Missing ; after pop();return");
		}
	}
	
	

	if(expr(&rExpr)){
		if(rExpr.type.tb!=TB_VOID)addInstr(&owner->fn.instr,OP_DROP);
	}
	if(consume(SEMICOLON)){
		pop();return true;
	}
	restore(start, lastConsumed, logFile, startInstr);
	pop();return false;
}





//fnDef: ( typeBase | VOID ) ID LPAR ( fnParam ( COMMA fnParam )* )? RPAR stmCompound
//prefix unic: STRUCT ID ID LPAR, VOID
bool fnDef(){ char *br = "fnDef";push(br);
	Token *start = iTk; Instr
*startInstr=owner?lastInstr(owner->fn.instr):NULL;
	Token *lastConsumed = consumedTk;
	Type t;

	bool passedTypeBase = false;
	bool passedVoid = false;


	Token *tkName;

	if(typeBase(&t)){
		if(consume(ID)){
			tkName = consumedTk;
			passedTypeBase = true;
		}
	}

	if(consume(VOID)){
		t.tb=TB_VOID;
		if(passedTypeBase){
			tkerr("A function can't both return a type and return void");
		}
		if(consume(ID)){
			tkName = consumedTk;
			passedVoid = true;
		}else{
			tkerr("Function missing name");
		}
	}

	

	if(!passedTypeBase && !passedVoid){
		restore(start, lastConsumed, logFile, startInstr);
		pop();return false;
	}	
	
	

	if(consume(LPAR)){
		Symbol *fn=findSymbolInDomain(symTable,tkName->text);
		if(fn)tkerr("symbol redefinition: %s",tkName->text);
		fn=newSymbol(tkName->text,SK_FN);
		fn->type=t;
		addSymbolToDomain(symTable,fn);
		owner=fn;
		pushDomain();

		if(fnParam()){
			while(consume(COMMA)){
				if(!fnParam()){
					tkerr("Missing parameter after comma, or invalid parameter");
				}
			}
		}
		if(consume(RPAR)){
			addInstr(&fn->fn.instr,OP_ENTER);
			if(stmCompound(false)){
				fn->fn.instr->arg.i=symbolsLen(fn->fn.locals);
				if(fn->type.tb==TB_VOID)
					addInstrWithInt(&fn->fn.instr,OP_RET_VOID,symbolsLen(fn->fn.params));
				dropDomain();
				owner=NULL;

				
				pop();return true;
			}else{
				tkerr("Missing function body");
			}
		}else{
			tkerr("Must close function parameters with ) or invalid parameter");
		}
	}else{
		if(passedVoid){
			tkerr("Must open function parameter list with (");
		}
	}
	
	restore(start, lastConsumed, logFile, startInstr);
	pop();return false;
}


//structDef: STRUCT ID LACC varDef* RACC SEMICOLON
//prefix unic: STRUCT ID LACC
bool structDef(){ char *br = "structDef";push(br);
	Token *start = iTk; Instr
*startInstr=owner?lastInstr(owner->fn.instr):NULL;
	Token *lastConsumed = consumedTk;
	if(consume(STRUCT)){
		if(consume(ID)){
			Token *tkName = consumedTk; 
			if(consume(LACC)){
				{
					Symbol *s=findSymbolInDomain(symTable,tkName->text);
					if(s)tkerr("symbol redefinition: %s",tkName->text);
					s=addSymbolToDomain(symTable,newSymbol(tkName->text,SK_STRUCT));
					s->type.tb=TB_STRUCT;
					s->type.s=s;
					s->type.n=-1;
					pushDomain();
					owner=s;
					
				}
				while(varDef()){}
				if(consume(RACC)){
					if(consume(SEMICOLON)){
						{
							owner=NULL;
							dropDomain();
						}
						pop();return true;
					}else{
						tkerr("Struct declaration missing ;");
					}
				}else{
					tkerr("Struct declaration missing }");
				}
			}else{
				restore(start, lastConsumed, logFile, startInstr);
				pop();return false;
			}
		}else{
			//look-ahead
			//if(iTk->code == LACC){
			//	tkerr("Struct declaration missing struct name");
			//}else{
				restore(start, lastConsumed, logFile, startInstr);
				pop();return false;
			//}
			
		}
	}
	pop();return false;
}


// unit: ( structDef | fnDef | varDef )* END
bool unit(){ char *br = "unit";push(br);
	for(;;){
		if(structDef()){}
		else if(fnDef()){}
		else if(varDef()){}
		else break;
		}

	if(consume(END)){
		pop();return true;
		}
	
	pop();return false;
	}

void parse(Token *tokens){
	iTk=tokens;
	if(!unit())tkerr("syntax error, invalid variable/struct/function declaration, found: %s", getTokenName(iTk));
	printf("Success!\n");
}
