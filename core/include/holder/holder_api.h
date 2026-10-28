/*
 * Generated from contract/interface/app.py, contract/interface/cards.py, contract/interface/errors.py, contract/interface/links.py, contract/interface/milestones.py, contract/interface/projects.py, contract/interface/resources.py by holder_contract.
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
/* The request conflicts with what exists: what would be created already exists, such as a card with the same ID or file, or the thing to change is not in the state the request needs, such as restoring a card that is not in the trash, or removing a tag that a card would still carry in its prose. */
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

/* What is known about a resource: each name, such as identifier or author, with its values in order. A name is
 * not empty.
 */
typedef struct holder_resource_metadata_entry {
    const char* key;
    holder_string_list values;
} holder_resource_metadata_entry;

typedef struct holder_resource_metadata {
    const holder_resource_metadata_entry* items;
    size_t count;
} holder_resource_metadata;

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

/* What the link points at. */
typedef enum holder_new_link_target_type {
    HOLDER_NEW_LINK_TARGET_TYPE_CARD = 1,
    HOLDER_NEW_LINK_TARGET_TYPE_AI_MESSAGE = 2,
    HOLDER_NEW_LINK_TARGET_TYPE_AI_THREAD = 3,
    HOLDER_NEW_LINK_TARGET_TYPE_RESOURCE = 4,
    HOLDER_NEW_LINK_TARGET_TYPE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_new_link_target_type;

/* A link to make from a card. Its source is the card it is made from. */
typedef struct holder_new_link {
    holder_new_link_target_type target_type; /* What the link points at. */
    const char* target_id; /* The ID of what the link points at. */
    const char* target_project_id; /* The project of a card the link points at; omit for the card's own project. Only a card can be in another project, which need not be on this device. NULL when absent. */
    const char* kind; /* What the link means. NULL for the default, ref. */
    const char* label; /* A note on the link; null or omitted for none. May be NULL. */
    const int64_t* created_at; /* When the link was made, for imports; omit for now. A link that already exists keeps its own. NULL when absent. */
} holder_new_link;

/* One of a milestone's dates: a role and exactly one of a day or an instant. */
typedef struct holder_milestone_date {
    const char* role; /* What the date is for. */
    const char* date; /* The calendar day; present when the date is a day. NULL when absent. */
    const char* datetime; /* The instant; present when the date is an instant. NULL when absent. */
} holder_milestone_date;

typedef struct holder_milestone_date_list {
    const holder_milestone_date* items;
    size_t count;
} holder_milestone_date_list;

/* A milestone to put on a card. */
typedef struct holder_new_milestone {
    holder_milestone_date_list dates; /* The milestone's dates, in order; empty for a milestone without a date. */
    const char* kind; /* What kind of milestone it is; null or omitted for none. May be NULL. */
    const char* description; /* A note on the milestone; null or omitted for none. May be NULL. */
} holder_new_milestone;

/* A change to a milestone. Each field left out keeps its value. */
typedef struct holder_milestone_update {
    const holder_milestone_date_list* dates; /* The milestone's new dates, replacing all of them; empty for none. NULL when absent. */
    holder_string_change kind; /* What kind of milestone it is; null for none. Zero keeps it. */
    holder_string_change description; /* A note on the milestone; null for none. Zero keeps it. */
} holder_milestone_update;

/* A resource to make. */
typedef struct holder_new_resource {
    const char* resource_id; /* The resource's ID, for imports; omit for a new one. NULL when absent. */
    const char* type; /* What kind of thing the resource is. */
    const char* label; /* The resource's name. */
    const holder_resource_metadata* metadata; /* What is known about the resource; omit for nothing. A name without values is left out. NULL when absent. */
    const int64_t* created_at; /* When the resource was made, for imports; omit for now. NULL when absent. */
    const int64_t* updated_at; /* When the resource was last changed, for imports; omit for when it was made. NULL when absent. */
} holder_new_resource;

/* A change to a resource. Each field left out keeps its value. */
typedef struct holder_resource_update {
    const char* type; /* What kind of thing the resource is. NULL when absent. */
    const char* label; /* The resource's name. NULL when absent. */
    const holder_resource_metadata* metadata; /* Names whose values change: each name's values replace its old ones, an empty list removes the name, and names left out are kept. NULL when absent. */
    const int64_t* updated_at; /* When the resource was changed, for imports; omit for now. NULL when absent. */
} holder_resource_update;

/* How the project's files are stored. */
typedef enum holder_new_project_privacy_mode {
    HOLDER_NEW_PROJECT_PRIVACY_MODE_DEFAULT = 0, /* encrypted_git */
    HOLDER_NEW_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT = 1,
    HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN = 2,
    HOLDER_NEW_PROJECT_PRIVACY_MODE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_new_project_privacy_mode;

/* A project to make. */
typedef struct holder_new_project {
    const char* project_id; /* The project's ID, for imports; omit for a new one. NULL when absent. */
    const char* name; /* The project's name. */
    const char* root_path; /* The directory for the project's repository, an absolute path; omit for one under this device's projects directory, named for the project. NULL when absent. */
    const char* git_remote_url; /* The Git remote to sync with; null or omitted for none. One Git does not accept is an invalid argument. May be NULL. */
    const char* git_provider; /* Who hosts the Git remote; null or omitted when unknown. May be NULL. */
    holder_new_project_privacy_mode privacy_mode; /* How the project's files are stored. Zero for the default, encrypted_git. */
    const int64_t* created_at; /* When the project was made, for imports, after the epoch; omit for now. NULL when absent. */
    const int64_t* updated_at; /* When the project's settings last changed, for imports, after the epoch; omit for when it was made. NULL when absent. */
} holder_new_project;

/* A change to a project's settings. Each field left out keeps its value. */
typedef struct holder_project_update {
    const char* name; /* The project's name. NULL when absent. */
    holder_string_change git_remote_url; /* The Git remote to sync with; null for none. Zero keeps it. */
    holder_string_change git_provider; /* Who hosts the Git remote; null when unknown. Zero keeps it. */
} holder_project_update;

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

/* A tag and how many of a project's live cards carry it. */
typedef struct holder_tag_count {
    const char* tag; /* The tag, normalized: in lower case. */
    int32_t card_count; /* The live cards in the project that carry the tag: at least one, or the tag is not listed. */
} holder_tag_count;

typedef struct holder_tag_count_list {
    const holder_tag_count* items;
    size_t count;
} holder_tag_count_list;

/* What the link is from. */
typedef enum holder_link_source_type {
    HOLDER_LINK_SOURCE_TYPE_CARD = 1,
    HOLDER_LINK_SOURCE_TYPE_AI_MESSAGE = 2,
    HOLDER_LINK_SOURCE_TYPE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_link_source_type;

/* What the link points at. */
typedef enum holder_link_target_type {
    HOLDER_LINK_TARGET_TYPE_CARD = 1,
    HOLDER_LINK_TARGET_TYPE_AI_MESSAGE = 2,
    HOLDER_LINK_TARGET_TYPE_AI_THREAD = 3,
    HOLDER_LINK_TARGET_TYPE_RESOURCE = 4,
    HOLDER_LINK_TARGET_TYPE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_link_target_type;

/* A link from a card, or from an AI message, to something in a project. A link names both of its ends in full:
 * each end's project, type and ID.
 */
typedef struct holder_link {
    const char* source_project_id; /* The project of the card or AI message the link is from. */
    holder_link_source_type source_type; /* What the link is from. */
    const char* source_id; /* The ID of the card or AI message the link is from. */
    const char* target_project_id; /* The project of what the link points at: the source's own project, or, for a card, another project, which need not be on this device. A link grants no access to its target. */
    holder_link_target_type target_type; /* What the link points at. */
    const char* target_id; /* The ID of what the link points at. */
    const char* kind; /* What the link means. */
    const char* label; /* A note on the link; null when it has none. May be NULL. */
    int64_t created_at; /* When the link was made. */
} holder_link;

typedef struct holder_link_list {
    const holder_link* items;
    size_t count;
} holder_link_list;

/* A milestone on a card: its dates, which may repeat a role or be none, and what it is. */
typedef struct holder_milestone {
    const char* milestone_id; /* The milestone's ID. */
    const char* project_id; /* The project of the card the milestone is on. */
    const char* card_id; /* The card the milestone is on. */
    holder_milestone_date_list dates; /* The milestone's dates, in the order they were given. */
    const char* kind; /* What kind of milestone it is; null when it has none. May be NULL. */
    const char* description; /* A note on the milestone; null when it has none. May be NULL. */
    int64_t created_at; /* When the milestone was made. */
    int64_t updated_at; /* When the milestone was last changed. */
} holder_milestone;

typedef struct holder_milestone_list {
    const holder_milestone* items;
    size_t count;
} holder_milestone_list;

/* A milestone on a calendar, with the title of its card. */
typedef struct holder_calendar_milestone {
    const char* milestone_id; /* The milestone's ID. */
    const char* project_id; /* The project of the card the milestone is on. */
    const char* card_id; /* The card the milestone is on. */
    holder_milestone_date_list dates; /* The milestone's dates, in the order they were given. */
    const char* kind; /* What kind of milestone it is; null when it has none. May be NULL. */
    const char* description; /* A note on the milestone; null when it has none. May be NULL. */
    int64_t created_at; /* When the milestone was made. */
    int64_t updated_at; /* When the milestone was last changed. */
    const char* card_title; /* The title of the card the milestone is on. */
} holder_calendar_milestone;

typedef struct holder_calendar_milestone_list {
    const holder_calendar_milestone* items;
    size_t count;
} holder_calendar_milestone_list;

/* Everything about a card except its content. */
typedef struct holder_card_metadata {
    const char* card_id; /* The card's ID. */
    const char* project_id; /* The project the card belongs to. */
    const char* title; /* The card's title. */
    const char* rel_path; /* The card's file, relative to the project's root. */
    const char* parent_card_id; /* The card's parent; null for a root card. May be NULL. */
    double sort_key; /* The card's place among its siblings; lower comes first. */
    int64_t created_at; /* When the card was created. */
    int64_t updated_at; /* When the card was last changed. */
    const int64_t* deleted_at; /* When the card was moved to the trash; null when it is not in the trash. May be NULL. */
} holder_card_metadata;

typedef struct holder_card_metadata_list {
    const holder_card_metadata* items;
    size_t count;
} holder_card_metadata_list;

/* What happens in a project over a window of time: its milestones, and the cards made and changed. */
typedef struct holder_calendar {
    holder_calendar_milestone_list milestones; /* The milestones with a date in the window, as a day in its days or an instant in its instants, each once, by when they were made and then by ID. */
    holder_card_metadata_list created_cards; /* The live cards made in the window, by when they were made and then by ID. */
    holder_card_metadata_list updated_cards; /* The live cards last changed in the window, other than by being made, by when and then by ID. */
} holder_calendar;

/* How the copy is stored: as the file is, or encrypted for the project. */
typedef enum holder_asset_placement_encoding {
    HOLDER_ASSET_PLACEMENT_ENCODING_PLAIN = 1,
    HOLDER_ASSET_PLACEMENT_ENCODING_HOLDER_ASSET_V1 = 2,
    HOLDER_ASSET_PLACEMENT_ENCODING_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_asset_placement_encoding;

/* One stored copy of an asset. */
typedef struct holder_asset_placement {
    const char* placement_id; /* The copy's ID. */
    const char* location_id; /* The storage location that holds the copy. */
    holder_asset_placement_encoding encoding; /* How the copy is stored: as the file is, or encrypted for the project. */
    int64_t stored_byte_size; /* The size of the stored copy, in bytes. */
    int64_t created_at; /* When the copy was stored. */
} holder_asset_placement;

typedef struct holder_asset_placement_list {
    const holder_asset_placement* items;
    size_t count;
} holder_asset_placement_list;

/* A file that belongs to a resource. */
typedef struct holder_resource_asset {
    const char* asset_id; /* The asset's ID. */
    const char* original_filename; /* The file's name when it was imported. */
    const char* media_type; /* The file's media type, such as application/pdf. */
    int64_t byte_size; /* The file's size, in bytes. */
    const char* plaintext_sha256; /* The SHA-256 of the file's content, in lower-case hex. */
    holder_asset_placement_list placements; /* The stored copies of the file. */
    int64_t created_at; /* When the asset was imported. */
    int64_t updated_at; /* When the asset was last changed. */
} holder_resource_asset;

typedef struct holder_resource_asset_list {
    const holder_resource_asset* items;
    size_t count;
} holder_resource_asset_list;

/* A resource in a project, with its files. */
typedef struct holder_resource {
    const char* resource_id; /* The resource's ID. */
    const char* project_id; /* The project the resource belongs to. */
    const char* type; /* What kind of thing the resource is. */
    const char* label; /* The resource's name. */
    holder_resource_metadata metadata; /* What is known about the resource, by name. */
    holder_resource_asset_list assets; /* The resource's files. */
    int64_t created_at; /* When the resource was made. */
    int64_t updated_at; /* When the resource was last changed. */
} holder_resource;

typedef struct holder_resource_list {
    const holder_resource* items;
    size_t count;
} holder_resource_list;

/* How the project's files are stored. */
typedef enum holder_project_privacy_mode {
    HOLDER_PROJECT_PRIVACY_MODE_ENCRYPTED_GIT = 1,
    HOLDER_PROJECT_PRIVACY_MODE_PLAIN = 2,
    HOLDER_PROJECT_PRIVACY_MODE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_project_privacy_mode;

/* A project in a listing. */
typedef struct holder_project_list_item {
    const char* project_id; /* The project's ID. */
    const char* name; /* The project's name. */
    const char* root_path; /* The directory that holds the project's repository on this device. */
    const char* git_remote_url; /* The Git remote the project syncs with; null when it has none. May be NULL. */
    const char* git_provider; /* Who hosts the Git remote, such as github; null when unknown. May be NULL. */
    holder_project_privacy_mode privacy_mode; /* How the project's files are stored. */
    const char* project_key_id; /* The ID of an encrypted project's key; null for a plain project. May be NULL. */
    int64_t created_at; /* When the project was made. */
    int64_t updated_at; /* When the project's settings last changed. */
    const int32_t* card_count; /* The project's live cards; present when asked for. NULL when absent. */
    const int32_t* root_card_count; /* The project's live root cards; present when asked for. NULL when absent. */
} holder_project_list_item;

typedef struct holder_project_list_item_list {
    const holder_project_list_item* items;
    size_t count;
} holder_project_list_item_list;

/* A project on this device. */
typedef struct holder_project {
    const char* project_id; /* The project's ID. */
    const char* name; /* The project's name. */
    const char* root_path; /* The directory that holds the project's repository on this device. */
    const char* git_remote_url; /* The Git remote the project syncs with; null when it has none. May be NULL. */
    const char* git_provider; /* Who hosts the Git remote, such as github; null when unknown. May be NULL. */
    holder_project_privacy_mode privacy_mode; /* How the project's files are stored. */
    const char* project_key_id; /* The ID of an encrypted project's key; null for a plain project. May be NULL. */
    int64_t created_at; /* When the project was made. */
    int64_t updated_at; /* When the project's settings last changed. */
} holder_project;

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
 * tag: Only cards with this tag. May be NULL.
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

/* Tag a card.
 * The tag is added to the card's trailing tag line, which is made when the card has none. A card that already
 * carries the tag, anywhere in its content, is left as it is. A card in the trash or in another project is not
 * found.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_add_card_tag(holder_context* context, const char* project_id, const char* card_id, const char* tag, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

/* Take a tag off a card.
 * The tag is removed from the card's trailing tag line, and the line with it when it is left empty, so that the
 * card no longer carries it. A card that does not carry the tag is left as it is. Holder does not rewrite prose,
 * so a card that would still carry the tag in its prose is a Conflict, and is left as it is. A card in the trash
 * or in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_CONFLICT, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_remove_card_tag(holder_context* context, const char* project_id, const char* card_id, const char* tag, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

/* List a project's tags.
 * Each tag carried by a live card in the project, with how many carry it, by descending count and then by tag.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_list_project_tags(holder_context* context, const char* project_id, holder_arena* arena, const holder_tag_count_list** out_tag_count_list, holder_error** out_error);

/* List a card's links.
 * The links the card makes, newest first. A card in the trash or in another project is not found.
 * include_deleted: Include links to targets in the trash. A link to a card in another project is listed whether or not that card can be found here.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_list_card_links(holder_context* context, const char* project_id, const char* card_id, bool include_deleted, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error);

typedef enum holder_list_card_backlinks_scope {
    HOLDER_LIST_CARD_BACKLINKS_SCOPE_DEFAULT = 0, /* local */
    HOLDER_LIST_CARD_BACKLINKS_SCOPE_LOCAL = 1,
    HOLDER_LIST_CARD_BACKLINKS_SCOPE_EXTERNAL = 2,
    HOLDER_LIST_CARD_BACKLINKS_SCOPE_ALL_AVAILABLE = 3,
    HOLDER_LIST_CARD_BACKLINKS_SCOPE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_list_card_backlinks_scope;

/* List the links to a card.
 * Links from cards and AI messages, newest first. External links come only from projects on this device, so
 * they are never all there may be. A card in the trash or in another project is not found.
 * scope: local lists links from the card's own project; external, links from the other projects on this device; all_available, both.
 * include_deleted: Include links from cards and AI messages in the trash.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_list_card_backlinks(holder_context* context, const char* project_id, const char* card_id, holder_list_card_backlinks_scope scope, bool include_deleted, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error);

/* Link a card to something.
 * A link is one target, of one type in one project, with one kind. Making a link that already exists sets its
 * label and keeps when it was made. A target in the card's project must exist and not be in the trash; a card in
 * another project may be one this device does not have. A card in the trash or in another project is not found,
 * and neither is a target that does not exist or is in the trash.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_add_card_link(holder_context* context, const char* project_id, const char* card_id, const holder_new_link* link, holder_arena* arena, const holder_link** out_link, holder_error** out_error);

typedef enum holder_remove_card_link_target_type {
    HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_CARD = 1,
    HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_AI_MESSAGE = 2,
    HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_AI_THREAD = 3,
    HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_RESOURCE = 4,
    HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_remove_card_link_target_type;

/* Remove a link from a card.
 * Removes the one link to that target, of that type in that project, with that kind, and returns the card's
 * remaining links as list_card_links lists them by default. A card without that link is left as it is. A card in
 * the trash or in another project is not found.
 * kind: NULL for ref.
 * target_project_id: Omit for the card's own project. May be NULL.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_remove_card_link(holder_context* context, const char* project_id, const char* card_id, holder_remove_card_link_target_type target_type, const char* target_id, const char* kind, const char* target_project_id, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error);

/* List a card's milestones.
 * By when they were made, then by ID. A card in the trash or in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_list_card_milestones(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_milestone_list** out_milestone_list, holder_error** out_error);

/* Put a milestone on a card.
 * The milestone gets a new ID. Each date must be a real day or instant. A card in the trash or in another project
 * is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_add_card_milestone(holder_context* context, const char* project_id, const char* card_id, const holder_new_milestone* milestone, holder_arena* arena, const holder_milestone** out_milestone, holder_error** out_error);

/* Change a milestone.
 * Each field left out keeps its value, and an update that changes nothing writes nothing. Each date must be a real
 * day or instant. A card in the trash or in another project is not found, and neither is a milestone that is not
 * on the card.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_update_card_milestone(holder_context* context, const char* project_id, const char* card_id, const char* milestone_id, const holder_milestone_update* update, holder_arena* arena, const holder_milestone** out_milestone, holder_error** out_error);

/* Take a milestone off a card.
 * Returns the card's remaining milestones, as list_card_milestones lists them. A card without the milestone is
 * left as it is. A card in the trash or in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_remove_card_milestone(holder_context* context, const char* project_id, const char* card_id, const char* milestone_id, holder_arena* arena, const holder_milestone_list** out_milestone_list, holder_error** out_error);

/* List a project's milestones with a day in a range.
 * The range holds first_day, last_day and the days between; it is empty when first_day is after last_day. A
 * milestone is listed once, by its first day in the range and then by ID. Milestones on cards in the trash are
 * left out.
 * role: Only dates with this role; omit for any. May be NULL.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_list_milestones_by_day(holder_context* context, const char* project_id, const char* first_day, const char* last_day, const char* role, holder_arena* arena, const holder_milestone_list** out_milestone_list, holder_error** out_error);

/* List a project's milestones with an instant in a range.
 * The range holds first_instant, last_instant and the instants between; it is empty when first_instant is after
 * last_instant. A milestone is listed once, by its first instant in the range and then by ID. Milestones on cards
 * in the trash are left out.
 * role: Only dates with this role; omit for any. May be NULL.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_list_milestones_by_instant(holder_context* context, const char* project_id, int64_t first_instant, int64_t last_instant, const char* role, holder_arena* arena, const holder_milestone_list** out_milestone_list, holder_error** out_error);

/* What happens in a project over a window of time.
 * The caller gives the window twice, as instants and as the days they fall on where the caller is, because a day
 * has no time zone: a milestone is in the window when one of its days is in the days, or one of its instants in
 * the instants. Cards are made and changed at instants. Either range is empty when its start is after its end.
 * first_day: The day first_instant falls on where the caller is.
 * last_day: The day last_instant falls on where the caller is.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_get_calendar(holder_context* context, const char* project_id, int64_t first_instant, int64_t last_instant, const char* first_day, const char* last_day, holder_arena* arena, const holder_calendar** out_calendar, holder_error** out_error);

/* List a project's resources.
 * By when they last changed, latest first, then by ID.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_list_resources(holder_context* context, const char* project_id, holder_arena* arena, const holder_resource_list** out_resource_list, holder_error** out_error);

/* Fetch a resource.
 * A resource in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_get_resource(holder_context* context, const char* project_id, const char* resource_id, holder_arena* arena, const holder_resource** out_resource, holder_error** out_error);

/* Make a resource.
 * A project that does not exist is not found. Resource IDs are unique on this device, so an ID already in use,
 * in this project or another, is a Conflict.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_CONFLICT, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_create_resource(holder_context* context, const char* project_id, const holder_new_resource* resource, holder_arena* arena, const holder_resource** out_resource, holder_error** out_error);

/* Change a resource.
 * Each field left out keeps its value. A resource in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_update_resource(holder_context* context, const char* project_id, const char* resource_id, const holder_resource_update* update, holder_arena* arena, const holder_resource** out_resource, holder_error** out_error);

/* Delete a resource.
 * The resource goes, and so do the links to it from cards. Its files' stored copies are left where they are.
 * Returns the resource as it was. A resource in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_delete_resource(holder_context* context, const char* project_id, const char* resource_id, holder_arena* arena, const holder_resource** out_resource, holder_error** out_error);

/* List the links to a resource.
 * Links of every kind, attachments among them, from cards and AI messages in the resource's project, newest
 * first. A resource in another project is not found.
 * include_deleted: Include links from cards and AI messages in the trash.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_list_resource_backlinks(holder_context* context, const char* project_id, const char* resource_id, bool include_deleted, holder_arena* arena, const holder_link_list** out_link_list, holder_error** out_error);

/* List the resources attached to a card.
 * By when they last changed, latest first, then by ID. A card in the trash or in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_list_card_resources(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_resource_list** out_resource_list, holder_error** out_error);

/* Attach a resource to a card.
 * An attachment is the card's link of kind attachment to the resource. A card that has it already is left as it
 * is. Returns the card's attached resources, as list_card_resources lists them. A card in the trash or in another
 * project is not found, and neither is a resource that is not in the card's project.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_attach_card_resource(holder_context* context, const char* project_id, const char* card_id, const char* resource_id, holder_arena* arena, const holder_resource_list** out_resource_list, holder_error** out_error);

/* Detach a resource from a card.
 * The resource itself is kept. A card without the attachment is left as it is, and a resource that no longer
 * exists can still be detached. Returns the card's attached resources, as list_card_resources lists them. A card
 * in the trash or in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_detach_card_resource(holder_context* context, const char* project_id, const char* card_id, const char* resource_id, holder_arena* arena, const holder_resource_list** out_resource_list, holder_error** out_error);

typedef enum holder_list_projects_order {
    HOLDER_LIST_PROJECTS_ORDER_NONE = 0, /* null */
    HOLDER_LIST_PROJECTS_ORDER_UPDATED_AT_DESC = 1,
    HOLDER_LIST_PROJECTS_ORDER_UPDATED_AT_ASC = 2,
    HOLDER_LIST_PROJECTS_ORDER_CREATED_AT_DESC = 3,
    HOLDER_LIST_PROJECTS_ORDER_CREATED_AT_ASC = 4,
    HOLDER_LIST_PROJECTS_ORDER_NAME_DESC = 5,
    HOLDER_LIST_PROJECTS_ORDER_NAME_ASC = 6,
    HOLDER_LIST_PROJECTS_ORDER_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_list_projects_order;

/* List the projects on this device.
 * Filters combine. Projects that tie in the order keep the order they were stored in.
 * name: Only projects whose name contains this text. May be NULL.
 * updated_after: Only projects last changed at or after this instant. May be NULL.
 * updated_before: Only projects last changed at or before this instant. May be NULL.
 * order: Omit for Home first, then the latest changed. The zero value is null.
 * limit: NULL for 100.
 * offset: How many of the ordered projects to skip.
 * include_card_counts: Include each project's counts of live cards.
 */
int holder_list_projects(holder_context* context, const char* name, const int64_t* updated_after, const int64_t* updated_before, holder_list_projects_order order, const int32_t* limit, int32_t offset, bool include_card_counts, holder_arena* arena, const holder_project_list_item_list** out_project_list_item_list, holder_error** out_error);

/* Fetch a project.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_get_project(holder_context* context, const char* project_id, holder_arena* arena, const holder_project** out_project, holder_error** out_error);

/* Make a project.
 * Its repository is made, with its settings, and an encrypted project's key. An ID a project on this device has
 * is a Conflict. Making a project that was removed from this device makes it known here again.
 * Errors: HOLDER_ERROR_CONFLICT, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_create_project(holder_context* context, const holder_new_project* project, holder_arena* arena, const holder_project** out_project, holder_error** out_error);

/* Change a project's settings.
 * Each field left out keeps its value, and an update that changes nothing writes nothing. The settings are
 * committed to the project's repository; a Git remote is set there before it is stored, and one Git does not
 * accept is an invalid argument.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_update_project(holder_context* context, const char* project_id, const holder_project_update* update, holder_arena* arena, const holder_project** out_project, holder_error** out_error);

/* Remove a project from this device.
 * The project's files and keys are kept, and its removal is remembered, so that looking for projects does not
 * bring it back; making it again does. Returns the project as it was.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_delete_project(holder_context* context, const char* project_id, holder_arena* arena, const holder_project** out_project, holder_error** out_error);

#ifdef __cplusplus
}
#endif

#endif
