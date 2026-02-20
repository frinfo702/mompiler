#include "9cc.h"

#ifdef __APPLE__
#define ENTRY_LABEL "_main"
#else
#define ENTRY_LABEL "main"
#endif

int main(int argc, char **argv) {
  if (argc != 2)
    error("%s: invalid number of arguments", argv[0]);

  user_input = argv[1];
  current_token = tokenize();
  program();

  printf(".globl %s\n", ENTRY_LABEL);
  printf("%s:\n", ENTRY_LABEL);

  printf("  push %%rbp\n");
  printf("  mov %%rsp, %%rbp\n");
  printf("  sub $%d, %%rsp\n", stack_size);

  printf("  mov $0, %%rax\n");
  for (int i = 0; i < code_len; i++) {
    gen(code[i]);
    if (code[i]->kind != ND_RETURN)
      printf("  pop %%rax\n");
  }

  printf(".Lreturn:\n");
  printf("  mov %%rbp, %%rsp\n");
  printf("  pop %%rbp\n");
  printf("  ret\n");
  return 0;
}
