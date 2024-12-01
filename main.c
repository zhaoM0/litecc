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
  int offset = 0;
  for (Var* var = prog->locals; var != NULL; var = var->next, offset += 8) {
    var->offset = offset;
  }
  prog->stack_size = offset;

  // Traverse the AST to emit assembly.
  codegen(prog);

  return 0;
}