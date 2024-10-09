/*
    Author: John Ko
    Course: CSC-453
    Description: This program will generate symbol table to use linked list with two structures, which are
                Scope and Symbol.
                + args_num is added to struct of Symbol to check the number of arguments to the functions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

Scope* globalScope = NULL; // Global scope

// Function to create a new scope
Scope* createScope(int level, Scope* parent) {
    Scope* newScope = (Scope*)malloc(sizeof(Scope));
    newScope->level = level;
    newScope->symbols = NULL; // No symbols initially
    newScope->prev = parent; // Set parent scope
    newScope->next = NULL; // No next scope initially
    if (parent) {
        parent->next = newScope; // Link the new scope as the next of the parent
    }
    return newScope;
}

// Function to create a new symbol
Symbol* createSymbol(char* name, char* type, int scope, int args_num) {
    Symbol* newSymbol = (Symbol*)malloc(sizeof(Symbol));
    newSymbol->name = strdup(name);
    newSymbol->type = strdup(type);
    newSymbol->scope = scope;
    newSymbol->args_num = args_num;
    newSymbol->isAssigned = 0;
    newSymbol->isArg = 0;
    newSymbol->globalVar = 0;
    newSymbol->next = NULL;
    return newSymbol;
}

// Function to insert a symbol into a specific scope
void insertSymbol(Scope* scope, char* name, char* type, int args_num) {
    Symbol* newSymbol = createSymbol(name, type, scope->level, args_num);
    newSymbol->next = scope->symbols;
    scope->symbols = newSymbol;
}

// Function to find a symbol in a specific scope
Symbol* findSymbol(Scope* scope, char* name) {
    for (Symbol* sym = scope->symbols; sym != NULL; sym = sym->next) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL; // Symbol not found
}

// Function to remove all symbols in a specific scope
void removeSymbolsInScope(Scope* scope) {
    Symbol* current = scope->symbols;
    while (current != NULL) {
        Symbol* toDelete = current;
        current = current->next;
        free(toDelete->name);
        free(toDelete->type);
        free(toDelete);
    }
    scope->symbols = NULL; // All symbols removed
}

// Function to recursively free all scopes and their symbols
void freeScopes(Scope* scope) {
    if (scope == NULL) return;
    
    // Free symbols within the current scope
    removeSymbolsInScope(scope);
    
    // Recursively free child scopes if any
    if (scope->next != NULL) {
        freeScopes(scope->next);
    }
    
    // Finally, free the current scope itself
    free(scope);
}

// Function to initialize the cleanup process from the global scope
void cleanupGlobalScope() {
    if (globalScope != NULL) {
        freeScopes(globalScope);
        globalScope = NULL; // Ensure the global pointer is set to NULL after cleanup
    }
}

// Function to print all symbols in a specific scope to test the code.
void printSymbolsInScope(Scope* scope) {
    printf("Symbols in scope level %d:\n", scope->level);
    for (Symbol* sym = scope->symbols; sym != NULL; sym = sym->next) {
        printf("  Name: %s, Type: %s\n", sym->name, sym->type);
    }
}

void updateSymbol(Scope* scope, char* name, int memoryLoc) {
    Symbol* sym = findSymbol(scope, name);
    sym->memLoc = memoryLoc;
    sym->isAssigned = 1;
}

// Initialize the global scope at the beginning
void initGlobalScope() {
    globalScope = createScope(0, NULL); // Global scope with level 0 and no parent
}

