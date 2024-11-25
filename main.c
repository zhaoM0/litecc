#include "litecc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s invalid number of arguments", argv[0]);
  }
  
  user_input = argv[1];
  
  // Scanner
  token = tokenize();
  
  // Parser
  Node* node = expr();

  // CodeGen
  codegen(node);

  return 0;
}