#include "litecc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s invalid number of arguments", argv[0]);
  }
  
  user_input = argv[1];
  
  // Scanner
  token = tokenize();
  
  // Parser
  Program* prog = program();

  // Assign offsets to local variables.
  for (Function* fn = prog->fns; fn != NULL; fn = fn->next) {
    int offset = 0;
    for (VarList* vl = fn->locals; vl != NULL; vl = vl->next) {
      Var* var = vl->var;
      offset += var->ty->size;
      var->offset = offset;
    }
    fn->stack_size = offset;
  }

  // Traverse the AST to emit assembly.
  codegen(prog);

  return 0;
}