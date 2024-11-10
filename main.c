#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
  #define debug(fmt, ...) fprintf(stderr, "DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
  #define debug(fmt, ...) 
#endif

typedef enum {
  TK_RESERVED,
  TK_NUM,
  TK_EOF,
} TokenKind;


typedef struct Token {
  TokenKind kind;
  int val;
  char* str;
  struct Token* next;
} Token;

Token *token;

Token *new_token(TokenKind kind, Token *cur, char *str) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  cur->next = tok;
  return tok;
}

bool at_eof() {
  return token->kind == TK_EOF;
}

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
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
    if (*p == '-' || *p == '+') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }
  }
  
  cur = new_token(TK_EOF, cur, p);
  return head.next;
}

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

int expect_number() {
  if (token->kind != TK_NUM) {
    error("Current token isn't number.");
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

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
    return 1;
  }

  token = tokenize(argv[1]);

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");
  printf("  mov rax, %d\n", expect_number());

  while (!at_eof()) {
    if (consume('+')) {
      printf("  add rax, %d\n", expect_number());
    } else if (consume('-')) {
      printf("  sub rax, %d\n", expect_number());
    } else {
      error("Meeting unexpected token.");
    }
  }

  printf("  ret\n");
  return 0;
}