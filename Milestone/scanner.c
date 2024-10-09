/*
    Course: CSC-453
    Author: John Ko
    Purpose: This program will check the tokens, which is what scanner does.
            It will read in standard input when the program is run. 
            It will ignore all the whitespaces and comments that look like the regular
            comment sign in c and java. After that,  it will detect lexeme from standard
            input to check character by character. At the end, it will follow enum structure
            of identifiers or keywords and get the token and its lexeme. 

            ++ Had changes to count line number when the line of standard input ended.
*/

#include <stdio.h>
#include "scanner.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *lexeme; /* to communicate with parser */
int lval; /* to communicate with parser */
int line_num = 1; 

/* This function skips the whitespace and comment when it is detected. */
void skip_WS_and_comment() {
    char ch;
    int in_comment = 0;

    while ((ch = getchar()) != EOF) {  // Find white spaces or comment until the stdin ends.
        if (ch == '\n') {
            line_num++;
            continue;
        }

        if (isspace(ch)) {  // When whitespace is detected it skips to the next character.
            continue;
        }

        if (ch == '/') {  // It detects the beginning of the comment
            char next_ch = getchar();
            if (next_ch == '*') {  // Checks the comment starts
                in_comment = 1;  // It means the comment is opened
                continue;
            } else {
                ungetc(next_ch, stdin);  // When it is not a comment, go back to previous character.
            }
        }

        // It checks the closing comment to check the comment is on and the character is *.
        if (ch == '*' && in_comment) {
            char next_ch = getchar();
            if (next_ch == '/') {  // Check it will close the comment
                in_comment = 0;  // It means the comment ended
                continue;
            } else {
                ungetc(next_ch, stdin);  // When it does not close the comment, go back to the stdin
            }
        }

        // When the character is not in the comment and it is valid character, go back to get_token function.
        if (!in_comment) {
            ungetc(ch, stdin);
            return;
        }
    }
}

/* 
This function gets a token character by character to match the pattern of standard input to see
the lexeme and token are valid.
*/
Token get_token() {
    char ch;
    char buffer[256];  // Character data structure to store the lexeme.

    skip_WS_and_comment();  // Function call to check the current character is whitespace or comment.
    ch = getchar();  // Get a next character.

    if (ch == EOF) return EOF;  // When there is no more characters left, it will end the program.

    // This if-statement will check the first character starts with digit, so it can get integer constant.
    if (isdigit(ch)) {
        int index = 0;
        lval = ch - '0';  // Store the first digit
        buffer[index] = ch;
        index++;
        while (isdigit(ch = getchar())) {  // It will find or digits until the next character is not a digit.
            lval = lval * 10 + (ch - '0');  // It moves digit to left hand side to put another digit at the end.
            buffer[index] = ch;
            index++;
        }
        buffer[index] = '\0'; 

        if (ch != EOF) {  // When there are more characters left, it will go back to the position of standard input.
            ungetc(ch, stdin);
        }

        lexeme = strdup(buffer);  // Copy the value to lexeme.
        return INTCON;
    }

    // This if-statement will detect string arguments that can be an identifier or keywords of if, while int, else, or return.
    int index = 0;
    if (isalpha(ch)) {  // Check if the character starts with alphabet.
        buffer[index] = ch;
        index++;
        ch = getchar();
        while (isalpha(ch) || isdigit(ch) || ch == '_') {  // Check the next characters are alphabet, digit, or underscore.
            buffer[index] = ch;
            index++;
            ch = getchar();
        }
        buffer[index] = '\0';

        if (ch != EOF) {  // When there are more characters left, it will go back to the position of standard input.
            ungetc(ch, stdin);
        }

        lexeme = strdup(buffer);  // Copy the value to lexeme.

        if (strcmp(buffer, "if") == 0) return kwIF;  // Check the string is if keyword
        else if (strcmp(buffer, "else") == 0) return kwELSE;  // Check the string is else keyword
        else if (strcmp(buffer, "while") == 0) return kwWHILE;  // Check the string is while keyword
        else if (strcmp(buffer, "return") == 0) return kwRETURN;  // Check the string is return keyword
        else if (strcmp(buffer, "int") == 0) return kwINT;  // Check the string is int keyword
        else return ID;  // Otherwise, the string will be identifier.
    } 

    // It checks left parenthesis
    if (ch == '(') {
        buffer[0] = '(';
        buffer[1] = '\0';
        lexeme = strdup(buffer);
        return LPAREN;
    }

    // It checks right parenthesis
    if (ch == ')') {
        buffer[0] = ')';
        buffer[1] = '\0';
        lexeme = strdup(buffer);
        return RPAREN;
    }

    // It checks left curly brace
    if (ch == '{') {
        buffer[0] = '{';
        buffer[1] = '\0';
        lexeme = strdup(buffer);
        return LBRACE;
    }

    // It checks right curly brace
    if (ch == '}') {
        buffer[0] = '}';
        buffer[1] = '\0';
        lexeme = strdup(buffer);
        return RBRACE;
    }

    // // It checks comma
    if (ch == ',') {
        buffer[0] = ',';
        buffer[1] = '\0';
        lexeme = strdup(buffer);
        return COMMA;
    }

    // It checks semicolon
    if (ch == ';') {
        buffer[0] = ';';
        buffer[1] = '\0';
        lexeme = strdup(buffer);
        return SEMI;
    }

    // It checks equal sign to be an assignment operator or equal sign operator to check boolean
    if (ch == '=') {
        ch = getchar();
        if (ch == '=') {  // When the next character is also equal sign, it will be equal sign operator.
            buffer[0] = '=';
            buffer[1] = '=';
            buffer[2] = '\0';
            lexeme = strdup(buffer);
            return opEQ;
        } else {  // If not, it will be assignment operator.
            ungetc(ch, stdin);
            buffer[0] = '=';
            buffer[1] = '\0';
            lexeme = strdup(buffer);
            return opASSG;
        }
    }

    // It check add operator
    if (ch == '+') {
        buffer[0] = '+';
        buffer[1] = '\0';
        lexeme = strdup(buffer);
        return opADD;
    }

    // It check sub operator
    if (ch == '-') {
        buffer[0] = '-';
        buffer[1] = '\0';
        lexeme = strdup(buffer);
        return opSUB;
    }

    // // It check multiplication operator
    if (ch == '*') {
        buffer[0] = '*';
        buffer[1] = '\0';
        lexeme = strdup(buffer);
        return opMUL;
    }

    // It check division operator
    if (ch == '/') {
        buffer[0] = '/';
        buffer[1] = '\0';
        lexeme = strdup(buffer);
        return opDIV;
    }

    // It checks not operator and not equal operator
    if (ch == '!') {
        ch = getchar();
        if (ch == '=') {  // When the next character of not operator is equal sign, it would be Not equal operator.
            buffer[0] = '!';
            buffer[1] = '=';
            buffer[2] = '\0';
            lexeme = strdup(buffer);
            return opNE;
        } else {  // If not, it will be just not operator.
            ungetc(ch, stdin);
            buffer[0] = '!';
            buffer[1] = '\0';
            lexeme = strdup(buffer);
            return opNOT;
        }
    }

    // It checks less than operator and less than or equal to operator.
    if (ch == '<') {
        ch = getchar();
        // When the next character of less than operator is equal sign, it will be less than or equal to oeprator.
        if (ch == '=') {
            buffer[0] = '<';
            buffer[1] = '=';
            buffer[2] = '\0';
            lexeme = strdup(buffer);
            return opLE;
        } else {  // If not, it will be less than operator.
            ungetc(ch, stdin);
            buffer[0] = '<';
            buffer[1] = '\0';
            lexeme = strdup(buffer);
            return opLT;
        }
    }

    // It checks larger than operator and larger than or equal to operator.
    if (ch == '>') {
        ch = getchar();
        // When the next character of larger than operator is equal sign, it will be larger than or equal to oeprator.
        if (ch == '=') {
            buffer[0] = '>';
            buffer[1] = '=';
            buffer[2] = '\0';
            lexeme = strdup(buffer);
            return opGE;
        } else {  // If not, it will be larger than operator.
            ungetc(ch, stdin);
            buffer[0] = '>';
            buffer[1] = '\0';
            lexeme = strdup(buffer);
            return opGT;
        }
    }

    // It checks && operator.
    if (ch == '&') {
        ch = getchar();
        if (ch == '&') {  // When &'s next character is also &, it will AND operator.
            buffer[0] = '&';
            buffer[1] = '&';
            buffer[2] = '\0';
            lexeme = strdup(buffer);
            return opAND;
        } else {  // Otherwise, go back to the current standard input.
            ungetc(ch, stdin);
        }
    }

    // It checks || operator.
    if (ch == '|') {
        ch = getchar();
        if (ch == '|') {  // When |'s next character is also |, it will OR operator.
            buffer[0] = '|';
            buffer[1] = '|';
            buffer[2] = '\0';
            lexeme = strdup(buffer);
            return opOR;
        } else {  // Otherwise, go back to the current standard input.
            ungetc(ch, stdin);
        }
    }

    return UNDEF;  // When none of the tokens were detected, it will return undefined.
}