/*
 * Generated from contract/interface/app.py, contract/interface/cards.py, contract/interface/errors.py by holder_contract.
 * Do not edit; change the contract, then run make -C contract update.
 *
 * The C declarations for the contract's operations, included by holder/holder.h, which declares holder_context, holder_error and the base status codes. Results live in the holder_arena the caller passes, until holder_arena_free.
 */
#ifndef HOLDER_API_H
#define HOLDER_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct holder_arena holder_arena;

/* The thing asked for does not exist, or is in the trash. */
#define HOLDER_ERROR_NOT_FOUND 100
/* The request conflicts with what exists: what would be created already exists, such as a card with the same ID or file, or the thing to change is not in the state the request needs, such as restoring a card that is not in the trash. */
#define HOLDER_ERROR_CONFLICT 101
/* The keyring that holds an encrypted project's key cannot be used: it is locked, or this device has none. */
#define HOLDER_ERROR_KEYRING_UNAVAILABLE 102
/* An encrypted project's key is not on this device; the project's recovery token restores it. */
#define HOLDER_ERROR_KEY_MISSING 103
/* A project's stored data is damaged or unsafe: an encrypted file does not decrypt, or holds plaintext. */
#define HOLDER_ERROR_DATA_CORRUPTED 104
/* A card cannot go where it was asked to: under itself or one of its descendants, beside a card that cannot be its sibling, or up a level from the top. */
#define HOLDER_ERROR_INVALID_MOVE 105

holder_arena* holder_arena_new(void);
void holder_arena_free(holder_arena* arena);

typedef struct holder_string_list {
    const char* const* items;
    size_t count;
} holder_string_list;

/* What a partial update does to a field. */
typedef enum holder_change {
    HOLDER_KEEP = 0, /* Leave it as it is. */
    HOLDER_SET = 1, /* Set it to value. */
    HOLDER_CLEAR = 2, /* Make it null. */
    HOLDER_CHANGE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_change;

typedef struct holder_string_change {
    holder_change change;
    const char* value; /* Read when change is HOLDER_SET. */
} holder_string_change;

/* A card to create. */
typedef struct holder_new_card {
    const char* title; /* The card's title. */
    const char* content; /* The card's body. NULL for the default, "". */
    const char* parent_card_id; /* The card's parent; null or omitted for a root card. May be NULL. */
    const double* sort_key; /* The card's place among its siblings; omit to let Holder choose. NULL when absent. */
    const int64_t* created_at; /* When the card was created, for imports; omit for now. NULL when absent. */
    const int64_t* updated_at; /* When the card was last changed, for imports; omit to use created_at. NULL when absent. */
} holder_new_card;

/* Changes to a card. Each field left out keeps its value. */
typedef struct holder_card_update {
    const char* title; /* The card's new title. NULL when absent. */
    const char* content; /* The card's new body. NULL when absent. */
    holder_string_change parent_card_id; /* The card's new parent; null makes it a root card. Zero keeps it. */
    const double* sort_key; /* The card's new place among its siblings. NULL when absent. */
    const int64_t* updated_at; /* When the card was changed, for imports; omit for now. NULL when absent. */
} holder_card_update;

/* into makes the card target_card_id's last child; before and after place it beside target_card_id, as its sibling; left and right swap it with its previous or next sibling; to_start and to_end make it the first or last of its siblings; up_level makes it its grandparent's last child. */
typedef enum holder_card_move_intent {
    HOLDER_CARD_MOVE_INTENT_INTO = 1,
    HOLDER_CARD_MOVE_INTENT_BEFORE = 2,
    HOLDER_CARD_MOVE_INTENT_AFTER = 3,
    HOLDER_CARD_MOVE_INTENT_LEFT = 4,
    HOLDER_CARD_MOVE_INTENT_RIGHT = 5,
    HOLDER_CARD_MOVE_INTENT_TO_START = 6,
    HOLDER_CARD_MOVE_INTENT_TO_END = 7,
    HOLDER_CARD_MOVE_INTENT_UP_LEVEL = 8,
    HOLDER_CARD_MOVE_INTENT_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_card_move_intent;

/* Where to move a card, relative to another card or among its siblings. */
typedef struct holder_card_move {
    holder_card_move_intent intent; /* into makes the card target_card_id's last child; before and after place it beside target_card_id, as its sibling; left and right swap it with its previous or next sibling; to_start and to_end make it the first or last of its siblings; up_level makes it its grandparent's last child. */
    const char* target_card_id; /* The card to move into, before or after; required for those intents, and ignored by the others. NULL when absent. */
    holder_string_change parent_card_id; /* For left, right, to_start and to_end: the parent among whose children to place it, null for the root cards; omit for its current parent. Ignored by the other intents. Zero keeps it. */
} holder_card_move;

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
 * A card in the trash or in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_get_card(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

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
 * tag: Only cards with this tag, matched without case and without the leading #. A tag starts with an ASCII letter and holds only ASCII letters, digits, _, / and -; a hex colour such as fff is not a tag. May be NULL.
 * order: updated_desc breaks ties by descending card ID; sort_key_asc follows sibling order.
 * limit: At most 1000 when include_content is true. NULL for 200.
 * cursor: The previous page's next_cursor. Keep the project, filters and order unchanged. May be NULL.
 * include_content: Include each card's body.
 * include_child_count: Include each card's count of live children.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_list_cards(holder_context* context, const char* project_id, holder_list_cards_parent parent, const char* tag, holder_list_cards_order order, const int32_t* limit, const char* cursor, bool include_content, bool include_child_count, holder_arena* arena, const holder_card_list_item_page** out_page, holder_error** out_error);

/* Create a card.
 * Fails with NotFound when the project or the parent does not exist.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_CONFLICT, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_create_card(holder_context* context, const char* project_id, const holder_new_card* card, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

/* Change a card.
 * Each field left out keeps its value, and an update that leaves every field out changes nothing. The title and
 * content are written first, then the parent and place, each as its own change in the project's history. A card
 * in the trash or in another project is not found, and neither is a new parent that does not exist or is in the
 * trash; a parent under the card itself is an InvalidMove.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_INVALID_MOVE, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_update_card(holder_context* context, const char* project_id, const char* card_id, const holder_card_update* update, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

typedef enum holder_trash_card_children {
    HOLDER_TRASH_CARD_CHILDREN_DEFAULT = 0, /* promote */
    HOLDER_TRASH_CARD_CHILDREN_PROMOTE = 1,
    HOLDER_TRASH_CARD_CHILDREN_TRASH_SUBTREE = 2,
    HOLDER_TRASH_CARD_CHILDREN_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_trash_card_children;

/* Move a card to the trash.
 * The card and every card that changes with it are written as one change in the project's history. A card already
 * in the trash, or in another project, is not found. Returns the card as it is in the trash.
 * children: promote moves the card's live children into its place among its siblings, keeping their own descendants; trash_subtree moves every live descendant to the trash with it.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_trash_card(holder_context* context, const char* project_id, const char* card_id, holder_trash_card_children children, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

/* Restore a card from the trash.
 * Restores this card only. It returns to its parent when that parent is live and reachable, otherwise to its
 * nearest live, reachable ancestor or the top level. Children promoted when it was trashed stay where they are;
 * descendants trashed with it stay in the trash, and come back under it when restored. A card in another
 * project is not found, and one that is not in the trash is a Conflict.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_CONFLICT, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_restore_card(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

/* Move a card in its project's hierarchy.
 * When the card's new neighbours leave no room between their places, nearby siblings are re-spaced in the same
 * change, keeping their order; other cards' sort_key values a caller holds may then be out of date. A card in the
 * trash or in another project is not found, and neither is a target or parent that does not exist or is in the
 * trash. A move that changes nothing writes nothing.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_INVALID_MOVE, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_move_card(holder_context* context, const char* project_id, const char* card_id, const holder_card_move* move, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

#ifdef __cplusplus
}
#endif

#endif
