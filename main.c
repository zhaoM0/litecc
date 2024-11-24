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
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
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
  int len;
  Token* next;
};

// global variables
Token *token;
char *user_input;

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
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

void error_at(char* loc, char* fmt, ...);

bool at_eof() {
  return token->kind == TK_EOF;
}

static bool startwith(char *p, const char *t) {
  return strncmp(p, t, strlen(t)) == 0;
}

// Scanner: tokenize source code to independt token.
Token *tokenize(void) {
  char *p = user_input;
  Token head = {};
  Token *cur = &head;

  while (*p) {
    // Skip whitespace characters.
    if (isspace(*p)) {
      ++p;
      continue;
    }
    // Multi-letter punctuators
    if (startwith(p, "==") || startwith(p, "!=") ||
        startwith(p, "<=") || startwith(p, ">=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      p += 2;
      continue;
    }
    // Single-letter punctuators
    if (ispunct(*p)) {
      cur = new_token(TK_RESERVED, cur, p++, 1);
      continue;
    }
    // Integer literal
    if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      char *q = p;
      cur->val = strtol(p, &p, 10);
      cur->len = p - q;
      continue;
    }
    // Invalid token.
    error_at(p, "invalid token");
  }
  
  cur = new_token(TK_EOF, cur, p, 0);
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
        char* sval = (char *)calloc(10, sizeof(char));
        strncpy(sval, cur->str, cur->len);
        printf("(RES, %s) -> ", sval);
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

bool consume(char *op) {
  if (token->kind != TK_RESERVED ||
      token->len != strlen(op) || 
      strncmp(token->str, op, token->len)) {
    return false;
  }
  token = token->next;
  return true;
}

void expect(char *op) {
  if (!consume(op))
    error_at(token->str, "expected \"%s\"", op);
}

// Parser: transform expression to syntax tree.
// Inferfaces
static Node* expr();
static Node* equality();
static Node* relational();
static Node* add();
static Node* mul();
static Node* unary();
static Node* primary();


static Node* expr() {
  return equality();
}

static Node* equality() {
  Node* node = relational();

  while(1) {
    if (consume("==")) {
      node = new_node(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_node(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

static Node* relational() {
  Node* node = add();

  while(1) {
    if (consume("<")) {
      node = new_node(ND_LT, node, add());
    } else if (consume("<=")) {
      node = new_node(ND_LE, node, add());
    } else if (consume(">")) {
      node = new_node(ND_LT, add(), node);
    } else if (consume(">=")) {
      node = new_node(ND_LE, add(), node);
    } else {
      return node;
    }
  }
}


static Node* add() {
  Node* node = mul();

  while(1) {
    if (consume("+")) {
      node = new_node(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

static Node* mul() {
  Node* node = unary();

  while(1) {
    if (consume("*")) {
      node = new_node(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_node(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

static Node* unary() {
  if (consume("+")) {
    return unary();
  } else if (consume("-")) {
    return new_node(ND_SUB, new_num_node(0), unary());
  } 
  return primary();
}

static Node* primary() {
  if (consume("(")) {
    Node* node = expr();
    expect(")");
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
    case ND_EQ:
      printf("  cmp rax, rdi\n");
      printf("  sete al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_NE:
      printf("  cmp rax, rdi\n");
      printf("  setne al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LT:
      printf("  cmp rax, rdi\n");
      printf("  setl al\n");
      printf("  movzb rax, al\n");
      break;
    case ND_LE:
      printf("  cmp rax, rdi\n");
      printf("  setle al\n");
      printf("  movzb rax, al\n");
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