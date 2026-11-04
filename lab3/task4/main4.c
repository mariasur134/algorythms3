#include "functions4.h"

#define MENU_EXIT 0

// print how two strings relate to each other
void print_string_relations(const String* a, const String* b){
    printf("  \"%s\" vs \"%s\": order(len, lex) = %d, lex = %d, equal = %d\n",
           a->data, b->data, string_compare_order(a, b),
           string_compare_lex(a, b), string_equal(a, b));
}

// demonstration of part 1: every String function
status_t demo_strings(void){
    String a, b, c, d;
    String* heap = NULL;
    a.data = b.data = c.data = d.data = NULL;
    a.length = b.length = c.length = d.length = 0;

    status_t st = string_create(&a, "apple");
    if (st == STATUS_OK) st = string_create(&b, "pear");
    if (st == STATUS_OK) st = string_create(&c, "apricot");
    if (st == STATUS_OK) st = string_create(&d, "apple");
    if (st == STATUS_OK){
        printf("Created: a=\"%s\"(%d) b=\"%s\"(%d) c=\"%s\"(%d) d=\"%s\"(%d)\n",
               a.data, a.length, b.data, b.length, c.data, c.length, d.data, d.length);
        printf("Relations:\n");
        print_string_relations(&a, &b);   // shorter string is smaller
        print_string_relations(&a, &c);   // 5 < 7
        print_string_relations(&a, &d);   // equal
        print_string_relations(&b, &a);

        st = string_copy(&b, &c);         // copy into an existing String
        if (st == STATUS_OK)
            printf("After copy(b <- c): b=\"%s\"(%d)\n", b.data, b.length);
    }
    if (st == STATUS_OK){
        st = string_clone(&a, &heap);     // copy into a new String in the heap
        if (st == STATUS_OK)
            printf("Clone of a in the heap: \"%s\"(%d)\n", heap->data, heap->length);
    }
    if (st == STATUS_OK){
        st = string_concat(&a, &c);       // a = a + c
        if (st == STATUS_OK)
            printf("After concat(a += c): a=\"%s\"(%d), the clone is still \"%s\"\n",
                   a.data, a.length, heap->data);
    }
    if (st == STATUS_OK){
        st = string_concat(&a, &a);       // concatenation with itself
        if (st == STATUS_OK)
            printf("After concat(a += a): a=\"%s\"(%d)\n", a.data, a.length);
    }

    string_destroy(&a);
    string_destroy(&b);
    string_destroy(&c);
    string_destroy(&d);
    string_free(heap);
    return st;
}

void print_menu(void){
    printf("\n===== Post office =====\n");
    printf("1 - add mail\n");
    printf("2 - remove mail by identifier\n");
    printf("3 - find mail by identifier\n");
    printf("4 - show all mails (sorted by index, id)\n");
    printf("5 - mark mail as delivered\n");
    printf("6 - show delivered mails (by creation time)\n");
    printf("7 - show expired mails (by creation time)\n");
    printf("8 - demo of the String functions\n");
    printf("0 - exit\n");
}

// ask an identifier of an existing mail (any 14-digit string)
status_t ask_id_text(String* id){
    return ask_digits("Mail identifier (14 digits): ", ID_LEN, id);
}

status_t action_add(Post* post){
    Mail m;
    status_t st = ask_mail(post, &m);
    if (st == STATUS_OK)
        st = post_add(post, &m);
    if (st != STATUS_OK)
        mail_destroy(&m);       // post did not take the mail
    else
        printf("Mail added\n");
    return st;
}

status_t action_remove(Post* post){
    String id;
    status_t st = ask_id_text(&id);
    if (st != STATUS_OK)
        return st;
    st = post_remove(post, id.data);
    if (st == STATUS_OK)
        printf("Mail removed\n");
    string_destroy(&id);
    return st;
}

status_t action_find(const Post* post){
    String id;
    int pos;
    status_t st = ask_id_text(&id);
    if (st != STATUS_OK)
        return st;
    st = post_find(post, id.data, &pos);
    if (st == STATUS_OK)
        print_mail(&post->mails[pos]);
    string_destroy(&id);
    return st;
}

status_t action_deliver(Post* post){
    String id;
    status_t st = ask_id_text(&id);
    if (st != STATUS_OK)
        return st;
    st = post_set_delivered(post, id.data);
    if (st == STATUS_OK)
        printf("Mail marked as delivered\n");
    string_destroy(&id);
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
        print_mail_list(list, n);
    free(list);
    return st;
}

// choose and run one menu item; STATUS_ERR_EOF is returned only when input is over
status_t run_choice(Post* post, int choice){
    switch (choice){
        case 1: return action_add(post);
        case 2: return action_remove(post);
        case 3: return action_find(post);
        case 4: print_all_mails(post); return STATUS_OK;
        case 5: return action_deliver(post);
        case 6: return action_list(post, FILTER_DELIVERED);
        case 7: return action_list(post, FILTER_EXPIRED);
        case 8: return demo_strings();
        default:
            printf("Unknown menu item\n");
            return STATUS_OK;
    }
}

// create the post office: ask its address
status_t setup_post(Post* post){
    post->office = (Address*)malloc(sizeof(Address));
    if (post->office == NULL)
        return STATUS_ERR_ALLOC;
    address_init(post->office);
    printf("Enter the address of the post office\n");
    return ask_address(post->office);
}

int main(void){
    Post post;
    post_init(&post);

    status_t st = setup_post(&post);
    if (st == STATUS_OK){
        printf("Post office: ");
        print_address(post.office);
        printf("\n");
    }

    while (st == STATUS_OK){
        print_menu();
        char* line = NULL;
        st = ask_line("Your choice: ", &line);
        if (st != STATUS_OK)
            break;
        int choice = -1;
        if (strcmp(line, "0") == 0)
            choice = MENU_EXIT;
        else if (parse_natural(line, &choice) != STATUS_OK)
            choice = -1;
        free(line);
        if (choice == MENU_EXIT)
            break;
        st = run_choice(&post, choice);
        // errors of one action are only reported, the dialog goes on
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
