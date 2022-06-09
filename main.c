#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int rIDX = 0;
int assignctr = 0;


#define MAXLEN 256

// Token types
typedef enum
{
    UNKNOWN, END, ENDFILE,
    INT, ID,
    ADDSUB, MULDIV,
    ASSIGN,
    LPAREN, RPAREN,
    INCDEC,
    AND, OR, XOR
} TokenSet;

// Test if a token matches the current token
extern int match(TokenSet token);

// Get the next token
extern void advance(void);

// Get the lexeme of the current token
extern char *getLexeme(void);


#define TBLSIZE 64

// Error types
typedef enum
{
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR, INVALIDVAR
} ErrorType;

// Structure of the symbol table
typedef struct
{
    int val;
    char name[MAXLEN];
} Symbol;

// Structure of a tree node
typedef struct _Node
{
    TokenSet data;
    int val;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
} BTNode;

// The symbol table
extern Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
extern void initTable(void);

// Get the value of a variable
extern int getval(char *str);

// Set the value of a variable
extern int setval(char *str, int val);

// Make a new node according to token type and lexeme
extern BTNode *makeNode(TokenSet tok, const char *lexe);

// Free the syntax tree
extern void freeTree(BTNode *root);

extern void statement(void);
extern BTNode *assign_expr(void);
extern BTNode *or_expr(void);
extern BTNode *or_expr_tail(BTNode *left);
extern BTNode *xor_expr(void);
extern BTNode *xor_expr_tail(BTNode *left);
extern BTNode *and_expr();
extern BTNode *and_expr_tail(BTNode *left);
extern BTNode *addsub_expr(void);
extern BTNode *addsub_expr_tail(BTNode *left);
extern BTNode *muldiv_expr(void);
extern BTNode *muldiv_expr_tail(BTNode *left);
extern BTNode *unary_expr(void);
extern BTNode *factor(void);

void error();


// Evaluate the syntax tree
extern int evaluateTree(BTNode *root);

// Print the syntax tree in prefix
extern void printPrefix(BTNode *root);


static TokenSet getToken(void);
static TokenSet curToken = UNKNOWN;
static char lexeme[MAXLEN];

TokenSet getToken(void)
{
    int i = 0;
    char c = '\0';

    while((c = fgetc(stdin)) == ' ' || c == '\t');

    if(isdigit(c))
    {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while(isdigit(c) && i < MAXLEN)
        {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;
    }
    else if(c == '+')
    {
        c = fgetc(stdin);
        if(c == '+')
        {
            strcpy(lexeme, "++");
            return INCDEC;
        }
        else
        {
            ungetc(c, stdin);
            strcpy(lexeme, "+");
            return ADDSUB;
        }
    }
    else if(c == '-')
    {
        c = fgetc(stdin);
        if(c == '-')
        {
            strcpy(lexeme, "--");
            return INCDEC;
        }
        else
        {
            ungetc(c, stdin);
            strcpy(lexeme, "-");
            return ADDSUB;
        }
    }
    else if(c == '*' || c == '/')
    {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    }
    else if(c == '\n')
    {
        lexeme[0] = '\0';
        return END;
    }
    else if(c == '=')
    {
        strcpy(lexeme, "=");
        return ASSIGN;
    }
    else if(c == '(')
    {
        strcpy(lexeme, "(");
        return LPAREN;
    }
    else if(c == ')')
    {
        strcpy(lexeme, ")");
        return RPAREN;
    }
    else if(c == '&')
    {
        strcpy(lexeme, "&");
        return AND;
    }
    else if(c == '|')
    {
        strcpy(lexeme, "|");
        return OR;
    }
    else if(c == '^')
    {
        strcpy(lexeme, "^");
        return XOR;
    }
    else if(isalpha(c) || c == '_')
    {
        int idx = 0;
        while((isdigit(c) || isalpha(c) || c == '_') && i < MAXLEN)
        {
            lexeme[idx++] = c;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[idx] = '\0';
        return ID;
    }
    else if(c == EOF)
    {
        return ENDFILE;
    }
    else
    {
        return UNKNOWN;
    }
}

void advance(void)
{
    curToken = getToken();
}

int match(TokenSet token)
{
    if(curToken == UNKNOWN)
        advance();
    return token == curToken;
}

char *getLexeme(void)
{
    return lexeme;
}


int sbcount = 0;
Symbol table[TBLSIZE];

void initTable(void)
{
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    sbcount = 3;
    printf("MOV r0 [0]\n");
    printf("MOV r1 [4]\n");
    printf("MOV r2 [8]\n");
}

int getval(char *str)
{
    int i = 0;

    for(i = 0; i < sbcount; i++)
        if(strcmp(str, table[i].name) == 0)
            return table[i].val;

    if(sbcount >= TBLSIZE)
        error();

    error();
}

int setval(char *str, int val)
{
    int i = 0;

    for(i = 0; i < sbcount; i++)
    {
        if(strcmp(str, table[i].name) == 0)
        {
            table[i].val = val;
            return val;
        }
    }

    if(sbcount >= TBLSIZE)
        error();

    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    sbcount++;
    return val;
}

BTNode *makeNode(TokenSet tok, const char *lexe)
{
    BTNode *node = (BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root)
{
    if(root != NULL)
    {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

// statement := END | assign_expr END
void statement(void)
{
    BTNode *retp = NULL;

    if(match(ENDFILE))
    {
        printf("MOV r0 [0]\n");
        printf("MOV r1 [4]\n");
        printf("MOV r2 [8]\n");
        printf("EXIT 0\n");
        exit(0);
    }
    else if(match(END))
    {
        advance();
    }
    else
    {
        retp = assign_expr();
        assignctr = 0;
        rIDX = 0;
        if(match(END))
        {
            evaluateTree(retp);
            freeTree(retp);
            advance();
        }
        else
        {
            error();
        }
    }
}

// assign_expr := ID ASSIGN assign_expr | or_expr
BTNode *assign_expr(void)
{
    BTNode *retp = NULL;
    BTNode *left = or_expr();
    if(match(ASSIGN))
    {
        if(left->data == ID)
        {
            retp = makeNode(ASSIGN, getLexeme());
            advance();
            retp->left = left;
            retp->right = assign_expr();
        }
        else
        {
            error();
        }
    }
    else
    {
        retp = left;
    }
    return retp;
}

// or_expr := xor_expr or_expr_tail
BTNode *or_expr(void)
{
    BTNode *node = xor_expr();
    return or_expr_tail(node);
}

// or_expr_tail := OR xor_expr or_expr_tail | NiL
BTNode *or_expr_tail(BTNode *left)
{
    BTNode *node = NULL;

    if(match(OR))
    {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        return or_expr_tail(node);
    }
    else
    {
        return left;
    }
}

// xor_expr := and_expr xor_expr_tail
BTNode *xor_expr(void)
{
    BTNode *node = and_expr();
    return xor_expr_tail(node);
}

// xor_expr_tail := XOR and_expr xor_expr_tail | NiL
BTNode *xor_expr_tail(BTNode *left)
{
    BTNode *node = NULL;

    if(match(XOR))
    {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        return xor_expr_tail(node);
    }
    else
    {
        return left;
    }
}

// and_expr := addsub_expr and_expr_tail
BTNode *and_expr()
{
    BTNode *node = addsub_expr();
    return and_expr_tail(node);
}

// and_expr_tail := AND addsub_expr and_expr_tail | NiL
BTNode *and_expr_tail(BTNode *left)
{
    BTNode *node = NULL;

    if(match(AND))
    {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        return and_expr_tail(node);
    }
    else
    {
        return left;
    }
}

// addsub_expr := muldiv_expr addsub_expr_tail
BTNode *addsub_expr(void)
{
    BTNode *node = muldiv_expr();
    return addsub_expr_tail(node);
}

// addsub_expr_tail := ADDSUB muldiv_expr addsub_expr_tail | NiL
BTNode *addsub_expr_tail(BTNode *left)
{
    BTNode *node = NULL;

    if(match(ADDSUB))
    {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        return addsub_expr_tail(node);
    }
    else
    {
        return left;
    }
}

// muldiv_expr := unary_expr muldiv_expr_tail
BTNode *muldiv_expr(void)
{
    BTNode *node = unary_expr();
    return muldiv_expr_tail(node);
}

// muldiv_expr_tail := MULDIV unary_expr muldiv_expr_tail | NiL
BTNode *muldiv_expr_tail(BTNode *left)
{
    BTNode *node = NULL;

    if(match(MULDIV))
    {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary_expr();
        return muldiv_expr_tail(node);
    }
    else
    {
        return left;
    }
}

// unary_expr := ADDSUB unary_expr | factor
BTNode *unary_expr()
{
    BTNode *node = NULL;

    if(match(ADDSUB))
    {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = makeNode(INT, "0");
        node->right = unary_expr();
    }
    else
    {
        node = factor();
    }
    return node;
}

// factor := INT | ID | INCDEC ID | LPAREN assign_expr RPAREN
BTNode *factor(void)
{
    BTNode *retp = NULL, *left = NULL;

    if(match(INT))
    {
        retp = makeNode(INT, getLexeme());
        advance();
    }
    else if(match(ID))
    {
        retp = makeNode(ID, getLexeme());
        advance();
    }
    else if(match(INCDEC))
    {
        retp = makeNode(INCDEC, getLexeme());
        advance();
        if (match(ID))
        {
            retp->left = makeNode(ID, getLexeme());
            retp->right = makeNode(INT, "1");
            advance();
        }
        else
        {
            error();
        }
    }
    else if(match(LPAREN))
    {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
            error();
    }
    else
    {
        error();
    }
    return retp;
}

void error()
{
    printf("EXIT 1\n");
    exit(0);
}


int varcount;
int tempidx = 0;
int temp[1000];

int evaluateTree(BTNode *root)
{
    int retval = 0, lv = 0, rv = 0;

    int idx;
    if(root != NULL)
    {
        switch(root->data)
        {
        case ID:
            retval = getval(root->lexeme);
            for(idx = 0 ; idx < sbcount; idx++)
            {
                if(table[idx].name == NULL)
                    break;
                if(strcmp(table[idx].name, root->lexeme) == 0)
                    break;
            }
            if(rIDX > 7)
            {
                printf("MOV [%d] r%d\n", sbcount * 4, tempidx);
                temp[tempidx] = sbcount + 1;
                tempidx++;
                sbcount++;
            }
            printf("MOV r%d [%d]\n", rIDX % 8, idx * 4);
            rIDX++;
            break;
        case INT:
            retval = atoi(root->lexeme);
            if(rIDX > 7)
            {
                printf("MOV [%d] r%d\n", sbcount * 4, tempidx);
                temp[tempidx] = sbcount + 1;
                tempidx++;
                sbcount++;
            }
            printf("MOV r%d %d\n", rIDX % 8, retval);
            rIDX++;
            break;
        case ASSIGN:
            assignctr++;
            rv = evaluateTree(root->right);
            retval = setval(root->left->lexeme, rv);
            for(idx = 0 ; idx < sbcount; idx++)
            {
                if(strcmp(table[idx].name, root->left->lexeme) == 0)
                    break;
            }
            printf("MOV [%d] r%d\n", idx * 4, rIDX - 1);
            if(rIDX > 7)
            {
                printf("MOV r%d [%d]\n", tempidx - 1, (tempidx - 1) * 4);
                tempidx--;
                sbcount--;
            }
            else
            {
                if(rIDX > 0 && assignctr < 1)
                    rIDX--;
            }
            break;
        case ADDSUB:
        case INCDEC:
        case AND:
        case OR:
        case XOR:
            lv = evaluateTree(root->left);
            rv = evaluateTree(root->right);
            if(strcmp(root->lexeme, "+") == 0)
            {
                retval = lv + rv;
                printf("ADD r%d r%d\n", (rIDX - 2) % 8, (rIDX - 1) % 8);
                rIDX--;

            }
            else if(strcmp(root->lexeme, "-") == 0)
            {
                retval = lv - rv;
                printf("SUB r%d r%d\n", (rIDX - 2) % 8, (rIDX - 1) % 8);

                    rIDX--;
            }
            else if(strcmp(root->lexeme, "++") == 0)
            {
                assignctr++;
                retval = lv + rv;
                setval(root->left->lexeme, retval);
                printf("ADD r%d r%d\n", (rIDX - 2) % 8, (rIDX - 1) % 8);
                rIDX--;
                for(idx = 0 ; idx < sbcount; idx++)
                {
                    if(strcmp(table[idx].name, root->left->lexeme) == 0)
                        break;
                }
                printf("MOV [%d] r%d\n", idx * 4, rIDX - 1);
                if(rIDX > 0 && assignctr < 1)
                    rIDX--;
            }
            else if(strcmp(root->lexeme, "--") == 0)
            {
                assignctr++;
                retval = lv - rv;
                setval(root->left->lexeme, retval);
                printf("SUB r%d r%d\n", (rIDX - 2) % 8, (rIDX - 1) % 8);
                rIDX--;
                for(idx = 0 ; idx < sbcount; idx++)
                {
                    if(strcmp(table[idx].name, root->left->lexeme) == 0)
                        break;
                }
                printf("MOV [%d] r%d\n", idx * 4, rIDX - 1);
                if(rIDX > 0 && assignctr < 1)
                    rIDX--;
            }
            else if(strcmp(root->lexeme, "&") == 0)
            {
                retval = lv & rv;
                printf("AND r%d r%d\n", (rIDX - 2) % 8, (rIDX - 1) % 8);
                rIDX--;
            }
            else if(strcmp(root->lexeme, "|") == 0)
            {
                retval = lv | rv;
                printf("OR r%d r%d\n", (rIDX - 2) % 8, (rIDX - 1) % 8);

                rIDX--;
            }
            else if(strcmp(root->lexeme, "^") == 0)
            {
                retval = lv ^ rv;
                printf("XOR r%d r%d\n", (rIDX - 2) % 8, (rIDX - 1) % 8);

                rIDX--;
            }
            if(tempidx > 0)
            {
                printf("MOV r%d [%d]\n", tempidx - 1, temp[tempidx - 1] * 4 - 4);
                tempidx--;
                sbcount--;
            }
            break;
        case MULDIV:
            lv = evaluateTree(root->left);
            if(strcmp(root->lexeme, "/") == 0)
            {
                rv = evaluateTree(root->right);
                varcount = 0;
                findVar(root->right);
                if(rv == 0)
                {
                    if(varcount > 0)
                    {
                        rv = 1;
                    }
                    else
                    {
                        error();
                    }
                }
                retval = lv / rv;
                printf("DIV r%d r%d\n", (rIDX - 2) % 8, (rIDX - 1) % 8);
                if(tempidx > 0)
                {
                    printf("MOV r%d [%d]\n", tempidx - 1, temp[tempidx - 2] * 4);
                    tempidx--;
                    sbcount--;
                }
                else
                    rIDX--;
            }
            else if(strcmp(root->lexeme, "*") == 0)
            {
                rv = evaluateTree(root->right);
                retval = lv * rv;
                printf("MUL r%d r%d\n", (rIDX - 2) % 8, (rIDX - 1) % 8);
                if(tempidx > 0)
                {
                    printf("MOV r%d [%d]\n", tempidx - 1, temp[tempidx - 2] * 4);
                    tempidx--;
                    sbcount--;
                }
                else
                    rIDX--;
            }
            break;
        default:
            retval = 0;
        }
    }
    return retval;
}

void findVar(BTNode *root)
{
    if(root->data == ID)
        varcount++;

    if(root->left)
        findVar(root->left);
    if(root->right)
        findVar(root->right);
}

void printPrefix(BTNode *root)
{
    if(root != NULL)
    {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}


int main()
{
    initTable();
    while(1)
    {
        statement();
    }
    return 0;
}
