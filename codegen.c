#include "litecc.h"

static char *argreg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static int labelseq = 1;
static char* funcname;

static void gen(Node* node);

// Pushes the given node's address to the stack
static void gen_addr(Node* node) {
  switch (node->kind) {
  case ND_VAR: {
    Var* var = node->var;
    if (var->is_local) {
      printf("  lea rax, [rbp-%d]\n", var->offset);
      printf("  push rax\n");
    } else {
      printf("  push offset %s\n", var->name);
    }
    return;
  }
  case ND_DEREF:
    gen(node->lhs);
    return;
  }

  error_tok(node->tok, "not an lvalue");
}

static void gen_lval(Node* node) {
  if (node->ty->kind == TY_ARRAY)
    error_tok(node->tok, "not an lvalue");
  gen_addr(node);
}

static void load(void) {
  printf("  pop rax\n");
  printf("  mov rax, [rax]\n");
  printf("  push rax\n");
}

static void store(void) {
  printf("  pop rdi\n");
  printf("  pop rax\n");
  printf("  mov [rax], rdi\n");
  printf("  push rdi\n");
}

// Generate code for a given node.
static void gen(Node* node) {
  if (node == NULL) {
    return;
  }

  switch (node->kind) {
    case ND_NULL:
      return;
    case ND_NUM:
      printf("  push %ld\n", node->val);
      return;
    case ND_EXPR_STMT:
      gen(node->lhs);
      printf("  add rsp, 8\n");
      return;
    case ND_VAR:
      gen_addr(node);
      if (node->ty->kind != TY_ARRAY)
        load();
      return;
    case ND_ASSIGN:
      gen_lval(node->lhs);
      gen(node->rhs);
      store();
      return;
    case ND_ADDR:
      gen_addr(node->lhs);
      return;
    case ND_DEREF:
      gen(node->lhs);
      if (node->ty->kind != TY_ARRAY)
        load();
      return;
    case ND_IF: {
      int seq = labelseq++;
      if (node->els) {
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je  .L.else.%d\n", seq);
        gen(node->then);
        printf("  jmp .L.end.%d\n", seq);
        printf(".L.else.%d:\n", seq);
        gen(node->els);
        printf(".L.end.%d:\n", seq);
      } else {
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je  .L.end.%d\n", seq);
        gen(node->then);
        printf(".L.end.%d:\n", seq);
      }
      return;
    }
    case ND_WHILE: {
      int seq = labelseq++;
      printf(".L.begin.%d:\n", seq);
      gen(node->cond);
      printf("  pop rax\n");
      printf("  cmp rax, 0\n");
      printf("  je  .L.end.%d\n", seq);
      gen(node->then);
      printf("  jmp .L.begin.%d\n", seq);
      printf(".L.end.%d:\n", seq);
      return;
    }
    case ND_FOR: {
      int seq = labelseq++;
      if (node->init) { 
        gen(node->init);
      }
      printf(".L.begin.%d:\n", seq);
      if (node->cond) { 
        gen(node->cond);
        printf("  pop rax\n");
        printf("  cmp rax, 0\n");
        printf("  je  .L.end.%d\n", seq);
      } 
      gen(node->then);
      if (node->inc) {
        gen(node->inc);
      }
      printf("  jmp .L.begin.%d\n", seq);
      printf(".L.end.%d:\n", seq);
      return;
    }
    case ND_FUNCALL: {
      int nargs = 0;
      for (Node* arg = node->args; arg != NULL; arg = arg->next) {
        gen(arg);
        nargs++;
      }
      for (int i = nargs - 1; i >= 0; i--) {
        printf("  pop %s\n", argreg[i]);
      }

      // We need to align RSP to a 16 byte boundary before
      // calling a function because it is an ABI requirement.
      // RAX is set to 0 for variadic function.
      int seq = labelseq++;
      printf("  mov rax, rsp\n");
      printf("  and rax, 15\n");
      printf("  jnz .L.call.%d\n", seq);  // jump if not zero
      printf("  mov rax, 0\n");           // for variable parameters
      printf("  call %s\n", node->funcname);
      printf("  jmp .L.end.%d\n", seq);
      printf(".L.call.%d:\n", seq);
      printf("  sub rsp, 8\n");
      printf("  mov rax, 0\n");
      printf("  call %s\n", node->funcname);
      printf("  add rsp, 8\n");
      printf(".L.end.%d:\n", seq);
      printf("  push rax\n");
      return;
    }
    case ND_BLOCK: {
      for (Node* cur = node->block; cur != NULL; cur = cur->next) {
        gen(cur);
      }
      return;
    }
    case ND_RETURN: {
      gen(node->lhs);
      printf("  pop rax\n");
      printf("  jmp .L.return.%s\n", funcname);
      return;
    }
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");
  
  switch (node->kind) {
    case ND_ADD: 
      printf("  add rax, rdi\n"); 
      break;
    case ND_PTR_ADD:
      printf("  imul rdi, %d\n", node->ty->base->size);
      printf("  add rax, rdi\n");
      break;
    case ND_SUB: 
      printf("  sub rax, rdi\n");
      break;
    case ND_PTR_SUB:
      printf("  imul rdi, %d\n", node->ty->base->size);
      printf("  sub rax, rdi\n");
      break;
    case ND_PTR_DIFF:
      printf("  sub rax, rdi\n");
      printf("  cqo\n");
      printf("  mov rdi, %d\n", node->lhs->ty->base->size);
      printf("  idiv rdi\n");
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

static void emit_data(Program* prog) {
  printf(".data\n");

  for (VarList* vl = prog->globals; vl != NULL; vl = vl->next) {
    Var* var = vl->var;
    printf("%s:\n", var->name);
    printf("  .zero %d\n", var->ty->size);
  }
}

static void emit_text(Program* prog) {
  printf(".text\n");

  for (Function* fn = prog->fns; fn != NULL; fn = fn->next) {
    printf(".global %s\n", fn->name);
    printf("%s:\n", fn->name);
    funcname = fn->name;

    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", fn->stack_size);

    // Push arguments to the stack
    int i = 0;
    for (VarList* vl = fn->params; vl != NULL; vl = vl->next) {
      Var* var = vl->var;
      printf("  mov [rbp-%d], %s\n", var->offset, argreg[i++]);
    }

    // Emit code
    for (Node* cur = fn->node; cur != NULL; cur = cur->next) {
      gen(cur);
    }

    // Epilogue
    printf(".L.return.%s:\n", funcname);
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n"); 
  }
}

void codegen(Program* prog) {
  printf(".intel_syntax noprefix\n");
  emit_data(prog);
  emit_text(prog);
}