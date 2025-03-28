#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_IDENTIFIER_SIZE 10
#define MAX_INTEGER_SIZE 8
#define MAX_STRING_SIZE 256

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_INTCONST,
    TOKEN_STRING,
    TOKEN_LEFTCURLY,
    TOKEN_RIGHTCURLY,
    TOKEN_ENDOFLINE,
    TOKEN_COMMA,
    TOKEN_OPERATOR,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char value[MAX_STRING_SIZE + 1];
} Token;

typedef struct ASTNode {
    TokenType type;
    char value[MAX_STRING_SIZE + 1];
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;
    struct ASTNode *body;
} ASTNode;

typedef struct {
    char name[MAX_IDENTIFIER_SIZE + 1];
    int intValue;
    char stringValue[MAX_STRING_SIZE + 1];
    int isInt;
} Variable;
ASTNode *parseStatement();
ASTNode *parseProgram();

Token *tokens;
int tokenCount = 0;
int currentTokenIndex = 0;
Variable variables[100];
int variableCount = 0;

Token getNextToken() {
    if (currentTokenIndex >= tokenCount) {
        Token t;
        t.type = TOKEN_ERROR;
        strcpy(t.value, "EOF");
        return t;
    }
    return tokens[currentTokenIndex++];
}

void ungetToken() {
    if (currentTokenIndex > 0) {
        currentTokenIndex--;
    }
}

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

void lexer(const char *filename) {
    FILE *sourceFile = fopen(filename, "r");
    if (sourceFile == NULL) {
        perror("Error opening file");
        exit(1);
    }

    tokens = malloc(sizeof(Token) * 1000); // Allocate memory for tokens
    int c;
    char buffer[MAX_STRING_SIZE + 1];
    int bufferIndex = 0;
    while ((c = fgetc(sourceFile)) != EOF) {
        if (isspace(c)) {
            continue;
        } else if (isalpha(c)) {
            buffer[bufferIndex++] = c;
            while ((c = fgetc(sourceFile)) != EOF && (isalnum(c) || c == '_')) {
                buffer[bufferIndex++] = c;
            }
            buffer[bufferIndex] = '\0';
            ungetc(c, sourceFile);
            Token token;
            if (bufferIndex <= MAX_IDENTIFIER_SIZE) {
                if (isKeyword(buffer)) {
                    token.type = TOKEN_KEYWORD;
                } else {
                    token.type = TOKEN_IDENTIFIER;
                }
                strcpy(token.value, buffer);
                printf("Token: %s, Type: %d\n", token.value, token.type);
                tokens[tokenCount++] = token;
            } else {
                printf("Error: Identifier too long\n");
                break;
            }
            bufferIndex = 0;
        } else if (isdigit(c)) {
            bufferIndex = 0;
            buffer[bufferIndex++] = c;
            while ((c = fgetc(sourceFile)) != EOF && isdigit(c)) {
                buffer[bufferIndex++] = c;
            }
            buffer[bufferIndex] = '\0';
            ungetc(c, sourceFile);
            if (bufferIndex <= MAX_INTEGER_SIZE) {
                Token token;
                token.type = TOKEN_INTCONST;
                strcpy(token.value, buffer);
                tokens[tokenCount++] = token;
                printf("Token: %s, Type: Integer\n", token.value);
            } else {
                printf("Error: Integer constant too long\n");
                break;
            }
            bufferIndex = 0;
        } else if (c == '"') {
            bufferIndex = 0;
            while ((c = fgetc(sourceFile)) != EOF && c != '"' && bufferIndex < MAX_STRING_SIZE) {
                buffer[bufferIndex++] = c;
            }
            if (c == '"') {
                buffer[bufferIndex] = '\0';
                Token token;
                token.type = TOKEN_STRING;
                strcpy(token.value, buffer);
                tokens[tokenCount++] = token;
                printf("Token: \"%s\", Type: String\n", token.value);
            } else {
                printf("Error: Unterminated string constant\n");
                break;
            }
            bufferIndex = 0;
        } else if (c == '{' || c == '}' || c == '.' || c == ',' || c == '+' || c == '-' || c == '*' || c == '/') {
            Token token;
            token.type = (c == '{') ? TOKEN_LEFTCURLY :
                         (c == '}') ? TOKEN_RIGHTCURLY :
                         (c == '.') ? TOKEN_ENDOFLINE :
                         (c == ',') ? TOKEN_COMMA : TOKEN_OPERATOR;
            token.value[0] = c;
            token.value[1] = '\0';
            tokens[tokenCount++] = token;
            printf("Token: %s, Type: %s\n", token.value, token.type == TOKEN_OPERATOR ? "Operator" : "Delimiter");
        } else if (c == '/') {
            if ((c = fgetc(sourceFile)) == '*') {
                while ((c = fgetc(sourceFile)) != EOF && !(c == '*' && (c = fgetc(sourceFile)) == '/')) {}
                if (c == EOF) {
                    printf("Error: Unterminated comment\n");
                    break;
                }
            } else {
                ungetc(c, sourceFile);
                Token token;
                token.type = TOKEN_OPERATOR;
                strcpy(token.value, "/");
                tokens[tokenCount++] = token;
                printf("Token: /, Type: Operator\n");
            }
        } else {
            printf("Error: Invalid character '%c'\n", c);
            break;
        }
    }

    fclose(sourceFile);
}

ASTNode *createASTNode(TokenType type, char *value) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = type;
    strcpy(node->value, value);
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    return node;
}

ASTNode *parseExpression() {
    printf("Parsing expression\n");

    ASTNode *left = createASTNode(tokens[currentTokenIndex].type, tokens[currentTokenIndex].value);
    currentTokenIndex++;
    
    while (currentTokenIndex < tokenCount && tokens[currentTokenIndex].type == TOKEN_OPERATOR) {
        ASTNode *operatorNode = createASTNode(TOKEN_OPERATOR, tokens[currentTokenIndex].value);
        currentTokenIndex++;
        
        ASTNode *right = createASTNode(tokens[currentTokenIndex].type, tokens[currentTokenIndex].value);
        currentTokenIndex++;
        
        operatorNode->left = left;
        operatorNode->right = right;
        left = operatorNode;
    }
    
    return left;
}

ASTNode *parseVariableDeclaration() {
    printf("Parsing variable declaration\n");

    Token token = getNextToken();
    if (token.type != TOKEN_KEYWORD) {
        printf("Error: Expected keyword\n");
        return NULL;
    }

    char varType[MAX_STRING_SIZE + 1];
    strcpy(varType, token.value);

    token = getNextToken();
    if (token.type != TOKEN_IDENTIFIER) {
        printf("Error: Expected identifier\n");
        return NULL;
    }

    char varName[MAX_STRING_SIZE + 1];
    strcpy(varName, token.value);

    ASTNode *node = createASTNode(TOKEN_KEYWORD, varType);
    node->left = createASTNode(TOKEN_IDENTIFIER, varName);

    token = getNextToken();
    if (token.type == TOKEN_KEYWORD && strcmp(token.value, "is") == 0) {
        ASTNode *rhs = parseExpression();
        node->right = rhs;

        token = getNextToken();
        if (token.type != TOKEN_ENDOFLINE) {
            printf("Error: Expected end of line\n");
            return NULL;
        }
    } else {
        ungetToken();
        token = getNextToken();
        if (token.type != TOKEN_ENDOFLINE) {
            printf("Error: Expected end of line\n");
            return NULL;
        }
    }

    // Add variable to the variables list
    Variable var;
    strcpy(var.name, varName);
    var.isInt = strcmp(varType, "int") == 0;
    variables[variableCount++] = var;

    return node;
}



ASTNode *parseAssignment() {
    printf("Parsing assignment\n");

    Token token = getNextToken();
    if (token.type != TOKEN_IDENTIFIER) {
        printf("Error: Expected identifier\n");
        return NULL;
    }

    char varName[MAX_STRING_SIZE + 1];
    strcpy(varName, token.value);

    token = getNextToken();
    if (token.type != TOKEN_KEYWORD || strcmp(token.value, "is") != 0) {
        printf("Error: Expected 'is'\n");
        return NULL;
    }

    ASTNode *node = createASTNode(TOKEN_IDENTIFIER, varName);
    ASTNode *rhs = parseExpression();
    node->right = rhs;

    token = getNextToken();
    if (token.type != TOKEN_ENDOFLINE) {
        printf("Error: Expected end of line\n");
        return NULL;
    }

    return node;
}

ASTNode *parseWriteStatement() {
    printf("Parsing write statement\n");

    Token token = getNextToken();
    if (token.type != TOKEN_KEYWORD || strcmp(token.value, "write") != 0) {
        printf("Error: Expected 'write'\n");
        return NULL;
    }

    token = getNextToken();
    if (token.type != TOKEN_IDENTIFIER) {
        printf("Error: Expected identifier\n");
        return NULL;
    }

    ASTNode *node = createASTNode(TOKEN_KEYWORD, "write");
    node->left = createASTNode(TOKEN_IDENTIFIER, token.value);

    token = getNextToken();
    if (token.type != TOKEN_ENDOFLINE) {
        printf("Error: Expected end of line\n");
        return NULL;
    }

    return node;
}

ASTNode *parseLoopStatement() {
    printf("Parsing loop statement\n");

    Token token = getNextToken();
    if (token.type != TOKEN_KEYWORD || strcmp(token.value, "loop") != 0) {
        printf("Error: Expected 'loop'\n");
        return NULL;
    }

    token = getNextToken();
    if (token.type != TOKEN_INTCONST) {
        printf("Error: Expected integer constant\n");
        return NULL;
    }

    int loopCount = atoi(token.value);
    char loopCountStr[MAX_INTEGER_SIZE + 1];
    snprintf(loopCountStr, sizeof(loopCountStr), "%d", loopCount);

    token = getNextToken();
    if (token.type != TOKEN_KEYWORD || strcmp(token.value, "times") != 0) {
        printf("Error: Expected 'times'\n");
        return NULL;
    }

    ASTNode *node = createASTNode(TOKEN_KEYWORD, "loop");
    node->left = createASTNode(TOKEN_INTCONST, loopCountStr);

    token = getNextToken();
    if (token.type == TOKEN_LEFTCURLY) {
        node->body = parseProgram(); // Reuse parseProgram to parse the block
        token = getNextToken();
        if (token.type != TOKEN_RIGHTCURLY) {
            printf("Error: Expected '}'\n");
            return NULL;
        }
    } else {
        ungetToken();
        node->body = parseStatement();
    }

    return node;
}


ASTNode *parseStatement() {
    printf("Parsing statement\n");

    Token token = getNextToken();
    ungetToken();

    if (token.type == TOKEN_KEYWORD) {
        if (strcmp(token.value, "int") == 0 || strcmp(token.value, "text") == 0) {
            return parseVariableDeclaration();
        } else if (strcmp(token.value, "write") == 0) {
            return parseWriteStatement();
        } else if (strcmp(token.value, "loop") == 0) {
            return parseLoopStatement();
        } else {
            printf("Error: Invalid statement\n");
            return NULL;
        }
    } else if (token.type == TOKEN_IDENTIFIER) {
        return parseAssignment();
    } else {
        printf("Error: Invalid statement\n");
        return NULL;
    }
}

ASTNode *parseProgram() {
    printf("Parsing program\n");

    ASTNode *head = NULL;
    ASTNode *current = NULL;

    while (currentTokenIndex < tokenCount) {
        ASTNode *statement = parseStatement();
        if (statement == NULL) {
            break;
        }
        if (head == NULL) {
            head = statement;
            current = statement;
        } else {
            current->next = statement;
            current = statement;
        }
    }

    return head;
}



Variable *findVariable(char *name) {
    for (int i = 0; i < variableCount; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i];
        }
    }
    return NULL;
}


int evaluateExpression(ASTNode *node) {
    if (node->type == TOKEN_INTCONST) {
        return atoi(node->value);
    } else if (node->type == TOKEN_IDENTIFIER) {
        Variable *var = findVariable(node->value);
        if (var != NULL && var->isInt) {
            return var->intValue;
        } else {
            printf("Error: Undefined variable or not an integer\n");
            exit(1);
        }
    } else if (node->type == TOKEN_OPERATOR) {
        int leftValue = evaluateExpression(node->left);
        int rightValue = evaluateExpression(node->right);
        if (strcmp(node->value, "+") == 0) {
            return leftValue + rightValue;
        } else if (strcmp(node->value, "-") == 0) {
            return leftValue - rightValue;
        } else if (strcmp(node->value, "*") == 0) {
            return leftValue * rightValue;
        } else if (strcmp(node->value, "/") == 0) {
            if (rightValue == 0) {
                printf("Error: Division by zero\n");
                exit(1);
            }
            return leftValue / rightValue;
        } else {
            printf("Error: Invalid operator\n");
            exit(1);
        }
    } else {
        printf("Error: Invalid expression\n");
        exit(1);
    }
    return 0;
}

char *evaluateStringExpression(ASTNode *node) {
    if (node->type == TOKEN_STRING) {
        return strdup(node->value);
    } else if (node->type == TOKEN_IDENTIFIER) {
        Variable *var = findVariable(node->value);
        if (var != NULL && !var->isInt) {
            return strdup(var->stringValue);
        } else {
            printf("Error: Undefined variable or not a string\n");
            exit(1);
        }
    } else if (node->type == TOKEN_OPERATOR) {
        char *leftValue = evaluateStringExpression(node->left);
        char *rightValue = evaluateStringExpression(node->right);

        if (strcmp(node->value, "+") == 0) {
            size_t resultSize = strlen(leftValue) + strlen(rightValue) +1;
            char *result = (char *)malloc(resultSize);
            if (result == NULL) {
                printf("Error: Memory allocation failed\n");
                exit(1);
            }
            snprintf(result, resultSize, "%s%s", leftValue, rightValue);

            free(leftValue);
            free(rightValue);
            return result;
        } else if (strcmp(node->value, "-") == 0) {
            char *result = strdup(leftValue);
            if (result == NULL) {
                printf("Error: Memory allocation failed\n");
                exit(1);
            }

            char *subPos = strstr(result, rightValue);
            if (subPos != NULL) {
                memmove(subPos, subPos + strlen(rightValue), strlen(subPos + strlen(rightValue)) + 1);
            }

            free(leftValue);
            free(rightValue);
            return result;
        } else {
            printf("Error: Invalid operator for strings\n");
            exit(1);
        }
    } else {
        printf("Error: Invalid string expression\n");
        exit(1);
    }
    return NULL;
}
void execute(ASTNode *node);

void executeLoopStatement(ASTNode *node) {
    int loopCount = atoi(node->left->value);

    for (int i = 0; i < loopCount; i++) {
        ASTNode *body = node->body;
        while (body != NULL) {
            execute(body);
            body = body->next;
        }
    }
}

void execute(ASTNode *node) {
    while (node != NULL) {
        if (node->type == TOKEN_KEYWORD) {
            if (strcmp(node->value, "int") == 0 || strcmp(node->value, "text") == 0) {
                Variable var;
                strcpy(var.name, node->left->value);
                var.isInt = strcmp(node->value, "int") == 0;
                if (node->right != NULL) {
                    if (var.isInt) {
                        var.intValue = evaluateExpression(node->right);
                    } else {
                        strcpy(var.stringValue, evaluateStringExpression(node->right));
                    }
                }
                variables[variableCount++] = var;
            } else if (strcmp(node->value, "write") == 0) {
                Variable *var = findVariable(node->left->value);
                if (var != NULL) {
                    if (var->isInt) {
                        printf("%d\n", var->intValue);
                    } else {
                        printf("%s\n", var->stringValue);
                    }
                } else {
                    printf("Error: Undefined variable\n");
                    exit(1);
                }
            } else if (strcmp(node->value, "loop") == 0) {
                executeLoopStatement(node);
            }
        } else if (node->type == TOKEN_IDENTIFIER) {
            Variable *var = findVariable(node->value);
            if (var != NULL) {
                if (var->isInt) {
                    var->intValue = evaluateExpression(node->right);
                } else {
                    strcpy(var->stringValue, evaluateStringExpression(node->right));
                }
            } else {
                printf("Error: Undefined variable\n");
                exit(1);
            }
        }
        node = node->next;
    }
}

int main() {
    lexer("/Users/burakkocabas/Desktop/PL_PROJECT2/PL_PROJECT2/PL_PROJECT2/code.sta.txt");
    ASTNode *program = parseProgram();
    execute(program);
    return 0;
}

