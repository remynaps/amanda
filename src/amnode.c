#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "amnode.h"

//private methods
void delEmptyNames(Node **node);
void freeNode(Node *node);
void printNodes(Node *node);
 
Node *createNode(char *name, char *function) {
        Node *node = malloc(sizeof(Node));
        node->next = NULL;
        node->name = (char *)malloc(sizeof(char) * strlen(name) + 1);
        strcpy(node->name, name);
        node->function = (char *)malloc(sizeof(char) * strlen(function) + 1);
        strcpy(node->function, function);
        return node;
}
 
void appendNode(Node **node, char *name, char *function) {
        if (*node == NULL) {
                *node = createNode(name, function);
        }else{
                appendNode(&(*node)->next, name, function);
        }
}
 
void freeNode(Node *node) {
        free(node->name);
        free(node->function);
        free(node);
}
 
void delNode(Node **node, char *name) {
        if (*node != NULL) {
                if (strcmp((*node)->name, name) == 0) {
                        Node *temp_node = *node;
                        *node = (*node)->next;
                        freeNode(temp_node);
                        delEmptyNames(node);
                        delNode(node, name);
                }else{
                        delNode(&(*node)->next, name);
                }
        }
}

void delEmptyNames (Node **node) {
        if (*node != NULL) {
                if (strcmp((*node)->name, "") == 0 || strcmp((*node)->name, "where") == 0) {
                        Node *temp_node = *node;
                        *node = (*node)->next;
                        freeNode(temp_node);
                        delEmptyNames(node);
                }
        }
}
 
void printNodes(Node *node) {
        while (node != NULL) {
                printf("Name: %s, Function %s\n", node->name, node->function);
                node = node->next;
        }
}