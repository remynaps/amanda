#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amnode.h"


//private methods
static void delEmptyNames(Node **node);
static void freeNode(Node **node);

Node *createNode(const char *name, const char *function)
{
  Node *node = malloc(sizeof(Node));
  node->next = NULL;
  node->name = (char *)malloc(sizeof(char) * strlen(name) + 1);
  strcpy(node->name, name);
  node->function = (char *)malloc(sizeof(char) * strlen(function) + 1);
  strcpy(node->function, function);
  return node;
}

void printNodes(Node *node)
{
  while (node != NULL)
  {
    printf("%s", node->function);
    node = node->next;
  }
}

void appendNode(Node **node, const char *name, const char *function)
{
  if (*node == NULL)
  {
    *node = createNode(name, function);
  }
  else
  {
    appendNode(&(*node)->next, name, function);
  }
}

void delNode(Node **node, const char *name)
{
  if (*node != NULL)
  {
    if (strcmp((*node)->name, name) == 0)
    {
      Node *temp_node = *node;
      *node = (*node)->next;
      freeNode(&temp_node);
      delEmptyNames(node);
      delNode(node, name);
    }
    else
    {
      delNode(&(*node)->next, name);
    }
  }
}

void clearNode(Node **node) {
  if (*node != NULL) {
    clearNode(&(*node)->next);
    freeNode(node);
  }
}

static void delEmptyNames (Node **node)
{
  if (*node != NULL)
  {
    if (strcmp((*node)->name, "") == 0 || strcmp((*node)->name, "where") == 0)
    {
      Node *temp_node = *node;
      *node = (*node)->next;
      freeNode(&temp_node);
      delEmptyNames(node);
    }
  }
}

static void freeNode(Node **node)
{
  free((*node)->name);
  free((*node)->function);
  free(*node);
  *node = NULL;
}
