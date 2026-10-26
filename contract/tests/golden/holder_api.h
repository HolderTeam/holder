/*
 * Generated from contract/interface/cards.py by holder_contract.
 * Do not edit; change the contract, then run make -C contract update.
 *
 * The C declarations for the contract's operations. Results live in the holder_arena the caller passes, until holder_arena_free.
 */
#ifndef HOLDER_API_H
#define HOLDER_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct holder_context holder_context;
typedef struct holder_error holder_error;
typedef struct holder_arena holder_arena;

#ifndef HOLDER_OK
#define HOLDER_OK 0
#define HOLDER_ERROR_INVALID_ARGUMENT 1
#define HOLDER_ERROR_RUNTIME 2
#define HOLDER_ERROR_ALLOCATION 3
#endif

/* The thing asked for does not exist, or is in the trash. */
#define HOLDER_ERROR_NOT_FOUND 100

holder_arena* holder_arena_new(void);
void holder_arena_free(holder_arena* arena);

typedef struct holder_string_list {
    const char* const* items;
    size_t count;
} holder_string_list;

/* A card to create. */
typedef struct holder_card_create {
    const char* project_id; /* The project to create it in. */
    const char* title; /* The card's title. */
    const char* content; /* The card's body. NULL for the default, "". */
    const char* parent_card_id; /* The card's parent; null or omitted for a root card. May be NULL. */
    const double* sort_key; /* The card's place among its siblings; omit to let Holder choose. NULL when absent. */
    const int64_t* created_at; /* When the card was created, for imports; omit for now. NULL when absent. */
    const int64_t* updated_at; /* When the card was last changed, for imports; omit to use created_at. NULL when absent. */
} holder_card_create;

/* A tag written in a card's content. */
typedef struct holder_tag_occurrence {
    const char* tag; /* The normalized tag. */
    int64_t byte_start; /* Inclusive byte offset of the leading # in the content. */
    int64_t byte_end; /* Exclusive byte offset after the tag's name. */
} holder_tag_occurrence;

typedef struct holder_tag_occurrence_list {
    const holder_tag_occurrence* items;
    size_t count;
} holder_tag_occurrence_list;

/* A card, with its content and tags. */
typedef struct holder_card {
    const char* card_id; /* The card's ID. */
    const char* project_id; /* The project the card belongs to. */
    const char* title; /* The card's title. */
    const char* rel_path; /* The card's file, relative to the project's root. */
    const char* parent_card_id; /* The card's parent; null for a root card. May be NULL. */
    double sort_key; /* The card's place among its siblings; lower comes first. */
    int64_t created_at; /* When the card was created. */
    int64_t updated_at; /* When the card was last changed. */
    const int64_t* deleted_at; /* When the card was moved to the trash; null when it is not in the trash. May be NULL. */
    const char* content; /* The card's body. */
    holder_string_list tags; /* The card's normalized tags. */
    holder_tag_occurrence_list tag_occurrences; /* Each tag written in the content, in order. */
} holder_card;

/* A card in a listing. */
typedef struct holder_card_list_item {
    const char* card_id; /* The card's ID. */
    const char* project_id; /* The project the card belongs to. */
    const char* title; /* The card's title. */
    const char* rel_path; /* The card's file, relative to the project's root. */
    const char* parent_card_id; /* The card's parent; null for a root card. May be NULL. */
    double sort_key; /* The card's place among its siblings; lower comes first. */
    int64_t created_at; /* When the card was created. */
    int64_t updated_at; /* When the card was last changed. */
    const int64_t* deleted_at; /* When the card was moved to the trash; null when it is not in the trash. May be NULL. */
    const int32_t* child_count; /* The card's live children; present when asked for. NULL when absent. */
    const char* content; /* The card's body; present when asked for. NULL when absent. */
} holder_card_list_item;

typedef struct holder_card_list_item_list {
    const holder_card_list_item* items;
    size_t count;
} holder_card_list_item_list;

/* One page of a listing. */
typedef struct holder_card_list_item_page {
    holder_card_list_item_list items; /* The page's items. */
    const char* next_cursor; /* Pass back to get the next page; null on the last page. May be NULL. */
} holder_card_list_item_page;

/* Fetch a card.
 * A card in the trash is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_get_card(holder_context* context, const char* card_id, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

typedef enum holder_list_cards_parent_kind {
    HOLDER_LIST_CARDS_PARENT_NONE = 0,
    HOLDER_LIST_CARDS_PARENT_ROOTS,
    HOLDER_LIST_CARDS_PARENT_CARD_ID,
    HOLDER_LIST_CARDS_PARENT_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_list_cards_parent_kind;

typedef struct holder_list_cards_parent {
    holder_list_cards_parent_kind kind;
    const char* card_id; /* Set when kind is HOLDER_LIST_CARDS_PARENT_CARD_ID. */
} holder_list_cards_parent;

typedef enum holder_list_cards_order {
    HOLDER_LIST_CARDS_ORDER_DEFAULT = 0, /* card_id_asc */
    HOLDER_LIST_CARDS_ORDER_CARD_ID_ASC = 1,
    HOLDER_LIST_CARDS_ORDER_UPDATED_DESC = 2,
    HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC = 3,
    HOLDER_LIST_CARDS_ORDER_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_list_cards_order;

/* Page through a project's live cards.
 * Filters combine. Each page reads current data; iterating is not a snapshot.
 * parent: Omit for every level; roots selects root cards; a card ID selects its children. The zero value is null.
 * tag: Only cards with this tag, matched without case and without the leading #. May be NULL.
 * order: updated_desc breaks ties by descending card ID; sort_key_asc follows sibling order.
 * limit: NULL for 200.
 * cursor: The previous page's next_cursor. Keep the project, filters and order unchanged. May be NULL.
 * include_content: Include each card's body.
 * include_child_count: Include each card's count of live children.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_list_cards(holder_context* context, const char* project_id, holder_list_cards_parent parent, const char* tag, holder_list_cards_order order, const int32_t* limit, const char* cursor, bool include_content, bool include_child_count, holder_arena* arena, const holder_card_list_item_page** out_page, holder_error** out_error);

/* Create a card.
 * Fails with NotFound when the project or the parent does not exist.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_create_card(holder_context* context, const holder_card_create* card, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

#ifdef __cplusplus
}
#endif

#endif
