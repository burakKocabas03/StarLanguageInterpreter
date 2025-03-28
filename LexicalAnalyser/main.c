#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_IDENTIFIER_SIZE 10
#define MAX_INTEGER_SIZE 8
#define MAX_STRING_SIZE 256

int isKeyword(char *word) {
    char *keywords[] = {"int", "text", "is", "loop", "times", "read", "write", "newLine"};
    int i, totalKeywords = 8;
    for (i = 0; i < totalKeywords; ++i) {
        if (strcmp(word, keywords[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int main() {
    FILE *sourceFile = fopen("code.sta.txt", "r");
    FILE *outputFile = fopen("code.lex.txt", "w");

    if (sourceFile == NULL || outputFile == NULL) {
        perror("Error opening files");
        return 1;
    }

    int c;
    char buffer[MAX_STRING_SIZE + 1]; // +1 for null terminator
    int bufferIndex = 0;
    int stringExceeded = 0;
    int stringTerminated = 0;

    while ((c = fgetc(sourceFile)) != EOF) {
        if (isspace(c)) {
            continue; // ignore whitespace
        } else if (isalpha(c)) {
            // Process identifier or keyword
            buffer[bufferIndex++] = c;
            while ((c = fgetc(sourceFile)) != EOF && (isalnum(c) || c == '_')) {
                buffer[bufferIndex++] = c;
            }
            buffer[bufferIndex] = '\0';
            if (bufferIndex <= MAX_IDENTIFIER_SIZE) {
                if (isKeyword(buffer)) {
                    fprintf(outputFile, "Keyword(%s)\n", buffer);
                } else {
                    fprintf(outputFile, "Identifier(%s)\n", buffer);
                }
            } else {
                fprintf(outputFile, "Error: Identifier too long\n");
                printf("Error: Identifier too long\n");
                break; // Dosya işlemi sonlandırılıyor
            }
            bufferIndex = 0;
            ungetc(c, sourceFile); // push back last read character
        } else if (isdigit(c) || (c == '-' && isdigit(fgetc(sourceFile)))) {
            // Process integer constant
            bufferIndex = 0; // Reset buffer index for integer constant
            buffer[bufferIndex++] = c;
            while ((c = fgetc(sourceFile)) != EOF && isdigit(c)) {
                buffer[bufferIndex++] = c;
            }
            buffer[bufferIndex] = '\0';
            if (bufferIndex <= MAX_INTEGER_SIZE) {
                if (buffer[0] == '-') {
                    fprintf(outputFile, "Error: Negative integers are not allowed\n");
                    printf("Error: Negative integers are not allowed\n");
                    break; // Dosya işlemi sonlandırılıyor
                } else {
                    fprintf(outputFile, "IntConst(%s)\n", buffer);
                }
            } else {
                fprintf(outputFile, "Error: Integer constant too long\n");
                printf("Error: Integer constant too long\n");
                break; // Dosya işlemi sonlandırılıyor
            }
            bufferIndex = 0;
            ungetc(c, sourceFile); // push back last read character
        } else if (c == '"') {
            // Process string constant
            bufferIndex = 0;
            while ((c = fgetc(sourceFile)) != EOF && c != '"' && bufferIndex < MAX_STRING_SIZE) {
                buffer[bufferIndex++] = c;
            }
            if (bufferIndex == MAX_STRING_SIZE) {
                fprintf(outputFile, "Error: String constant exceeds maximum size\n");
                printf("Error: String constant exceeds maximum size\n");
                stringExceeded = 1;
            } else if (c == EOF) {
                fprintf(outputFile, "Error: Unterminated string constant\n");
                printf("Error: Unterminated string constant\n");
                stringTerminated = 1;
            } else {
                buffer[bufferIndex] = '\0';
                fprintf(outputFile, "String(%s)\n", buffer);
            }
            bufferIndex = 0;
            if (c == '"') {
                stringExceeded = 0;
                stringTerminated = 0;
            }
        } else if (c == '{') {
            fprintf(outputFile, "LeftCurlyBracket\n");
        } else if (c == '}') {
            fprintf(outputFile, "RightCurlyBracket\n");
        } else if (c == '.') {
            fprintf(outputFile, "EndOfLine\n");
        } else if (c == ',') {
            fprintf(outputFile, "Comma\n");
        } else if (c == '/') {
            if ((c = fgetc(sourceFile)) == '*') {
                // Jump all comment
                while ((c = fgetc(sourceFile)) != EOF && c != '*') {}
                if (c == EOF) {
                    fprintf(outputFile, "Error: Unterminated comment\n");
                    printf("Error: Unterminated comment\n");
                    break;
                }
                if ((c = fgetc(sourceFile)) != '/') {
                    fprintf(outputFile, "Error: Unterminated comment\n");
                    printf("Error: Unterminated comment\n");
                    break; // cant find end of comment
                }
                continue; // comment end code will continue  to operate
            } else {
                // We decided its not comment and we are throwing / to c
                ungetc(c, sourceFile);
                c = '/';
                fprintf(outputFile, "Operator(%c)\n", c);
            }
        } else if (c == '+' || c == '-' || c == '*' ) {
            fprintf(outputFile, "Operator(%c)\n", c);
        }  else {
            // ignore comments
            fprintf(outputFile, "Error: Invalid character\n");
            printf( "Error: Invalid character\n");
            break; // Dosya işlemi sonlandırılıyor
        }

        if (stringExceeded || stringTerminated) {
            // Skip until end of string constant
            while ((c = fgetc(sourceFile)) != EOF && c != '"') {}
            if (c == EOF) {
                break;
            }
            stringExceeded = 0;
            stringTerminated = 0;
        }
    }

    fclose(sourceFile);
    fclose(outputFile);
    return 0;
}

