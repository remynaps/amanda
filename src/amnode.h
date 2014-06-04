#ifndef AMNODE_H
#define AMNODE_H

typedef struct NODE {
    struct NODE *next;
    char *name, *function;
} Node;

void appendNode(Node **node, char *name, char *function);

void delNode(Node *node, char *name);

#endif
