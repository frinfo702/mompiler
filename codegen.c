#include "9cc.h"

typedef struct Node Node;

static void gen_lval(Node *node) {
  if (node->kind != ND_LVAR)
    error("left value of assignment is not a variable");

  printf("  lea -%d(%%rbp), %%rax\n", node->offset);
  printf("  push %%rax\n");
}

void gen(Node *node) {
  switch (node->kind) {
  case ND_NUM:
    printf("  push $%d\n", node->value);
    return;
  case ND_LVAR:
    gen_lval(node);
    printf("  pop %%rax\n");
    printf("  mov (%%rax), %%rax\n");
    printf("  push %%rax\n");
    return;
  case ND_ASSIGN:
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("  pop %%rdi\n");
    printf("  pop %%rax\n");
    printf("  mov %%rdi, (%%rax)\n");
    printf("  push %%rdi\n");
    return;
  case ND_RETURN:
    gen(node->lhs);
    printf("  pop %%rax\n");
    printf("  jmp .Lreturn\n");
    return;
  default:
    break;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop %%rdi\n");
  printf("  pop %%rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add %%rdi, %%rax\n");
    break;
  case ND_SUB:
    printf("  sub %%rdi, %%rax\n");
    break;
  case ND_MUL:
    printf("  imul %%rdi, %%rax\n");
    break;
  case ND_DIV:
    printf("  cqto\n");
    printf("  idiv %%rdi\n");
    break;
  case ND_EQ:
    printf("  cmp %%rdi, %%rax\n");
    printf("  sete %%al\n");
    printf("  movzbq %%al, %%rax\n");
    break;
  case ND_NE:
    printf("  cmp %%rdi, %%rax\n");
    printf("  setne %%al\n");
    printf("  movzbq %%al, %%rax\n");
    break;
  case ND_LT:
    printf("  cmp %%rdi, %%rax\n");
    printf("  setl %%al\n");
    printf("  movzbq %%al, %%rax\n");
    break;
  case ND_LE:
    printf("  cmp %%rdi, %%rax\n");
    printf("  setle %%al\n");
    printf("  movzbq %%al, %%rax\n");
    break;
  default:
    error("Unsupported node kind: %d", node->kind);
  }

  printf("  push %%rax\n");
}
