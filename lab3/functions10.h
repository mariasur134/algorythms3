#ifndef FUNCTIONS10_H
#define FUNCTIONS10_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INDENT "  "

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL,
    STATUS_ERR_ALLOC,
    STATUS_ERR_SYNTAX,
    STATUS_ERR_EMPTY,
    STATUS_ERR_EOF,
    STATUS_ERR_READ,
    STATUS_ERR_WRITE,
    STATUS_ERR_OPEN_IN,
    STATUS_ERR_OPEN_OUT,
    STATUS_ERR_SAME_FILE
} status_t;


typedef struct Node {
    char* name;
    struct Node* first_child;
    struct Node* last_child;
    struct Node* next_sibling;
} Node;
// all kids of 1 parent create single-linked list

const char* status_message(status_t st){
    switch (st){
        case STATUS_OK:            return "ok";
        case STATUS_ERR_NULL:      return "null pointer";
        case STATUS_ERR_ALLOC:     return "memory allocation failed";
        case STATUS_ERR_SYNTAX:    return "wrong bracket expression";
        case STATUS_ERR_EMPTY:     return "empty expression";
        case STATUS_ERR_EOF:       return "end of file";
        case STATUS_ERR_READ:      return "cannot read input file";
        case STATUS_ERR_WRITE:     return "cannot write output file";
        case STATUS_ERR_OPEN_IN:   return "cannot open input file";
        case STATUS_ERR_OPEN_OUT:  return "cannot open output file";
        case STATUS_ERR_SAME_FILE: return "input and output must be different files";
    }
    return "unknown error";
}

Node* node_create(const char* name, size_t len){
    Node* n = (Node*)calloc(1, sizeof(Node));
    if (n == NULL)
        return NULL;
    n->name = (char*)malloc(len + 1);          // len + '\0'
    if (n->name == NULL){
        free(n);
        return NULL;
    }
    memcpy(n->name, name, len);
    n->name[len] = '\0';
    return n;
}

void node_add_child(Node* parent, Node* child){
    if (parent->last_child == NULL)
        parent->first_child = child;
    else
        parent->last_child->next_sibling = child;
    parent->last_child = child;
} //O(1)


// recursion depth = depth of the tree

void tree_destroy(Node* root){
    if (root == NULL)
        return;
    Node* child = root->first_child; // keeping the child
    while (child != NULL){
        Node* next = child->next_sibling; // saving child's sibling
        tree_destroy(child); // deleteng all of the child's kids
        child = next; // continue with child's sibling
    }
    free(root->name); // deleteng the root
    free(root);
}

int is_name_char(int c){
    return c != '\0' && !isspace(c) && c != '(' && c != ')' && c != ',';
}

void skip_spaces(const char** p){
    while (isspace((unsigned char)**p))
        (*p)++;
}

status_t parse_node(const char** p, Node** out){ // parsing node with its kids
    skip_spaces(p);
    const char* begin = *p;
    while (is_name_char((unsigned char)**p))
        (*p)++;
    if (*p == begin)
        return STATUS_ERR_SYNTAX;           // no name no node

    Node* n = node_create(begin, (size_t)(*p - begin)); // name and len of name
    if (n == NULL)
        return STATUS_ERR_ALLOC;
    *out = n;

    skip_spaces(p);
    if (**p != '(') // no braces -> it's a leaf
        return STATUS_OK;
    (*p)++; // skipped "("
    for (;;){
        Node* child = NULL;
        status_t st = parse_node(p, &child);
        if (st != STATUS_OK){
            tree_destroy(child);
            return st;
        }
        node_add_child(n, child);
        skip_spaces(p);
        if (**p == ','){ // next kid
            (*p)++;
            continue;
        }
        if (**p == ')'){
            (*p)++;
            return STATUS_OK;
        }
        return STATUS_ERR_SYNTAX;
    }
}

status_t tree_parse(const char* s, Node** root){
    if (s == NULL || root == NULL)
        return STATUS_ERR_NULL;
    *root = NULL;
    const char* p = s;
    skip_spaces(&p);
    if (*p == '\0')
        return STATUS_ERR_EMPTY;

    Node* tree = NULL;
    status_t st = parse_node(&p, &tree);
    if (st == STATUS_OK){
        skip_spaces(&p);
        if (*p != '\0')
            st = STATUS_ERR_SYNTAX;         // second root
    }
    if (st != STATUS_OK){
        tree_destroy(tree);
        return st;
    }
    *root = tree;
    return STATUS_OK;
}

status_t print_node(FILE* out, const Node* node, size_t depth){
    for (size_t i = 0; i < depth; i++)
        if (fputs(INDENT, out) == EOF)
            return STATUS_ERR_WRITE;
    if (fprintf(out, "%s\n", node->name) < 0)
        return STATUS_ERR_WRITE;
    for (const Node* child = node->first_child; child != NULL; child = child->next_sibling){
        status_t st = print_node(out, child, depth + 1);
        if (st != STATUS_OK)
            return st;
    }
    return STATUS_OK;
}

status_t tree_print(FILE* out, const Node* root){
    if (out == NULL || root == NULL)
        return STATUS_ERR_NULL;
    return print_node(out, root, 0);
}
// fast and forward

int fast_pow(int a, int base_bin) {
    int last;
    int temp = a;
    int res = 1;
    while(base_bin != 0) {
        last = base_bin & 1;
        if (last == 1) res *= temp;
        base_bin >>= 1;
        temp *= temp;
    }
}

status_t read_line(FILE* in, char** line){
    if (in == NULL || line == NULL)
        return STATUS_ERR_NULL;
    *line = NULL;
    size_t cap = 2, len = 0;
    char* buf = (char*)malloc(cap);
    if (buf == NULL)
        return STATUS_ERR_ALLOC;
    int ch;
    while ((ch = fgetc(in)) != EOF && ch != '\n'){
        if (len + 1 >= cap){
            char* bigger = (char*)realloc(buf, cap * 2);
            if (bigger == NULL){
                free(buf);
                return STATUS_ERR_ALLOC;
            }
            buf = bigger;
            cap *= 2;
        }
        buf[len++] = (char)ch;
    }
    if (ferror(in)){
        free(buf);
        return STATUS_ERR_READ;
    }
    if (ch == EOF && len == 0){
        free(buf);
        return STATUS_ERR_EOF;
    }
    buf[len] = '\0';
    *line = buf;
    return STATUS_OK;
}

status_t process_stream(FILE* in, FILE* out, size_t* bad_lines){
    if (in == NULL || out == NULL || bad_lines == NULL)
        return STATUS_ERR_NULL;
    *bad_lines = 0;
    size_t number = 0;
    for (;;){
        char* line = NULL;
        status_t st = read_line(in, &line);
        if (st == STATUS_ERR_EOF)
            return STATUS_OK;
        if (st != STATUS_OK)
            return st;
        number++;

        Node* root = NULL;
        st = tree_parse(line, &root);
        if (st == STATUS_ERR_EMPTY){
            free(line);
            continue;
        }
        if (st == STATUS_ERR_SYNTAX){
            (*bad_lines)++;
            int r = fprintf(out, "%zu: %s\nError: %s\n\n", number, line, status_message(st));
            free(line);
            if (r < 0)
                return STATUS_ERR_WRITE;
            continue;
        }
        if (st != STATUS_OK){
            free(line);
            return st;
        }

        int r = fprintf(out, "%zu: %s\n", number, line);
        free(line);
        if (r < 0){
            tree_destroy(root);
            return STATUS_ERR_WRITE;
        }
        st = tree_print(out, root);
        tree_destroy(root);
        if (st != STATUS_OK)
            return st;
        if (fputc('\n', out) == EOF)
            return STATUS_ERR_WRITE;
    }
}

#endif
