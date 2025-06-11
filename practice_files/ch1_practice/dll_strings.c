#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Node Node;
struct Node {
    char *data;
    Node *next;
    Node *prev;
};

Node* node_create(char* data, Node* prev, Node* next) {
    Node* n = malloc(sizeof(Node));
    n->data = strdup(data);
    n->next = next;
    n->prev = prev;
    return n;
}

Node* create_head(char* data) {
    return node_create(data, NULL, NULL);
}

//returns the head
Node* append_node(Node* dll, char* data) {
    Node* original_head = dll;
    while(dll->next) {
        dll = dll->next;
    }
    dll->next = node_create(data, dll, NULL);
    return original_head;
}

Node* prepend_node(Node* dll, char* data) {
    if(!dll)
        return node_create(data, NULL, NULL);
    Node *n = node_create(data, NULL, dll);
    dll->prev = n;
    return n;
}

Node* insert_at(Node* dll, char* data, int index) {
    Node* original_head = dll;
    for(int i = 0; i < index - 1; i++) {
        //dll has no next pointer.
        if(!dll->next) {
            break;
        }
        dll = dll->next;
    }
    Node *n = node_create(data, dll, dll->next);
    if(dll->next)
        dll->next->prev = n;
    dll->next = n;
    return original_head;
}

int find_element(Node* dll, char* str) {
    while(dll->next) {
        if(strcmp(dll->data, str) == 0)
            return 1;
        dll = dll->next;
    }
    return 0;
}

void free_dll(Node* dll) {
    while(dll) {
        free(dll->data);
        Node *next = dll->next;
        free(dll);
        dll = next;
    }
}

void print_dll(Node* dll) {
    int i = 0;
    while(dll) {
        if(dll->data)
            printf("%d %s\n", i, dll->data);
        i++;
        dll = dll->next;
    }
}

int main(void) {
    puts("hello");
    Node* head = create_head("hello world");
    print_dll(head);
    head = append_node(head, "next");
    head = append_node(head, "second");
    head = prepend_node(head, "first");
    head = insert_at(head, "inserted", 5);
    print_dll(head);
    if (find_element(head, "first"))
        puts("found");
    else
        puts("not found");
    if (find_element(head, "fadfa"))
        puts("found");
    else
        puts("not found");
    free_dll(head);
}