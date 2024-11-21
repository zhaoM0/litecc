#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Debug util function.
#ifdef DEBUG
  #define debug(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
  #define debug(fmt, ...) 
#endif

// Core Data Structure.
typedef enum {
  TK_RESERVED,
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // interger
} NodeKind;

typedef struct Node Node;
struct Node {
  NodeKind kind;
  Node* lhs;
  Node* rhs;
  int val;
};

typedef struct Token Token;
struct Token {
  TokenKind kind;
  int val;
  char* str;
  struct Token* next;
};

// global variables
Token *token;
char *user_input;

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

Node *new_node(NodeKind kind, Node* lhs, Node* rhs) {
  Node* node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_num_node(int val) {
  Node* node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

// Scanner: tokenize source code to independt token.
Token *tokenize(void) {
  char *p = user_input;
  Token head = {};
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      ++p;
      continue;
    }
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }
    if (ispunct(*p)) {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }
  }
  
  cur = new_token(TK_EOF, cur, p);
  return head.next;
}

void dispaly_token_list() {
  Token *cur = token;
  while(cur->kind != TK_EOF) {
    switch (cur->kind) {
      case TK_NUM: 
        printf("(NUM, %d) -> ", cur->val); 
        break;
      case TK_RESERVED:
        printf("(RES, %c) -> ", cur->str[0]);
        break;
    }
    cur = cur->next;
  }
  printf("(EOF, NULL)\n");
}

// Exception: display error info.
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char* loc, char* fmt, ...) {
  va_list args;
  va_start(args, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  exit(1);
}

// Util functions.
int expect_number() {
  if (token->kind != TK_NUM) {
    error_at(token->str, "expected number.");
  }
  int val = token->val;
  token = token->next;
  return val;
}

bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op) {
    return false;
  }
  token = token->next;
  return true;
}

void expect(char op) {
  if (!consume(op))
    error_at(token->str, "expected '%c'", op);
}

// Parser: transform expression to syntax tree.
// Inferfaces
Node* expr();
Node* mul();
Node* primary();

Node* expr() {
  Node* node = mul();

  while(1) {
    if (consume('+')) {
      node = new_node(ND_ADD, node, mul());
    } else if (consume('-')) {
      node = new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

Node* mul() {
  Node* node = primary();

  while(1) {
    if (consume('*')) {
      node = new_node(ND_MUL, node, primary());
    } else if (consume('/')) {
      node = new_node(ND_DIV, node, primary());
    } else {
      return node;
    }
  }
}

Node* primary() {
  if (consume('(')) {
    Node* node = expr();
    expect(')');
    return node;
  }

  return new_num_node(expect_number());
}

// Assembly code generation
void gen(Node* node) {
  if (node == NULL) {
    return;
  }
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");
  
  switch (node->kind) {
    case ND_ADD: 
      printf("  add rax, rdi\n"); 
      break;
    case ND_SUB: 
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n"); 
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");  
      break;  
    default: 
      error("Unkown operator");
  }

  printf("  push rax\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s invalid number of arguments", argv[0]);
  }
  
  user_input = argv[1];
  token = tokenize();
  // dispaly_token_list(token);
  Node* node = expr();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  
  gen(node);

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}