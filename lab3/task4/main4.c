#include "functions4.h"

// part 1: show every String function
status_t demo_strings(void){
    String a = {NULL, 0}, b = {NULL, 0}, c = {NULL, 0};
    String* heap = NULL;

    status_t st = string_create(&a, "apple");
    if (st == STATUS_OK) st = string_create(&b, "pear");
    if (st == STATUS_OK) st = string_create(&c, "apricot");
    if (st == STATUS_OK){
        printf("a=\"%s\" b=\"%s\" c=\"%s\"\n", a.data, b.data, c.data);
        printf("order(a,b)=%d (5 > 4), order(a,c)=%d (5 < 7), lex(a,b)=%d\n",
               string_compare_order(&a, &b), string_compare_order(&a, &c),
               string_compare_lex(&a, &b));
        printf("equal(a,a)=%d, equal(a,b)=%d\n", string_equal(&a, &a), string_equal(&a, &b));
        st = string_copy(&b, &c);                   // b <- c (existing String)
    }
    if (st == STATUS_OK){
        printf("copy b<-c: b=\"%s\"(%d)\n", b.data, b.length);
        st = string_clone(&a, &heap);               // new String in the heap
    }
    if (st == STATUS_OK){
        printf("clone of a: \"%s\"(%d)\n", heap->data, heap->length);
        st = string_concat(&a, &c);                 // a += c
    }
    if (st == STATUS_OK){
        printf("concat a+=c: \"%s\"(%d), clone is still \"%s\"\n", a.data, a.length, heap->data);
        st = string_concat(&a, &a);                 // a += a
    }
    if (st == STATUS_OK)
        printf("concat a+=a: \"%s\"(%d)\n", a.data, a.length);

    string_destroy(&a);
    string_destroy(&b);
    string_destroy(&c);
    string_free(heap);
    return st;
}

void print_menu(void){
    printf("\n===== Post office =====\n"
           "1 - add mail\n2 - remove mail by identifier\n3 - find mail by identifier\n"
           "4 - show all mails (sorted by index, id)\n5 - mark mail as delivered\n"
           "6 - show delivered mails (by creation time)\n"
           "7 - show expired mails (by creation time)\n"
           "8 - demo of the String functions\n0 - exit\n");
}

status_t action_add(Post* post){
    Mail m;
    status_t st = read_mail(&m);
    if (st == STATUS_OK)
        st = post_add(post, &m);
    if (st != STATUS_OK)
        mail_destroy(&m);       // the post did not take the mail
    else
        printf("Mail added\n");
    return st;
}

// choice 2, 3, 5: ask an identifier and apply the action to the mail with it
status_t action_by_id(Post* post, int choice){
    char id[LINE_SIZE];
    int pos = 0;
    status_t st = read_line("Mail identifier (14 digits): ", id, LINE_SIZE);
    if (st == STATUS_OK && !is_digits(id, ID_LEN))
        st = STATUS_ERR_INPUT;
    if (st != STATUS_OK)
        return st;
    if (choice == 2)
        st = post_remove(post, id);
    else if (choice == 5)
        st = post_set_delivered(post, id);
    else if ((st = post_find(post, id, &pos)) == STATUS_OK)
        print_mail(&post->mails[pos]);
    if (st == STATUS_OK && choice != 3)
        printf(choice == 2 ? "Mail removed\n" : "Mail marked as delivered\n");
    return st;
}

status_t action_list(const Post* post, filter_t filter){
    long long now = 0;
    const Mail** list = NULL;
    int n = 0;
    status_t st = get_now_key(&now);
    if (st == STATUS_OK)
        st = post_collect(post, filter, now, &list, &n);
    if (st == STATUS_OK)
        print_mails(NULL, list, n);
    free(list);
    return st;
}

status_t run_choice(Post* post, int choice){
    switch (choice){
        case 1: return action_add(post);
        case 2: case 3: case 5: return action_by_id(post, choice);
        case 4: print_mails(post->mails, NULL, post->count); return STATUS_OK;
        case 6: return action_list(post, FILTER_DELIVERED);
        case 7: return action_list(post, FILTER_EXPIRED);
        case 8: return demo_strings();
        default: printf("Unknown menu item\n"); return STATUS_OK;
    }
}

int main(void){
    Post post = {NULL, NULL, 0, 0};
    printf("Enter the address of the post office\n");
    post.office = (Address*)malloc(sizeof(Address));
    status_t st = (post.office == NULL) ? STATUS_ERR_ALLOC : STATUS_ERR_INPUT;
    while (st == STATUS_ERR_INPUT){         // ask again until the address is valid
        st = read_address(post.office);
        if (st == STATUS_ERR_INPUT){
            print_status(st);
            address_destroy(post.office);
        }
    }

    while (st == STATUS_OK){
        char line[LINE_SIZE];
        int choice = -1;
        print_menu();
        st = read_line("Your choice: ", line, LINE_SIZE);
        if (st != STATUS_OK && st != STATUS_ERR_INPUT)
            break;
        if (st == STATUS_OK && strcmp(line, "0") == 0)
            break;
        if (st == STATUS_OK && parse_natural(line, &choice) != STATUS_OK)
            choice = -1;
        st = run_choice(&post, choice);
        // an error of one action is only reported, the dialog goes on
        if (st != STATUS_OK && st != STATUS_ERR_EOF && st != STATUS_ERR_ALLOC){
            print_status(st);
            st = STATUS_OK;
        }
    }
    if (st != STATUS_OK)
        print_status(st);
    post_destroy(&post);
    return 0;
}
