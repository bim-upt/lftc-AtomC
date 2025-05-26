#include <stdlib.h>
#include <stdio.h>
#include "lexer.h"
#include "utils.h"
#include "parser.h"
#include "ad.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"
#include "at.h"
#include "vm.h"

void initialize(FILE *logFile){
    parserSetLog(logFile);
}

int main(int argc, char **args){
    if(argc < 2){
        err("Did not provide input file: ./main input");
    }
    
    char *file = loadFile(args[1]);
    Token *result = tokenize(file);
    FILE *f = fopen("log.txt", "w");
    if(f == NULL){
        perror("Could not create log file");
    }
    fprintf(f, "Tokens:\n\n");
    showTokens(result, f);

    initialize(f);
    fprintf(f,"\n---------------------------------------\nConsumed tokens\n---------------------------------------\n");

    pushDomain();
    vmInit();
    parse(result);
    //showDomain(symTable, "global");
    //Instr *testCode = genTestProgram2();
    //run(testCode);


    Symbol *symMain=findSymbolInDomain(symTable,"main");
    if(!symMain)err("missing main function");
    Instr *entryCode=NULL;
    addInstr(&entryCode,OP_CALL)->arg.instr=symMain->fn.instr;
    addInstr(&entryCode,OP_HALT);
    run(entryCode);


    dropDomain();


    return 0;
    
}