#include "9cc.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// global environment variable
char *user_input;
Token *current_token;
Node *code[1000];
int code_len;
int stack_size;

typedef struct LVar LVar;
struct LVar {
  LVar *next;
  Token *tok;
  int offset;
};

static LVar *locals;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_num(int value) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_NUM;
  node->value = value;
  return node;
}

Node *new_node_lvar(LVar *lvar) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = ND_LVAR;
  node->offset = lvar->offset;
  return node;
}

Token *new_token(TokenKind kind, Token *prev_token, char *str, int length) {
  Token *new_tok = calloc(1, sizeof(Token));
  new_tok->kind = kind;
  new_tok->str = str;
  new_tok->length = length;
  prev_token->next = new_tok;
  return new_tok;
}

void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

void error_at(char *location, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int position = location - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", position, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

bool equal(Token *tok, char *op) {
  return tok->kind == TK_RESERVED && strlen(op) == tok->length &&
         memcmp(tok->str, op, tok->length) == 0;
}

bool consume(char *op) {
  if (!equal(current_token, op))
    return false;
  current_token = current_token->next;
  return true;
}

Token *consume_ident() {
  if (current_token->kind != TK_IDENT)
    return NULL;
  Token *tok = current_token;
  current_token = current_token->next;
  return tok;
}

void expect_symbol(char *op) {
  if (!equal(current_token, op))
    error_at(current_token->str, "expected: \"%s\" but got: \"%.*s\"", op,
             current_token->length, current_token->str);
  current_token = current_token->next;
}

int expect_number() {
  if (current_token->kind != TK_NUM)
    error_at(current_token->str, "expected a number");
  int val = current_token->val;
  current_token = current_token->next;
  return val;
}

bool at_eof() { return current_token->kind == TK_EOF; }

bool startswith(char *target_str, char *prefix) {
  return memcmp(target_str, prefix, strlen(prefix)) == 0;
}

bool is_ident1(char c) { return isalpha((unsigned char)c) || c == '_'; }

bool is_ident2(char c) { return isalnum((unsigned char)c) || c == '_'; }

Token *tokenize() {
  char *input_ptr = user_input;
  Token head;
  head.next = NULL;
  Token *tail = &head;

  while (*input_ptr) {
    if (isspace((unsigned char)*input_ptr)) {
      input_ptr++;
      continue;
    }

    if (startswith(input_ptr, "return") && !is_ident2(input_ptr[6])) {
      tail = new_token(TK_RESERVED, tail, input_ptr, 6);
      input_ptr += 6;
      continue;
    }

    if (startswith(input_ptr, "==") || startswith(input_ptr, "!=") ||
        startswith(input_ptr, "<=") || startswith(input_ptr, ">=")) {
      tail = new_token(TK_RESERVED, tail, input_ptr, 2);
      input_ptr += 2;
      continue;
    }

    if (strchr("+-*/()<>;=", *input_ptr)) {
      tail = new_token(TK_RESERVED, tail, input_ptr, 1);
      input_ptr++;
      continue;
    }

    if (is_ident1(*input_ptr)) {
      char *token_start = input_ptr;
      input_ptr++;
      while (is_ident2(*input_ptr))
        input_ptr++;
      tail = new_token(TK_IDENT, tail, token_start, input_ptr - token_start);
      continue;
    }

    if (isdigit((unsigned char)*input_ptr)) {
      tail = new_token(TK_NUM, tail, input_ptr, 0);
      char *token_start = input_ptr;
      tail->val = strtol(input_ptr, &input_ptr, 10);
      tail->length = input_ptr - token_start;
      continue;
    }

    error_at(input_ptr, "unexpected character");
  }

  new_token(TK_EOF, tail, input_ptr, 0);
  return head.next;
}

LVar *find_lvar(Token *tok) {
  for (LVar *var = locals; var; var = var->next) {
    if (var->tok->length == tok->length &&
        memcmp(tok->str, var->tok->str, tok->length) == 0) {
      return var;
    }
  }
  return NULL;
}

void program() {
  locals = NULL;
  code_len = 0;

  while (!at_eof()) {
    if (code_len >= 1000)
      error("too many statements");
    code[code_len++] = stmt();
  }

  int max_offset = 0;
  for (LVar *var = locals; var; var = var->next) {
    if (max_offset < var->offset)
      max_offset = var->offset;
  }
  stack_size = (max_offset + 15) / 16 * 16;
}

Node *stmt() {
  if (consume("return")) {
    Node *node = new_node(ND_RETURN, expr(), NULL);
    expect_symbol(";");
    return node;
  }

  Node *node = expr();
  if (consume(";") || at_eof())
    return node;

  error_at(current_token->str, "expected ';'");
  return NULL;
}

Node *expr() { return assign(); }

Node *assign() {
  Node *node = equality();

  if (consume("="))
    return new_node(ND_ASSIGN, node, assign());
  return node;
}

Node *equality() {
  Node *node = relational();

  for (;;) {
    if (consume("=="))
      node = new_node(ND_EQ, node, relational());
    else if (consume("!="))
      node = new_node(ND_NE, node, relational());
    else
      return node;
  }
}

Node *relational() {
  Node *node = add();

  for (;;) {
    if (consume("<"))
      node = new_node(ND_LT, node, add());
    else if (consume("<="))
      node = new_node(ND_LE, node, add());
    else if (consume(">"))
      node = new_node(ND_LT, add(), node);
    else if (consume(">="))
      node = new_node(ND_LE, add(), node);
    else
      return node;
  }
}

Node *add() {
  Node *node = mul();

  for (;;) {
    if (consume("+"))
      node = new_node(ND_ADD, node, mul());
    else if (consume("-"))
      node = new_node(ND_SUB, node, mul());
    else
      return node;
  }
}

Node *mul() {
  Node *node = unary();

  for (;;) {
    if (consume("*"))
      node = new_node(ND_MUL, node, unary());
    else if (consume("/"))
      node = new_node(ND_DIV, node, unary());
    else
      return node;
  }
}

Node *unary() {
  if (consume("+"))
    return unary();
  if (consume("-"))
    return new_node(ND_SUB, new_node_num(0), unary());
  return primary();
}

Node *primary() {
  if (consume("(")) {
    Node *node = expr();
    expect_symbol(")");
    return node;
  }

  Token *tok = consume_ident();
  if (tok) {
    LVar *var = find_lvar(tok);
    if (!var) {
      var = calloc(1, sizeof(LVar));
      var->next = locals;
      var->tok = tok;
      if (locals)
        var->offset = locals->offset + 8;
      else
        var->offset = 8;
      locals = var;
    }
    return new_node_lvar(var);
  }

  return new_node_num(expect_number());
}
