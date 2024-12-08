#include "litecc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s invalid number of arguments", argv[0]);
  }
  
  user_input = argv[1];
  
  // Scanner
  token = tokenize();
  
  // Parser
  Function* prog = program();

  // Assign offsets to local variables.
  for (Function* fn = prog; fn != NULL; fn = fn->next) {
    int offset = 0;
    for (Var* var = fn->locals; var != NULL; var = var->next) {
      offset += 8;
      var->offset = offset;
    }
    fn->stack_size = offset;
  }

  // Traverse the AST to emit assembly.
  codegen(prog);

  return 0;
}