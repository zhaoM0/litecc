#include "litecc.h"

char*  user_input;
Token* token;

// Util function for display token list.
void dispaly_tokens(void) {
  Token *cur = token;
  while(cur->kind != TK_EOF) {
    switch (cur->kind) {
      case TK_NUM: 
        printf("(NUM, %ld) -> ", cur->val); 
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

// Reports an error and exit.
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
static void verror_at(char* loc, char* fmt, va_list ap) {
  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, " ");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// Reports an error location and exit.
void error_at(char* loc, char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  verror_at(loc, fmt, args);
}

// Reports an error location and exit.
void error_tok(Token* tok, char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  verror_at(tok->str, fmt, ap);
}

// Consumes the current token if it matches `op`.
Token* consume(char *op) {
  if (token->kind != TK_RESERVED ||
      token->len != strlen(op) || 
      strncmp(token->str, op, token->len)) {
    return NULL;
  }
  Token* t = token;
  token = token->next;
  return t;
}

// Consume the current token if it is an identifier.
Token* consume_ident(void) {
  if (token->kind != TK_IDENT) {
    return NULL;
  }
  Token* t = token;
  token = token->next;
  return t;
}

// Ensure that the current token is `op`.
void expect(char *op) {
  if (consume(op) == NULL)
    error_tok(token, "expected \"%s\"", op);
}

// Ensure that the current token is TK_NUM.
long expect_number(void) {
  if (token->kind != TK_NUM) {
    error_tok(token, "expected number.");
  }
  long val = token->val;
  token = token->next;
  return val;
}

// Ensure that the current token is TK_IDNET.
char *expect_ident(void) {
  if (token->kind != TK_IDENT) {
    error_tok(token, "expected an identifier");
  }
  char* s = strndup(token->str, token->len);
  token = token->next;
  return s;
}

// Check is or not reach to file end.
bool at_eof() {
  return token->kind == TK_EOF;
}

// Create a new token and add it as the next token of `cur`.
static Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

static bool startwith(char *p, const char *t) {
  return strncmp(p, t, strlen(t)) == 0;
}

static bool is_alpha(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

static bool is_alnum(char c) {
  return is_alpha(c) || ('0' <= c && c <= '9');
}

static char *starts_with_reserved(char *p) {
  // Keyword
  static char* kw[] = {"return", "if", "else", "while", "for"};

  for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++) {
    int len = strlen(kw[i]);
    if (startwith(p, kw[i]) && !is_alnum(p[len])) {
      return kw[i];
    }
  }

  // Multi-letter punctuator
  static char* ops[] = {"==", "!=", "<=", ">="};

  for (int i = 0; i < sizeof(ops) / sizeof(*ops); i++) {
    if (startwith(p, ops[i])) {
      return ops[i];
    }
  }

  return NULL;
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
    
    // Keywords or multi-letter punctuators
    char *kw = starts_with_reserved(p);
    if (kw != NULL) {
      int len = strlen(kw);
      cur = new_token(TK_RESERVED, cur, p, len);
      p += len;
      continue;
    }
    
    // Identifier
    if (is_alpha(*p)) {
      char *q = p++;
      while (is_alnum(*p))
        ++p;
      cur = new_token(TK_IDENT, cur, q, p - q);
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