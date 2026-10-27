/* Calls the generated C API the way a consumer would, printing what comes back. */
#include "holder/holder.h"

#include <stdio.h>

holder_context* stub_context_new(void);
void stub_context_free(holder_context* context);

/* Prints a failed call's status and message, then frees the error. */
static void report(const char* what, int status, holder_error** error) {
    printf("%s: status %d, %s\n", what, status, holder_error_message(*error));
    holder_error_destroy(*error);
    *error = NULL;
}

int main(void) {
    holder_context* context = stub_context_new();
    holder_arena* arena = holder_arena_new();
    holder_error* error = NULL;
    const holder_card* card = NULL;

    holder_new_card root = {0};
    root.project_id = "p1";
    root.title = "Thesis";
    root.content = "Intro #draft";
    int status = holder_create_card(context, &root, arena, &card, &error);
    printf("create: status %d, %s %s, content '%s', tags %zu, first at %lld, updated_at == created_at: %d\n", status,
           card->card_id, card->title, card->content, card->tags.count,
           (long long)card->tag_occurrences.items[0].byte_start, card->updated_at == card->created_at);

    holder_new_card child = {0};
    child.project_id = "p1";
    child.title = "Chapter 1";
    child.parent_card_id = card->card_id;
    double sort_key = 2.5;
    child.sort_key = &sort_key;
    status = holder_create_card(context, &child, arena, &card, &error);
    printf("create child: status %d, parent %s, sort_key %.1f, content '%s'\n", status, card->parent_card_id,
           card->sort_key, card->content);

    holder_new_card untitled = {0};
    untitled.project_id = "p1";
    untitled.title = "";
    status = holder_create_card(context, &untitled, arena, &card, &error);
    report("create with an empty title", status, &error);
    holder_new_card orphan = {0};
    orphan.project_id = "p1";
    orphan.title = "Orphan";
    orphan.parent_card_id = "missing";
    status = holder_create_card(context, &orphan, arena, &card, &error);
    report("create under a missing parent", status, &error);

    status = holder_get_card(context, "c1", arena, &card, &error);
    printf("get: status %d, %s, deleted_at %s\n", status, card->title, card->deleted_at ? "set" : "NULL");
    status = holder_get_card(context, "c9", arena, &card, &error);
    report("get a missing card", status, &error);
    status = holder_get_card(context, "", arena, &card, &error);
    report("get with an empty ID", status, &error);

    const holder_card_list_item_page* page = NULL;
    holder_list_cards_parent all = {0};
    status = holder_list_cards(context, "p1", all, NULL, HOLDER_LIST_CARDS_ORDER_DEFAULT, NULL, NULL, false, false,
                               arena, &page, &error);
    printf("list all: status %d, %zu cards, content %s, child_count %s, next_cursor %s\n", status, page->items.count,
           page->items.items[0].content ? "present" : "absent", page->items.items[0].child_count ? "present" : "absent",
           page->next_cursor ? page->next_cursor : "NULL");

    holder_list_cards_parent roots = {HOLDER_LIST_CARDS_PARENT_ROOTS, NULL};
    status = holder_list_cards(context, "p1", roots, NULL, HOLDER_LIST_CARDS_ORDER_DEFAULT, NULL, NULL, true, true,
                               arena, &page, &error);
    printf("list roots with content and counts: status %d, %zu card, '%s', %d child\n", status, page->items.count,
           page->items.items[0].content, *page->items.items[0].child_count);

    holder_list_cards_parent children = {HOLDER_LIST_CARDS_PARENT_CARD_ID, "c1"};
    int32_t one = 1;
    status = holder_list_cards(context, "p1", children, NULL, HOLDER_LIST_CARDS_ORDER_UPDATED_DESC, &one, NULL,
                               false, false, arena, &page, &error);
    printf("list children of c1, one per page: status %d, %s\n", status, page->items.items[0].title);

    int32_t too_many = 9000;
    status = holder_list_cards(context, "p1", all, NULL, HOLDER_LIST_CARDS_ORDER_DEFAULT,
                                                     &too_many, NULL, false, false, arena, &page, &error);
    report("list with limit 9000", status, &error);
    status = holder_list_cards(context, "p1", all, NULL, (holder_list_cards_order)7,
                                                           NULL, NULL, false, false, arena, &page, &error);
    report("list with an unknown order", status, &error);
    holder_list_cards_parent bad_parent = {HOLDER_LIST_CARDS_PARENT_CARD_ID, NULL};
    status = holder_list_cards(context, "p1", bad_parent, NULL,
                                                             HOLDER_LIST_CARDS_ORDER_DEFAULT, NULL, NULL, false,
                                                             false, arena, &page, &error);
    report("list children of a NULL card", status, &error);
    status = holder_list_cards(context, "p9", all, NULL, HOLDER_LIST_CARDS_ORDER_DEFAULT,
                                                       NULL, NULL, false, false, arena, &page, &error);
    report("list a missing project", status, &error);

    /* A partial update keeps what it leaves out. */
    holder_card_update update = {0};
    update.title = "Thesis, revised";
    update.parent_card_id.change = HOLDER_SET;
    update.parent_card_id.value = "c2";
    status = holder_update_card(context, "c1", &update, arena, &card, &error);
    printf("update: status %d, %s, content '%s', parent %s\n", status, card->title, card->content,
           card->parent_card_id);
    holder_card_update clear = {0};
    clear.parent_card_id.change = HOLDER_CLEAR;
    status = holder_update_card(context, "c1", &clear, arena, &card, &error);
    printf("clear the parent: status %d, %s, parent %s\n", status, card->title,
           card->parent_card_id ? card->parent_card_id : "null");
    clear.parent_card_id.change = (holder_change)9;
    status = holder_update_card(context, "c1", &clear, arena, &card, &error);
    report("update with an unknown change", status, &error);

    holder_card_move move = {0};
    move.intent = HOLDER_CARD_MOVE_INTENT_INTO;
    move.target_card_id = "c1";
    status = holder_move_card(context, "c1", &move, arena, &card, &error);
    report("move a card into itself", status, &error);
    move.intent = (holder_card_move_intent)0;
    status = holder_move_card(context, "c1", &move, arena, &card, &error);
    report("move with no intent", status, &error);

    status = holder_trash_card(context, "c1", HOLDER_TRASH_CARD_CHILDREN_DEFAULT, arena, &card, &error);
    printf("trash: status %d, deleted at %lld\n", status, (long long)*card->deleted_at);
    status = holder_get_card(context, "c1", arena, &card, &error);
    report("get a trashed card", status, &error);
    status = holder_restore_card(context, "c1", arena, &card, &error);
    printf("restore: status %d, deleted_at %s\n", status, card->deleted_at ? "set" : "null");
    status = holder_restore_card(context, "c1", arena, &card, &error);
    report("restore a live card", status, &error);

    holder_arena_free(arena);
    stub_context_free(context);
    return 0;
}
