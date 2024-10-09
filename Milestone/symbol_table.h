#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

// Forward declaration of structures
typedef struct Symbol Symbol;
typedef struct Scope Scope;

// Symbol structure
struct Symbol {
    char* name;
    char* type;
    int isArg;  // Checks for the value used from argument
    int isAssigned;  // Checks the memory location is already set
    int globalVar;
    int scope;
    int args_num;
    int memLoc;
    Symbol* next; // Pointer to the next symbol in the same scope
};

// Scope structure
struct Scope {
    int level;
    Symbol* symbols; // Head pointer to symbols in this scope
    Scope* prev; // Pointer to the previous scope in the hierarchy
    Scope* next; // Pointer to the next scope in the hierarchy
};

// Function prototypes for managing scopes
Scope* createScope(int level, Scope* parent);
void removeSymbolsInScope(Scope* scope);

// Function prototypes for managing symbols
Symbol* createSymbol(char* name, char* type, int scope, int args_num);
void insertSymbol(Scope* scope, char* name, char* type, int args_num);
Symbol* findSymbol(Scope* scope, char* name);
void printSymbolsInScope(Scope* scope);
void freeScopes(Scope* scope);
void cleanupGlobalScope();
void updateSymbol(Scope* scope, char* name, int memoryLoc);

// Initialization function for global scope
void initGlobalScope(void);

#endif // SYMBOL_TABLE_H
