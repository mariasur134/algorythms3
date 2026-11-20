#ifndef FUNCTIONS3_H
#define FUNCTIONS3_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTE_VALUES 256 // [0:255]
#define MAX_CODE_LEN (BYTE_VALUES - 1) // вырожденное дерево - список, глубина последнего 255

typedef enum {
    STATUS_OK,
    STATUS_ERR_NULL,
    STATUS_ERR_ALLOC,
    STATUS_ERR_OPEN,
    STATUS_ERR_READ,
    STATUS_ERR_HEAP_EMPTY,
    STATUS_ERR_HEAP_FULL
} status_t;

// haffman node
typedef struct HNode {
    int symbol;                 // value itself, inner node has -1
    unsigned long long weight;  // summ of kids' byte frequency
    struct HNode* left;
    struct HNode* right;
} HNode;


typedef struct {
    HNode* items[BYTE_VALUES]; // <= 256 leafes, heap shrinks, so 256 should cover it
    size_t size;
} MinHeap;


// result for 1 file
typedef struct {
    unsigned long long freq[BYTE_VALUES];
    char* codes[BYTE_VALUES];
    unsigned long long file_size;
    unsigned long long encoded_bits;
    unsigned long long encoded_bytes;
    double saving_percent;
} Analysis;

const char* status_message(status_t st){
    switch (st){
        case STATUS_OK:            return "ok";
        case STATUS_ERR_NULL:      return "null pointer";
        case STATUS_ERR_ALLOC:     return "memory allocation failed";
        case STATUS_ERR_OPEN:      return "cannot open the file";
        case STATUS_ERR_READ:      return "cannot read the file";
        case STATUS_ERR_HEAP_EMPTY: return "heap is empty";
        case STATUS_ERR_HEAP_FULL:  return "heap is full";
    }
    return "unknown error";
}



void heap_init(MinHeap* h){
    h->size = 0;
}
static void swap_nodes(HNode** a, HNode** b){
    HNode* t = *a;
    *a = *b;
    *b = t;
}

// pushing to the end, then swap until "parent <= kids"
status_t heap_push(MinHeap* h, HNode* node){
    if (h == NULL || node == NULL)
        return STATUS_ERR_NULL;
    if (h->size == BYTE_VALUES)
        return STATUS_ERR_HEAP_FULL;

    size_t i = h->size;
    h->items[i] = node; // adding to the end
    h->size++;
    while (i > 0){
        size_t parent = (i - 1) / 2; // since it's an array
        if (h->items[parent]->weight <= h->items[i]->weight)
            break;
        swap_nodes(&h->items[parent], &h->items[i]);
        i = parent;
    }
    return STATUS_OK;
}

// popping the root, putting the last leaf instead, sift
status_t heap_pop(MinHeap* h, HNode** node){
    if (h == NULL || node == NULL)
        return STATUS_ERR_NULL;
    if (h->size == 0)
        return STATUS_ERR_HEAP_EMPTY;

    *node = h->items[0];
    h->size--;;
    h->items[0] = h->items[h->size];

    size_t i = 0;
    for (;;){
        size_t left = 2 * i + 1;
        size_t right = 2 * i + 2;
        size_t min = i;
        if (left < h->size && (h->items[left]->weight < h->items[min]->weight))
            min = left;
        if (right < h->size && (h->items[right]->weight < h->items[min]->weight))
            min = right;
        if (min == i)
            break;
        swap_nodes(&h->items[i], &h->items[min]);
        i = min;
    }
    return STATUS_OK;
}


static HNode* hnode_create(int symbol, unsigned long long weight, HNode* left, HNode* right){
    HNode* n = (HNode*)malloc(sizeof(HNode));
    if (n == NULL)
        return NULL;
    n->symbol = symbol;
    n->weight = weight;
    n->left = left;
    n->right = right;
    return n;
}

void tree_destroy(HNode* root){
    if (root == NULL)
        return;
    tree_destroy(root->left);
    tree_destroy(root->right);
    free(root);
}

status_t count_frequencies(const char* path, unsigned long long freq[BYTE_VALUES], unsigned long long* total){
    if (path == NULL || freq == NULL || total == NULL)
        return STATUS_ERR_NULL;
    FILE* f = fopen(path, "rb");
    if (f == NULL)
        return STATUS_ERR_OPEN;
    for (int i = 0; i < BYTE_VALUES; i++)
        freq[i] = 0;
    *total = 0;
    int ch;
    while ((ch = fgetc(f)) != EOF){
        freq[(unsigned char)ch]++;
        (*total)++;
    }
    int bad = ferror(f);
    fclose(f);
    return bad ? STATUS_ERR_READ : STATUS_OK;
}



status_t build_tree(const unsigned long long freq[BYTE_VALUES], HNode** root){
    if (freq == NULL || root == NULL)
        return STATUS_ERR_NULL;
    *root = NULL;
    MinHeap heap;
    heap_init(&heap);

    for (int b = 0; b < BYTE_VALUES; b++){
        if (freq[b] == 0)
            continue;

        // creating a bunch of nodes and pushin into heap
        HNode* leaf = hnode_create(b, freq[b], NULL, NULL);
        if (leaf == NULL){
            for (size_t i = 0; i < heap.size; i++)
                tree_destroy(heap.items[i]);
            return STATUS_ERR_ALLOC;
        }
        heap_push(&heap, leaf);
    }
    if (heap.size == 0)
        return STATUS_OK;

    while (heap.size > 1){
        HNode* left = NULL;
        HNode* right = NULL;
        heap_pop(&heap, &left);
        heap_pop(&heap, &right);
        // pop 2 kids then make inner node with value = -1
        HNode* parent = hnode_create(-1, left->weight + right->weight, left, right);
        if (parent == NULL){
            tree_destroy(left);
            tree_destroy(right);
            for (size_t i = 0; i < heap.size; i++)
                tree_destroy(heap.items[i]);
            return STATUS_ERR_ALLOC;
        }
        heap_push(&heap, parent);
    }
    return heap_pop(&heap, root); // returning pointer to the remaining node - root
}

// going down, left -> -1, rigth -> +1
static status_t assign_codes(const HNode* node, char* buf, size_t depth, char* codes[BYTE_VALUES]){
    if (node->left == NULL && node->right == NULL){ // leaf
        buf[depth] = '\0';
        codes[node->symbol] = (char*)malloc(depth + 1);
        if (codes[node->symbol] == NULL)
            return STATUS_ERR_ALLOC;
        strcpy(codes[node->symbol], buf); // copying temporary way to symbol
        return STATUS_OK;
    }
    // going left
    buf[depth] = '0';
    status_t st = assign_codes(node->left, buf, depth + 1, codes);
    if (st != STATUS_OK)
        return st;
    // going right
    buf[depth] = '1';
    return assign_codes(node->right, buf, depth + 1, codes);
}


status_t build_codes(const HNode* root, char* codes[BYTE_VALUES]){
    if (codes == NULL)
        return STATUS_ERR_NULL;
    for (int i = 0; i < BYTE_VALUES; i++)
        codes[i] = NULL;
    if (root == NULL)
        return STATUS_OK; // empty

    if (root->left == NULL && root->right == NULL){ // root only, assign 0
        codes[root->symbol] = (char*)malloc(strlen("0") + 1); // malloc 2
        if (codes[root->symbol] == NULL)
            return STATUS_ERR_ALLOC;
        strcpy(codes[root->symbol], "0");
        return STATUS_OK;
    }
    char buf[MAX_CODE_LEN + 1];
    return assign_codes(root, buf, 0, codes);
}


void analysis_destroy(Analysis* a){
    if (a == NULL)
        return;
    for (int i = 0; i < BYTE_VALUES; i++){
        free(a->codes[i]);
        a->codes[i] = NULL;
    }
}


// bit size = summ freq * len
// byte -> round
status_t analyze_file(const char* path, Analysis* a){
    if (path == NULL || a == NULL)
        return STATUS_ERR_NULL;
    for (int i = 0; i < BYTE_VALUES; i++)
        a->codes[i] = NULL;
    a->encoded_bits = 0;
    a->encoded_bytes = 0;
    a->saving_percent = 0.0;

    status_t st = count_frequencies(path, a->freq, &a->file_size);
    if (st != STATUS_OK)
        return st;

    HNode* root = NULL;
    st = build_tree(a->freq, &root);
    if (st != STATUS_OK)
        return st;
    st = build_codes(root, a->codes);
    tree_destroy(root);
    if (st != STATUS_OK){
        analysis_destroy(a);
        return st;
    }

    for (int b = 0; b < BYTE_VALUES; b++)
        if (a->codes[b] != NULL)
            a->encoded_bits += a->freq[b] * strlen(a->codes[b]);
    a->encoded_bytes = (a->encoded_bits + 7) / 8;
    if (a->file_size > 0)
        a->saving_percent = 100.0 * (1.0 - (double)a->encoded_bytes / (double)a->file_size);
    return STATUS_OK;
}


status_t analysis_print(FILE* out, const Analysis* a){
    if (out == NULL || a == NULL)
        return STATUS_ERR_NULL;
    fprintf(out, "byte  frequency  code\n");
    for (int b = 0; b < BYTE_VALUES; b++)
        if (a->codes[b] != NULL)
            fprintf(out, "%-5d %-10llu %s\n", b, a->freq[b], a->codes[b]);
    fprintf(out, "original size: %llu bytes\n", a->file_size);
    fprintf(out, "encoded size: %llu bits = %llu bytes\n", a->encoded_bits, a->encoded_bytes);
    fprintf(out, "saving: %.2f%%\n", a->saving_percent);
    return STATUS_OK;
}

#endif
