#include "litecc.h"

// All local variable instance created during parsing are
// accumulated to this list
Var* locals;

// Find a local variable by name.
static Var *find_var(Token* tok) {
  for (Var* vp = locals; vp != NULL; vp = vp->next) {
    if (vp->len == tok->len && 
        !strncmp(vp->name, tok->str, tok->len)) {
      return vp;
    }
  }
  return NULL;
}

static Node *new_node(NodeKind kind) {
  Node* node = (Node *)calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_binary(NodeKind kind, Node* lhs, Node* rhs) {
  Node* node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_unary(NodeKind kind, Node* expr) {
  Node* node = new_node(kind);
  node->lhs = expr;
  return node;
}

static Node *new_num(long val) {
  Node* node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node *new_var(Var *var) {
  Node* node = new_node(ND_VAR);
  node->var = var;
  return node;
}

static Var *new_lvar(char* name) {
  Var *var = calloc(1, sizeof(Var));
  var->next = locals;
  var->name = name;
  var->len  = strlen(name);
  locals = var;
  return var;
}

static Node* stmt(void);
static Node* expr(void);
static Node* assign(void);
static Node* equality(void);
static Node* relational(void);
static Node* add(void);
static Node* mul(void);
static Node* unary(void);
static Node* primary(void);


// program = stmt*
Function* program(void) {
  locals = NULL;

  Node head = {};
  Node* cur = &head;

  while (!at_eof()) {
    cur->next = stmt();
    cur = cur->next;
  }

  Function* prog = calloc(1, sizeof(Function));
  prog->node = head.next;
  prog->locals = locals;
  return prog;
}

static Node *read_expr_stmt(void) {
  return new_unary(ND_EXPR_STMT, expr());
}

// stmt = "return" expr ";"
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "for" "(" expr? ";" expr? ";" expr? ")" stmt
//      | "{" stmt* "}"
//      | expr ";"
static Node* stmt(void) {
  if (consume("return")) {
    Node* node = new_unary(ND_RETURN, expr());
    expect(";");
    return node;
  }

  if (consume("if")) {
    Node* node = new_node(ND_IF);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    if (consume("else")) {
      node->els = stmt();
    }
    return node;
  }

  if (consume("while")) {
    Node* node = new_node(ND_WHILE);
    expect("(");
    node->cond = expr();
    expect(")");
    node->then = stmt();
    return node;
  }

  if (consume("for")) {
    Node* node = new_node(ND_FOR);
    expect("(");
    if (!consume(";")) {
      node->init = read_expr_stmt();
      expect(";");
    }
    if (!consume(";")) {
      node->cond = expr();
      expect(";");
    }
    if (!consume(")")) {
      node->inc = read_expr_stmt();
      expect(")");
    }
    node->then = stmt();
    return node;
  }

  if (consume("{")) {
    Node  head = {};
    Node* cur = &head;

    while (!consume("}")) {
      cur->next = stmt();
      cur = cur->next;
    }
    
    Node* node = new_node(ND_BLOCK);
    node->block = head.next;
    return node;
  }

  Node *node = read_expr_stmt();
  expect(";");
  return node;
}

// expr = assign
static Node* expr(void) {
  return assign();
}

// assgin = equality ("=" assign)?
static Node* assign(void) {
  Node* node = equality();
  if (consume("=")) {
    node = new_binary(ND_ASSIGN, node, assign());
  }
  return node;
}

// equality = relational ("==" relational | "!=" relational)*
static Node* equality(void) {
  Node* node = relational();

  while(1) {
    if (consume("==")) {
      node = new_binary(ND_EQ, node, relational());
    } else if (consume("!=")) {
      node = new_binary(ND_NE, node, relational());
    } else {
      return node;
    }
  }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node* relational(void) {
  Node* node = add();

  while(1) {
    if (consume("<")) {
      node = new_binary(ND_LT, node, add());
    } else if (consume("<=")) {
      node = new_binary(ND_LE, node, add());
    } else if (consume(">")) {
      node = new_binary(ND_LT, add(), node);
    } else if (consume(">=")) {
      node = new_binary(ND_LE, add(), node);
    } else {
      return node;
    }
  }
}

// add = mul ("+" mul | "-" mul)*
static Node* add(void) {
  Node* node = mul();

  while(1) {
    if (consume("+")) {
      node = new_binary(ND_ADD, node, mul());
    } else if (consume("-")) {
      node = new_binary(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

// mul = unary ("*" unary | "/" unary)*
static Node* mul(void) {
  Node* node = unary();

  while(1) {
    if (consume("*")) {
      node = new_binary(ND_MUL, node, unary());
    } else if (consume("/")) {
      node = new_binary(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

// unary = ("+" | "-")? unary
//       | primary
static Node* unary(void) {
  if (consume("+")) {
    return unary();
  } else if (consume("-")) {
    return new_binary(ND_SUB, new_num(0), unary());
  } 
  return primary();
}

// func-args = "(" (assign ("," assign)*)? ")"
static Node* func_args() {
  if (consume(")")) {
    return NULL;
  }

  Node* head = assign();
  Node* cur = head;
  while (consume(",")) {
    cur->next = assign();
    cur = cur->next;
  }
  expect(")");
  return head;
}

// primary = num
//         | ident func-args?
//         | "(" expr ")" 
static Node* primary(void) {
  if (consume("(")) {
    Node* node = expr();
    expect(")");
    return node;
  }

  Token* tok = consume_ident();
  if (tok) {
    // Function call
    if (consume("(")) {
      Node* node = new_node(ND_FUNCALL);
      node->funcname = strndup(tok->str, tok->len);
      node->args = func_args();
      return node;
    }

    // Variable
    Var* var = find_var(tok);
    if (!var) {
      var = new_lvar(strndup(tok->str, tok->len));
    }
    return new_var(var);
  }

  return new_num(expect_number());
}