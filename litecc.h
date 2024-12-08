#define _GNU_SOURCE
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Debug utils.
#ifdef DEBUG
  #define debug(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
  #define debug(fmt, ...) 
#endif

//
// tokenize.c
//

typedef enum {
  TK_RESERVED,   // Keywords or punctuators
  TK_IDENT,      // Identifiers
  TK_NUM,        // Integer literals
  TK_EOF,        // End-of-file markers
} TokenKind;

typedef struct Token Token;
struct Token {
  TokenKind kind;  // Token kind
  long val;        // If kind is TK_NUM, its value
  char* str;       // Token string
  int len;         // Token length
  Token* next;     // Next token
};

void   dispaly_tokens(void);
void   error(char *fmt, ...);
void   error_at(char* loc, char* fmt, ...);
bool   consume(char *op);
Token *consume_ident(void);
void   expect(char *op);
long   expect_number(void);
char  *expect_ident(void);
bool   at_eof(void);
Token *tokenize(void);

extern char*  user_input;
extern Token* token;

//
// parse.c
// 

// Local variable
typedef struct Var Var;
struct Var {
  char* name;    // variable name
  int   len;     // name length
  int   offset;  // offset from rbp
};

typedef struct VarList VarList;
struct VarList {
  VarList *next;
  Var *var;
};

typedef enum {
  ND_ADD,        // +
  ND_SUB,        // -
  ND_MUL,        // *
  ND_DIV,        // /
  ND_EQ,         // ==
  ND_NE,         // !=
  ND_LT,         // <
  ND_LE,         // <=
  ND_ASSIGN,     // =
  ND_RETURN,     // "return"
  ND_IF,         // "if"
  ND_WHILE,      // "while"
  ND_FOR,        // "for"
  ND_BLOCK,      // Block -> "{...}"
  ND_FUNCALL,    // Function call
  ND_EXPR_STMT,  // Expression statement
  ND_VAR,        // Variable
  ND_NUM,        // Interger
} NodeKind;

// AST node type.
typedef struct Node Node;
struct Node {
  NodeKind kind;  // Node kind
  Node* next;     // Next node

  Node* lhs;      // Left-hand side
  Node* rhs;      // Right-hand side
  
  // "if", "while" or "for" statement
  Node* cond;
  Node* then;
  Node* els;
  Node* init;
  Node* inc;

  // Block
  Node* block;

  // Function Call
  char* funcname;
  Node* args;

  Var*  var;      // Used if kind == ND_VAR
  long  val;      // Used if kind == ND_NUM
};

typedef struct Function Function;
struct Function {
  Function* next;
  char*     name;
  VarList*  params;
  Node*     node;
  VarList*  locals;
  int       stack_size;
};

Function *program(void);

//
// codegen.c
//

void codegen(Function* prog);