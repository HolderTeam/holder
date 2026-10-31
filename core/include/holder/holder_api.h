/*
 * Generated from contract/interface/app.py, contract/interface/ai.py, contract/interface/card_references.py, contract/interface/cards.py, contract/interface/daemon_routes.py, contract/interface/errors.py, contract/interface/git.py, contract/interface/history.py, contract/interface/links.py, contract/interface/locations.py, contract/interface/maintenance.py, contract/interface/milestones.py, contract/interface/projects.py, contract/interface/rebuild.py, contract/interface/recovery.py, contract/interface/resources.py, contract/interface/search.py, contract/interface/trash.py by holder_contract.
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
/* A card's ancestor chain is broken, cyclic or too deep to follow. */
#define HOLDER_ERROR_HIERARCHY_INVALID 106
/* The project's Git history cannot be read right now. */
#define HOLDER_ERROR_HISTORY_UNAVAILABLE 107
/* This device cannot use the encrypted project's key to read its history. */
#define HOLDER_ERROR_HISTORY_KEY_UNAVAILABLE 108
/* A commit ID prefix matches more than one revision; use a longer prefix or the full ID. */
#define HOLDER_ERROR_REVISION_AMBIGUOUS 109
/* The item still exists but is not in Trash, so permanent removal is refused. */
#define HOLDER_ERROR_NOT_TRASHED 110
/* The bounded History response would exceed 2 MiB. */
#define HOLDER_ERROR_HISTORY_RESPONSE_TOO_LARGE 111
/* The recovery token or its PIN is invalid, or the token belongs to another project. */
#define HOLDER_ERROR_RECOVERY_TOKEN_INVALID 112
/* A project's files cannot be read back into the database. */
#define HOLDER_ERROR_REBUILD_FAILED 113

holder_arena* holder_arena_new(void);
void holder_arena_free(holder_arena* arena);

typedef struct holder_string_list {
    const char* const* items;
    size_t count;
} holder_string_list;

/* Portable non-secret provider options. */
typedef struct holder_location_configuration_entry {
    const char* key;
    const char* values;
} holder_location_configuration_entry;

typedef struct holder_location_configuration {
    const holder_location_configuration_entry* items;
    size_t count;
} holder_location_configuration;

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

/* Storage provider kind. */
typedef enum holder_new_storage_location_provider {
    HOLDER_NEW_STORAGE_LOCATION_PROVIDER_LOCAL_DIRECTORY = 1,
    HOLDER_NEW_STORAGE_LOCATION_PROVIDER_S3_COMPATIBLE = 2,
    HOLDER_NEW_STORAGE_LOCATION_PROVIDER_GOOGLE_DRIVE = 3,
    HOLDER_NEW_STORAGE_LOCATION_PROVIDER_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_new_storage_location_provider;

/* A portable location declaration to create. */
typedef struct holder_new_storage_location {
    const char* location_id; /* Optional imported ID; Holder makes one when omitted or empty. NULL when absent. */
    const char* project_id; /* The owning project. */
    const char* name; /* Display name. */
    holder_new_storage_location_provider provider; /* Storage provider kind. */
    const holder_location_configuration* configuration; /* Portable non-secret provider configuration; empty when omitted. NULL when absent. */
    const int64_t* created_at; /* Imported creation time; current time when omitted. NULL when absent. */
    const int64_t* updated_at; /* Imported update time; creation time when omitted. NULL when absent. */
} holder_new_storage_location;

/* Changes to a portable location declaration. */
typedef struct holder_storage_location_update {
    const char* name; /* Replacement display name. NULL when absent. */
    const holder_location_configuration* configuration; /* Replacement configuration map. NULL when absent. */
    const int64_t* updated_at; /* Update time; current time when omitted. NULL when absent. */
} holder_storage_location_update;

/* Which cards may match. */
typedef enum holder_card_reference_resolve_request_scope {
    HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_LIVE = 1,
    HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_TRASHED = 2,
    HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_EITHER = 3,
    HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_card_reference_resolve_request_scope;

/* A reference to resolve among one project's cards. */
typedef struct holder_card_reference_resolve_request {
    const char* project_id; /* The project to search. */
    const char* reference; /* A full canonical UUID, eligible UUID prefix, or exact card title. */
    holder_card_reference_resolve_request_scope scope; /* Which cards may match. */
} holder_card_reference_resolve_request;

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

/* Ask for a repair plan, or apply it. */
typedef struct holder_hierarchy_repair_request {
    const bool* apply; /* Apply the repair. Without it, only report. NULL for the default, false. */
} holder_hierarchy_repair_request;

/* PIN used to wrap the project key. */
typedef struct holder_recovery_token_export_request {
    const char* pin; /* User-provided PIN used to wrap or unwrap the key. */
} holder_recovery_token_export_request;

/* PIN and serialized .hrk token to restore. */
typedef struct holder_recovery_token_import_request {
    const char* pin; /* User-provided PIN used to wrap or unwrap the key. */
    const char* recovery_token; /* Serialized .hrk token JSON. */
} holder_recovery_token_import_request;

/* The project whose files should be read into the database. */
typedef struct holder_rebuild_request {
    const char* project_id; /* The project to rebuild. */
} holder_rebuild_request;

/* How to push. */
typedef struct holder_push_options {
    const char* branch; /* The branch to push; omit for the repository's default. NULL when absent. */
    const bool* set_upstream; /* Whether the remote branch becomes the local branch's upstream. NULL for the default, true. */
} holder_push_options;

/* A remote to test. */
typedef struct holder_remote_test {
    const char* remote_url; /* The Git remote to test; omit for the project's own. NULL when absent. */
} holder_remote_test;

/* Storage provider kind. */
typedef enum holder_project_storage_location_provider {
    HOLDER_PROJECT_STORAGE_LOCATION_PROVIDER_LOCAL_DIRECTORY = 1,
    HOLDER_PROJECT_STORAGE_LOCATION_PROVIDER_S3_COMPATIBLE = 2,
    HOLDER_PROJECT_STORAGE_LOCATION_PROVIDER_GOOGLE_DRIVE = 3,
    HOLDER_PROJECT_STORAGE_LOCATION_PROVIDER_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_project_storage_location_provider;

/* A portable declaration with this device's binding status. */
typedef struct holder_project_storage_location {
    const char* location_id; /* The location's ID. */
    const char* project_id; /* The owning project. */
    const char* name; /* Display name. */
    holder_project_storage_location_provider provider; /* Storage provider kind. */
    holder_location_configuration configuration; /* Portable non-secret provider configuration. */
    bool bound; /* Whether this device has a private binding. */
    const char* binding_preview; /* Safe binding display text, or null; credentials are never returned. May be NULL. */
    int64_t created_at; /* When the declaration was created. */
    int64_t updated_at; /* When the declaration was last updated. */
} holder_project_storage_location;

/* The location affected by an action. */
typedef struct holder_location_delete_result {
    const char* location_id; /* The affected location ID. */
} holder_location_delete_result;

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

/* Whether this step names the project or a card. */
typedef enum holder_card_breadcrumb_kind {
    HOLDER_CARD_BREADCRUMB_KIND_PROJECT = 1,
    HOLDER_CARD_BREADCRUMB_KIND_CARD = 2,
    HOLDER_CARD_BREADCRUMB_KIND_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_card_breadcrumb_kind;

/* One project or live card in a card's path from its project root. */
typedef struct holder_card_breadcrumb {
    holder_card_breadcrumb_kind kind; /* Whether this step names the project or a card. */
    const char* id; /* The project or card ID. */
    const char* title; /* The project or card title. */
} holder_card_breadcrumb;

typedef struct holder_card_breadcrumb_list {
    const holder_card_breadcrumb* items;
    size_t count;
} holder_card_breadcrumb_list;

/* A card that matches a reference. */
typedef struct holder_card_reference_candidate {
    const char* card_id; /* The card's canonical full UUID. */
    const char* title; /* The card's title. */
    const int64_t* deleted_at; /* When the card entered Trash, or null while live. May be NULL. */
} holder_card_reference_candidate;

typedef struct holder_card_reference_candidate_list {
    const holder_card_reference_candidate* items;
    size_t count;
} holder_card_reference_candidate_list;

/* The resolution outcome. */
typedef enum holder_card_reference_resolve_data_status {
    HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_RESOLVED = 1,
    HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_AMBIGUOUS = 2,
    HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_NOT_FOUND = 3,
    HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_card_reference_resolve_data_status;

/* A resolved, ambiguous, or absent card reference. */
typedef struct holder_card_reference_resolve_data {
    holder_card_reference_resolve_data_status status; /* The resolution outcome. */
    const char* match_kind; /* How a reference matched: full_id, id_prefix or exact_title. Absent when no card matched. NULL when absent. */
    const holder_card_reference_candidate* card; /* The matching card for a resolved reference. NULL when absent. */
    const holder_card_reference_candidate_list* candidates; /* Two candidate cards for an ambiguous reference. NULL when absent. */
} holder_card_reference_resolve_data;

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

/* Whether an encrypted project's card files are safe to push. */
typedef struct holder_encryption_safety_check {
    bool ok; /* Whether every checked file passed. */
    int32_t checked_files; /* Card files examined. */
    int32_t unsafe_files; /* Card files that failed the safety check. */
    holder_string_list unsafe_paths; /* Project-relative paths of unsafe files. */
    const char* message; /* A short explanation of the result. */
} holder_encryption_safety_check;

/* How the project's files are stored. */
typedef enum holder_project_encryption_check_privacy_mode {
    HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_ENCRYPTED_GIT = 1,
    HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_PLAIN = 2,
    HOLDER_PROJECT_ENCRYPTION_CHECK_PRIVACY_MODE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_project_encryption_check_privacy_mode;

/* A project's privacy mode and the result of checking its files. */
typedef struct holder_project_encryption_check {
    const char* project_id; /* The project checked. */
    holder_project_encryption_check_privacy_mode privacy_mode; /* How the project's files are stored. */
    holder_encryption_safety_check check; /* The safety check's result. */
} holder_project_encryption_check;

/* A hidden branch's top card and its new parent. */
typedef struct holder_hierarchy_repair_move {
    const char* card_id; /* The hidden branch's top card. */
    const char* parent_card_id; /* Its new parent, or null for a project root. May be NULL. */
} holder_hierarchy_repair_move;

typedef struct holder_hierarchy_repair_move_list {
    const holder_hierarchy_repair_move* items;
    size_t count;
} holder_hierarchy_repair_move_list;

/* A plan to make hidden live cards reachable, and whether it was applied. */
typedef struct holder_hierarchy_repair {
    bool applied; /* Whether changes were written. */
    holder_hierarchy_repair_move_list moved; /* The hidden branches whose top cards move. */
    int32_t changed_card_count; /* Cards moved or re-spaced to make room. */
    holder_string_list cycle_card_ids; /* Cards in parent cycles; left for manual repair. */
} holder_hierarchy_repair;

/* A portable encrypted project key. */
typedef struct holder_project_recovery_token_export_result {
    const char* project_id; /* The project whose key was exported. */
    const char* key_id; /* The encrypted project's key ID. */
    const char* recovery_token; /* Serialized .hrk token JSON. */
} holder_project_recovery_token_export_result;

/* The project whose key was restored. */
typedef struct holder_project_recovery_token_import_result {
    const char* project_id; /* The project whose key was restored. */
} holder_project_recovery_token_import_result;

/* Outcome of the attempted pull. */
typedef enum holder_recovery_token_global_import_result_pull_status {
    HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_NOT_ATTEMPTED = 1,
    HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_SUCCEEDED = 2,
    HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_FAILED = 3,
    HOLDER_RECOVERY_TOKEN_GLOBAL_IMPORT_RESULT_PULL_STATUS_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_recovery_token_global_import_result_pull_status;

/* The result of restoring a token to this device. */
typedef struct holder_recovery_token_global_import_result {
    const char* project_id; /* The project whose key was restored. */
    bool project_created; /* Whether this device made the project from the token. */
    bool remote_hint_present; /* Whether the token names a Git remote. */
    bool remote_configured; /* Whether that remote was configured. */
    const char* remote_error; /* Why remote configuration failed, or null. May be NULL. */
    holder_recovery_token_global_import_result_pull_status pull_status; /* Outcome of the attempted pull. */
    const char* pull_error; /* Why pulling failed, or null. May be NULL. */
} holder_recovery_token_global_import_result;

/* The numbers of entities restored from a project's files. */
typedef struct holder_rebuild_result {
    const char* project_id; /* The project rebuilt. */
    int32_t cards; /* Cards restored. */
    int32_t ai_messages; /* AI messages restored. */
    int32_t ai_threads; /* AI threads restored. */
    int32_t links; /* Links restored. */
} holder_rebuild_result;

/* Completion of a full database search-index rebuild. */
typedef struct holder_reindex_result {
    const char* message; /* A short completion message. */
} holder_reindex_result;

/* What this device last recorded about a project's sync: its last pull and push, what is waiting to go, and
 * when it will retry.
 */
typedef struct holder_sync_state {
    const char* project_id; /* The project. */
    const int64_t* last_commit_at; /* When the project's repository last had a commit; null when unknown. May be NULL. */
    const int64_t* last_push_at; /* When the project was last pushed; null when never. May be NULL. */
    const int64_t* last_pull_at; /* When the project was last pulled; null when never. May be NULL. */
    int32_t uncommitted_changes_count; /* Files changed and not yet committed. */
    int32_t unpushed_commits_count; /* Commits not yet on the remote. */
    const char* last_push_status; /* How the last push ended, as a PushStatus; null when never. May be NULL. */
    const char* last_pull_status; /* How the last pull ended; null when never. May be NULL. */
    const char* last_sync_error; /* The last failure's message; null when the last sync succeeded. May be NULL. */
    const int64_t* last_sync_error_at; /* When the last failure happened; null when none is recorded. May be NULL. */
    int32_t retry_count; /* Failed pushes in a row. */
    const int64_t* next_retry_at; /* When a push will next be tried; null when none is waiting. May be NULL. */
    int32_t pull_retry_count; /* Failed pulls in a row. */
    const int64_t* next_pull_retry_at; /* When a pull will next be tried; null when none is waiting. May be NULL. */
    const int64_t* updated_at; /* When this state was last recorded; null when nothing has been. May be NULL. */
} holder_sync_state;

/* How the pull ended. */
typedef enum holder_pull_phase_status {
    HOLDER_PULL_PHASE_STATUS_NOT_ATTEMPTED = 1,
    HOLDER_PULL_PHASE_STATUS_SUCCEEDED = 2,
    HOLDER_PULL_PHASE_STATUS_REMOTE_UNSET = 3,
    HOLDER_PULL_PHASE_STATUS_FAILED = 4,
    HOLDER_PULL_PHASE_STATUS_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_pull_phase_status;

/* What a pull did. */
typedef struct holder_pull_phase {
    bool attempted; /* Whether a pull was tried. */
    holder_pull_phase_status status; /* How the pull ended. */
    int32_t conflicts_resolved; /* Cards changed both here and on the remote, merged by Holder's card rules. */
    int32_t hidden_cards_repaired; /* Live cards the pull left under a missing or trashed parent, made reachable again. */
    const char* error_message; /* Why the pull failed; null when it did not. May be NULL. */
} holder_pull_phase;

/* How the push ended. */
typedef enum holder_push_phase_status {
    HOLDER_PUSH_PHASE_STATUS_NOT_ATTEMPTED = 1,
    HOLDER_PUSH_PHASE_STATUS_PUSHED = 2,
    HOLDER_PUSH_PHASE_STATUS_UP_TO_DATE = 3,
    HOLDER_PUSH_PHASE_STATUS_AUTH_FAILED = 4,
    HOLDER_PUSH_PHASE_STATUS_NOT_FOUND = 5,
    HOLDER_PUSH_PHASE_STATUS_NETWORK_ERROR = 6,
    HOLDER_PUSH_PHASE_STATUS_NON_FAST_FORWARD = 7,
    HOLDER_PUSH_PHASE_STATUS_REMOTE_UNSET = 8,
    HOLDER_PUSH_PHASE_STATUS_UNKNOWN_ERROR = 9,
    HOLDER_PUSH_PHASE_STATUS_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_push_phase_status;

/* What a push did. */
typedef struct holder_push_phase {
    bool attempted; /* Whether a push was tried. */
    holder_push_phase_status status; /* How the push ended. */
    int32_t ahead_count; /* Commits here that the remote does not have. */
    int32_t behind_count; /* Commits on the remote that are not here. */
    const char* local_head_commit; /* The commit this device's branch is at; null when unknown. May be NULL. */
    const char* error_message; /* Why the push failed; null when it did not. May be NULL. */
} holder_push_phase;

/* Whether every phase tried succeeded. */
typedef enum holder_sync_result_status {
    HOLDER_SYNC_RESULT_STATUS_SUCCEEDED = 1,
    HOLDER_SYNC_RESULT_STATUS_FAILED = 2,
    HOLDER_SYNC_RESULT_STATUS_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_sync_result_status;

/* What a pull, a push or a sync did, as recorded in the project's sync state. A Git failure is an outcome
 * here, not an error.
 */
typedef struct holder_sync_result {
    const char* project_id; /* The project. */
    holder_sync_result_status status; /* Whether every phase tried succeeded. */
    const char* remote_url; /* The Git remote used; null when the project has none. May be NULL. */
    holder_pull_phase pull; /* The pull; not attempted for a push. */
    holder_push_phase push; /* The push; not attempted for a pull, or after a pull that failed. */
} holder_sync_result;

/* A scheduled sync's pull and push outcomes. Both phases are not attempted when neither is due or the
 * project has no remote. A due push may still run after a failed pull so local changes get another chance.
 */
typedef struct holder_due_sync_result {
    const char* project_id; /* The project. */
    const char* remote_url; /* The configured remote; null when there is none. May be NULL. */
    holder_pull_phase pull; /* The pull, or not attempted when it is not due. */
    holder_push_phase push; /* The push, or not attempted when it is not due. */
} holder_due_sync_result;

/* What testing the remote found; remote_unset when there was none to test. */
typedef enum holder_remote_test_result_status {
    HOLDER_REMOTE_TEST_RESULT_STATUS_REACHABLE = 1,
    HOLDER_REMOTE_TEST_RESULT_STATUS_AUTH_FAILED = 2,
    HOLDER_REMOTE_TEST_RESULT_STATUS_NOT_FOUND = 3,
    HOLDER_REMOTE_TEST_RESULT_STATUS_NETWORK_ERROR = 4,
    HOLDER_REMOTE_TEST_RESULT_STATUS_INVALID_REMOTE_URL = 5,
    HOLDER_REMOTE_TEST_RESULT_STATUS_REMOTE_UNSET = 6,
    HOLDER_REMOTE_TEST_RESULT_STATUS_UNKNOWN_ERROR = 7,
    HOLDER_REMOTE_TEST_RESULT_STATUS_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_remote_test_result_status;

/* Whether a Git remote can be reached with this device's credentials. */
typedef struct holder_remote_test_result {
    const char* project_id; /* The project. */
    holder_remote_test_result_status status; /* What testing the remote found; remote_unset when there was none to test. */
    const char* remote_url; /* The remote tested; null when there was none. May be NULL. */
    bool remote_has_head; /* Whether the remote has any commits. */
    const char* error_message; /* Why the remote could not be reached; null when it could. May be NULL. */
} holder_remote_test_result;

/* The author recorded on a Git commit. */
typedef struct holder_history_author {
    const char* name; /* The author's name. */
    const char* email; /* The author's email address. */
} holder_history_author;

/* A changed path with any display details found in its historical version. */
typedef struct holder_history_affected_path {
    const char* path; /* The path relative to the project root. */
    const char* title; /* A historical title, when available. NULL when absent. */
    const char* detail; /* A historical detail, when available. NULL when absent. */
} holder_history_affected_path;

typedef struct holder_history_affected_path_list {
    const holder_history_affected_path* items;
    size_t count;
} holder_history_affected_path_list;

/* What kind of object these paths belong to. */
typedef enum holder_history_affected_object_kind {
    HOLDER_HISTORY_AFFECTED_OBJECT_KIND_CARD = 1,
    HOLDER_HISTORY_AFFECTED_OBJECT_KIND_RESOURCE = 2,
    HOLDER_HISTORY_AFFECTED_OBJECT_KIND_LOCATION = 3,
    HOLDER_HISTORY_AFFECTED_OBJECT_KIND_AI_DATA = 4,
    HOLDER_HISTORY_AFFECTED_OBJECT_KIND_PROJECT_SETTINGS = 5,
    HOLDER_HISTORY_AFFECTED_OBJECT_KIND_UNKNOWN = 6,
    HOLDER_HISTORY_AFFECTED_OBJECT_KIND_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_history_affected_object_kind;

/* Paths changed together in one commit, grouped by Holder object kind. */
typedef struct holder_history_affected_object {
    holder_history_affected_object_kind kind; /* What kind of object these paths belong to. */
    holder_string_list paths; /* The changed paths, retained for existing clients. */
    holder_history_affected_path_list items; /* The changed paths with historical display details. */
} holder_history_affected_object;

typedef struct holder_history_affected_object_list {
    const holder_history_affected_object* items;
    size_t count;
} holder_history_affected_object_list;

/* One Git commit affecting a project. */
typedef struct holder_project_activity {
    const char* oid; /* The full commit object ID. */
    holder_string_list parent_oids; /* The commit's parents. */
    holder_history_author author; /* The commit's author. */
    int64_t authored_at; /* When the commit was authored. */
    int64_t committed_at; /* When the commit was committed. */
    const char* message; /* The commit message. */
    holder_history_affected_object_list affected_objects; /* Changed paths grouped by Holder object kind. */
    bool is_merge; /* Whether the commit has multiple parents. */
} holder_project_activity;

typedef struct holder_project_activity_list {
    const holder_project_activity* items;
    size_t count;
} holder_project_activity_list;

/* One bounded page of project activity, newest first. */
typedef struct holder_project_history_page {
    const char* head_oid; /* The project HEAD captured for this page; null for an empty repository. May be NULL. */
    holder_project_activity_list activities; /* The activities on this page, newest first. */
    const char* next_cursor; /* The cursor for another page; null at the end. May be NULL. */
    bool scan_limited; /* Whether History reached its bounded scan before the end. */
} holder_project_history_page;

/* One exact commit saved for a card. */
typedef struct holder_card_history_save {
    const char* oid; /* The full commit ID. */
    holder_string_list parent_oids; /* The commit's parents. */
    int64_t authored_at; /* When the commit was authored. */
    int64_t committed_at; /* When the commit was committed. */
    const char* message; /* The commit message. */
} holder_card_history_save;

typedef struct holder_card_history_save_list {
    const holder_card_history_save* items;
    size_t count;
} holder_card_history_save_list;

/* One displayed card change, possibly grouping adjacent autosaves. */
typedef struct holder_card_history_entry {
    const char* first_oid; /* The first commit in the entry. */
    const char* last_oid; /* The last commit in the entry. */
    holder_string_list parent_oids; /* Parents of the first commit. */
    holder_string_list visible_parent_oids; /* Direct parents represented on this page; omitted connections must not be inferred. */
    holder_history_author author; /* The author's name and email. */
    int64_t started_at; /* When the first commit was authored. */
    int64_t ended_at; /* When the last commit was committed. */
    const char* kind; /* The kind of change shown. */
    const char* summary; /* A short description of the change. */
    int32_t commit_count; /* The number of commits grouped in this entry. */
    bool is_merge; /* Whether this entry includes a merge commit. */
    holder_card_history_save_list saves; /* Exact saved commits in chronological order. */
} holder_card_history_entry;

typedef struct holder_card_history_entry_list {
    const holder_card_history_entry* items;
    size_t count;
} holder_card_history_entry_list;

/* One bounded page of saved card changes, newest first. */
typedef struct holder_card_history_page {
    const char* head_oid; /* The project HEAD captured for this page; null for an empty repository. May be NULL. */
    holder_card_history_entry_list entries; /* The card changes on this page. */
    const char* next_cursor; /* The cursor for another page; null at the end. May be NULL. */
    bool scan_limited; /* Whether History reached its bounded scan before the end. */
} holder_card_history_page;

/* A card's saved state at one commit, including an explicit absent state. */
typedef struct holder_card_history_version {
    bool exists; /* Whether the card existed in this snapshot. */
    const char* oid; /* The full commit ID. */
    const char* title; /* The card's saved title, empty when absent. */
    const char* body; /* The card's saved body, empty when absent. */
} holder_card_history_version;

/* One line in a card comparison. */
typedef struct holder_card_history_diff_line {
    const char* origin; /* Space for unchanged, plus for added, minus for removed. */
    const char* text; /* The line's text. */
    const int64_t* old_line; /* Its old line number; null for an addition. May be NULL. */
    const int64_t* new_line; /* Its new line number; null for a removal. May be NULL. */
} holder_card_history_diff_line;

typedef struct holder_card_history_diff_line_list {
    const holder_card_history_diff_line* items;
    size_t count;
} holder_card_history_diff_line_list;

/* A bounded line comparison of two saved card states. */
typedef struct holder_card_history_comparison {
    holder_card_history_version before; /* The earlier state. */
    holder_card_history_version after; /* The later state. */
    const char* summary; /* A short description of the difference. */
    holder_card_history_diff_line_list lines; /* The comparison's lines. */
    bool truncated; /* Whether the line comparison was cut short. */
} holder_card_history_comparison;

/* A card's state at a selected revision. */
typedef struct holder_card_history_snapshot {
    const char* card_id; /* The card being inspected. */
    holder_card_history_version snapshot; /* The state at the selected commit. */
} holder_card_history_snapshot;

/* A card restored from a saved state by creating a new commit. */
typedef struct holder_card_history_restore_result {
    const char* card_id; /* The restored card. */
    const char* restored_from_oid; /* The full commit ID of the selected historical snapshot. */
    const char* result_oid; /* The full commit ID created by restoring it. */
    const char* title; /* The card's title after restoration. */
    int64_t updated_at; /* When the restored card was updated. */
    const int64_t* deleted_at; /* When the restored card is in Trash; null when live. May be NULL. */
} holder_card_history_restore_result;

/* The kind of item. */
typedef enum holder_trash_item_type {
    HOLDER_TRASH_ITEM_TYPE_CARD = 1,
    HOLDER_TRASH_ITEM_TYPE_AI_MESSAGE = 2,
    HOLDER_TRASH_ITEM_TYPE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_trash_item_type;

/* A trashed card or AI message. */
typedef struct holder_trash_item {
    holder_trash_item_type type; /* The kind of item. */
    const char* project_id; /* The project that owns the item. */
    const char* card_id; /* The card ID, present for a card. May be NULL. */
    const char* message_id; /* The message ID, present for an AI message. May be NULL. */
    const char* thread_id; /* The AI thread containing a message. May be NULL. */
    const char* title; /* The card title, if this is a card. May be NULL. */
    const char* role; /* The AI message role, if this is a message. May be NULL. */
    int64_t deleted_at; /* When the item entered Trash. */
    const int64_t* created_at; /* When the AI message was created. May be NULL. */
    const char* rel_path; /* The card's relative file path. May be NULL. */
} holder_trash_item;

typedef struct holder_trash_item_list {
    const holder_trash_item* items;
    size_t count;
} holder_trash_item_list;

/* The project whose Trash was emptied. */
typedef struct holder_trash_empty_result {
    const char* project_id; /* The requested project, including when it had no trashed items. */
} holder_trash_empty_result;

/* The ID of a permanently removed item. */
typedef struct holder_trash_delete_result {
    const char* card_id; /* The removed card's ID, or null for a message. May be NULL. */
    const char* message_id; /* The removed AI message's ID, or null for a card. May be NULL. */
} holder_trash_delete_result;

/* A card matching a full-text query. */
typedef struct holder_search_card {
    const char* card_id; /* The matching card's ID. */
    const char* title; /* The card's current title. */
    int64_t updated_at; /* When the card was last updated. */
    int64_t created_at; /* When the card was created. */
    const char* snippet; /* A highlighted excerpt from the match. */
    double rank; /* The FTS relevance score; lower scores rank first. */
} holder_search_card;

typedef struct holder_search_card_list {
    const holder_search_card* items;
    size_t count;
} holder_search_card_list;

/* An AI message matching a full-text query. */
typedef struct holder_search_message {
    const char* message_id; /* The matching message's ID. */
    int64_t created_at; /* When the message was created. */
    const char* snippet; /* A highlighted excerpt from the match. */
    double rank; /* The FTS relevance score; lower scores rank first. */
} holder_search_message;

typedef struct holder_search_message_list {
    const holder_search_message* items;
    size_t count;
} holder_search_message_list;

/* Get a storage declaration and this device's binding status.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_get_storage_location(holder_context* context, const char* location_id, holder_arena* arena, const holder_project_storage_location** out_project_storage_location, holder_error** out_error);

/* Create a project's portable storage declaration.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_create_storage_location(holder_context* context, const holder_new_storage_location* request, holder_arena* arena, const holder_project_storage_location** out_project_storage_location, holder_error** out_error);

/* Change a storage declaration; configuration replaces the whole map.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_update_storage_location(holder_context* context, const char* location_id, const holder_storage_location_update* request, holder_arena* arena, const holder_project_storage_location** out_project_storage_location, holder_error** out_error);

/* Delete an unused declaration, its binding and its preferred setting on this device.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_CONFLICT.
 */
int holder_delete_storage_location(holder_context* context, const char* location_id, holder_arena* arena, const holder_location_delete_result** out_location_delete_result, holder_error** out_error);

/* Fetch a card.
 * A card in the trash or in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_get_card(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card** out_card, holder_error** out_error);

/* Fetch a live card's metadata without reading its content.
 * A card in the trash or in another project is not found.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_get_card_metadata(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card_metadata** out_card_metadata, holder_error** out_error);

/* Read a live card's path from its project root.
 * The project comes first, then live ancestor cards and the requested card. A missing or trashed
 * card is not found; a broken or cyclic ancestor chain is invalid.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_HIERARCHY_INVALID.
 */
int holder_get_card_breadcrumbs(holder_context* context, const char* project_id, const char* card_id, holder_arena* arena, const holder_card_breadcrumb_list** out_card_breadcrumb_list, holder_error** out_error);

/* Resolve a card reference within a project and deletion scope.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_resolve_card_reference(holder_context* context, const holder_card_reference_resolve_request* request, holder_arena* arena, const holder_card_reference_resolve_data** out_card_reference_resolve_data, holder_error** out_error);

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

/* Check whether an encrypted project's card files are safe to push.
 * A plain project needs no check and reports a successful result with zero files.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_check_project_encryption(holder_context* context, const char* project_id, holder_arena* arena, const holder_project_encryption_check** out_project_encryption_check, holder_error** out_error);

/* Find hidden live card branches, and optionally move them into the reachable tree.
 * An empty body plans without writing. Applying moves each hidden branch to its nearest reachable
 * ancestor in one commit; a second apply changes nothing. Parent cycles are reported and left alone.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_repair_project_hierarchy(holder_context* context, const char* project_id, const holder_hierarchy_repair_request* request, holder_arena* arena, const holder_hierarchy_repair** out_hierarchy_repair, holder_error** out_error);

/* Wrap an encrypted project's key in a portable token protected by a PIN.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_export_project_recovery_token(holder_context* context, const char* project_id, const holder_recovery_token_export_request* request, holder_arena* arena, const holder_project_recovery_token_export_result** out_project_recovery_token_export_result, holder_error** out_error);

/* Restore an encrypted project's key from a token for that project.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_RECOVERY_TOKEN_INVALID, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_import_project_recovery_token(holder_context* context, const char* project_id, const holder_recovery_token_import_request* request, holder_arena* arena, const holder_project_recovery_token_import_result** out_project_recovery_token_import_result, holder_error** out_error);

/* Restore a token, finding its project here or making one from the token's metadata.
 * If the token names a Git remote, configure it and attempt a pull. Remote and pull failures
 * are reported in the result. Returns 201 when the project was created, otherwise 200.
 * Errors: HOLDER_ERROR_RECOVERY_TOKEN_INVALID, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_import_recovery_token_global(holder_context* context, const holder_recovery_token_import_request* request, holder_arena* arena, const holder_recovery_token_global_import_result** out_recovery_token_global_import_result, holder_error** out_error);

/* Rebuild a project's database state from its files.
 * A missing project is NotFound. Files that cannot be read back give RebuildFailed.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_REBUILD_FAILED.
 */
int holder_rebuild_project(holder_context* context, const holder_rebuild_request* request, holder_arena* arena, const holder_rebuild_result** out_rebuild_result, holder_error** out_error);

/* Rebuild all search indexes from database state. */
int holder_reindex_search(holder_context* context, holder_arena* arena, const holder_reindex_result** out_reindex_result, holder_error** out_error);

/* What this device last recorded about a project's sync.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_get_project_sync_state(holder_context* context, const char* project_id, holder_arena* arena, const holder_sync_state** out_sync_state, holder_error** out_error);

/* Pull a project's checked-out branch from its Git remote.
 * Changes fast-forward, or merge card by card where both sides changed, and the project's index is updated
 * before the result returns. A Git failure is an outcome in the result, recorded in the sync state. Git work on
 * one project runs one at a time.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_pull_project(holder_context* context, const char* project_id, holder_arena* arena, const holder_sync_result** out_sync_result, holder_error** out_error);

/* Push a project to its Git remote.
 * An encrypted project is checked first, so no plaintext card leaves the device: one whose files are not safe
 * to push is DataCorrupted, and nothing is pushed. A Git failure is an outcome in the result, recorded in the
 * sync state.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_push_project(holder_context* context, const char* project_id, const holder_push_options* options, holder_arena* arena, const holder_sync_result** out_sync_result, holder_error** out_error);

/* Pull a project, then push it.
 * The push is tried only when the pull succeeds. Otherwise as pull_project and push_project.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_sync_project(holder_context* context, const char* project_id, const holder_push_options* options, holder_arena* arena, const holder_sync_result** out_sync_result, holder_error** out_error);

/* Try the project's pull and push phases only when their cadence and retry policy say they are due.
 * Zero uses the built-in intervals (20 minutes for push, 5 minutes for pull). A project without a remote and
 * a project with nothing due return both phases as not attempted without changing sync state. A due push can
 * run after a failed pull; this keeps scheduled pushes independent from the pull retry schedule.
 * Git failures are phase outcomes and are recorded in the project's sync state.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_sync_project_if_due(holder_context* context, const char* project_id, int32_t push_interval_seconds, int32_t pull_interval_seconds, holder_arena* arena, const holder_due_sync_result** out_due_sync_result, holder_error** out_error);

/* Test whether a Git remote can be reached, with this device's credentials.
 * Lists the remote's refs and nothing more: nothing is fetched, and neither the project nor its repository
 * changes.
 * Errors: HOLDER_ERROR_NOT_FOUND.
 */
int holder_test_project_remote(holder_context* context, const char* project_id, const holder_remote_test* remote, holder_arena* arena, const holder_remote_test_result** out_remote_test_result, holder_error** out_error);

typedef enum holder_list_project_history_kind {
    HOLDER_LIST_PROJECT_HISTORY_KIND_NONE = 0, /* null */
    HOLDER_LIST_PROJECT_HISTORY_KIND_CARD = 1,
    HOLDER_LIST_PROJECT_HISTORY_KIND_RESOURCE = 2,
    HOLDER_LIST_PROJECT_HISTORY_KIND_LOCATION = 3,
    HOLDER_LIST_PROJECT_HISTORY_KIND_AI_DATA = 4,
    HOLDER_LIST_PROJECT_HISTORY_KIND_PROJECT_SETTINGS = 5,
    HOLDER_LIST_PROJECT_HISTORY_KIND_UNKNOWN = 6,
    HOLDER_LIST_PROJECT_HISTORY_KIND_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_list_project_history_kind;

/* Return bounded, cursor-paginated Git activities for a project.
 * Each commit groups its changed paths by Holder object kind. Unrecognised paths remain visible as unknown.
 * A missing decryption key is HistoryKeyUnavailable; other history read failures are HistoryUnavailable.
 * limit: NULL for 50.
 * cursor: A full commit OID from next_cursor. May be NULL.
 * kind: Only activities affecting this object kind. The zero value is null.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_HISTORY_KEY_UNAVAILABLE, HOLDER_ERROR_HISTORY_RESPONSE_TOO_LARGE, HOLDER_ERROR_HISTORY_UNAVAILABLE.
 */
int holder_list_project_history(holder_context* context, const char* project_id, const int32_t* limit, const char* cursor, holder_list_project_history_kind kind, holder_arena* arena, const holder_project_history_page** out_project_history_page, holder_error** out_error);

/* List saved history for a card.
 * Returns commits reachable from the captured project HEAD that changed the card. Adjacent autosaves may be
 * grouped into one displayed entry.
 * limit: NULL for 50.
 * cursor: A full commit ID from next_cursor. May be NULL.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_HISTORY_KEY_UNAVAILABLE, HOLDER_ERROR_HISTORY_RESPONSE_TOO_LARGE, HOLDER_ERROR_HISTORY_UNAVAILABLE.
 */
int holder_list_card_history(holder_context* context, const char* project_id, const char* card_id, const int32_t* limit, const char* cursor, holder_arena* arena, const holder_card_history_page** out_card_history_page, holder_error** out_error);

typedef enum holder_compare_card_history_mode {
    HOLDER_COMPARE_CARD_HISTORY_MODE_DEFAULT = 0, /* since */
    HOLDER_COMPARE_CARD_HISTORY_MODE_SINCE = 1,
    HOLDER_COMPARE_CARD_HISTORY_MODE_CHANGE = 2,
    HOLDER_COMPARE_CARD_HISTORY_MODE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_compare_card_history_mode;

/* Compare saved card versions or one revision's change.
 * In since mode, compares the selected before and after snapshots. In change mode, compares the first parent
 * of to_oid with to_oid. A root commit has a previously nonexistent card as its before state; a merge uses only
 * its first parent.
 * to_oid: The ending revision, or the change to inspect.
 * from_oid: Required in since mode; ignored in change mode. May be NULL.
 * mode: Compare two snapshots or one introduced change.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_REVISION_AMBIGUOUS, HOLDER_ERROR_HISTORY_KEY_UNAVAILABLE, HOLDER_ERROR_HISTORY_RESPONSE_TOO_LARGE, HOLDER_ERROR_HISTORY_UNAVAILABLE.
 */
int holder_compare_card_history(holder_context* context, const char* project_id, const char* card_id, const char* to_oid, const char* from_oid, holder_compare_card_history_mode mode, holder_arena* arena, const holder_card_history_comparison** out_card_history_comparison, holder_error** out_error);

/* Read a card snapshot at a selected revision, including an explicit absent state.
 * oid: The revision containing the card snapshot.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_REVISION_AMBIGUOUS, HOLDER_ERROR_HISTORY_KEY_UNAVAILABLE, HOLDER_ERROR_HISTORY_RESPONSE_TOO_LARGE, HOLDER_ERROR_HISTORY_UNAVAILABLE.
 */
int holder_get_card_history_snapshot(holder_context* context, const char* project_id, const char* card_id, const char* oid, holder_arena* arena, const holder_card_history_snapshot** out_card_history_snapshot, holder_error** out_error);

/* Restore a card to a saved snapshot.
 * Restores the complete snapshot as a new commit, preserving the old history. Lifecycle rules still apply:
 * restoring a trashed snapshot promotes live children, and restoring a live snapshot places the card under a
 * reachable parent.
 * oid: The revision containing the snapshot to restore.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_REVISION_AMBIGUOUS, HOLDER_ERROR_HISTORY_KEY_UNAVAILABLE, HOLDER_ERROR_HISTORY_UNAVAILABLE.
 */
int holder_restore_card_history_snapshot(holder_context* context, const char* project_id, const char* card_id, const char* oid, holder_arena* arena, const holder_card_history_restore_result** out_card_history_restore_result, holder_error** out_error);

/* List a project's trashed cards and AI messages.
 * A project that is not registered has an empty Trash.
 * project_id: The project whose Trash is listed.
 * type: card, ai_message, or all; an unknown type lists nothing. May be NULL.
 */
int holder_list_trash(holder_context* context, const char* project_id, const char* type, holder_arena* arena, const holder_trash_item_list** out_trash_item_list, holder_error** out_error);

typedef enum holder_empty_trash_type {
    HOLDER_EMPTY_TRASH_TYPE_DEFAULT = 0, /* all */
    HOLDER_EMPTY_TRASH_TYPE_CARD = 1,
    HOLDER_EMPTY_TRASH_TYPE_AI_MESSAGE = 2,
    HOLDER_EMPTY_TRASH_TYPE_ALL = 3,
    HOLDER_EMPTY_TRASH_TYPE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_empty_trash_type;

/* Permanently remove a project's trashed items.
 * An empty or unregistered project succeeds without changes. Old Git revisions of cards remain available.
 * project_id: The project whose Trash is emptied.
 * type: Which item kinds to remove.
 * Errors: HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_empty_trash(holder_context* context, const char* project_id, holder_empty_trash_type type, holder_arena* arena, const holder_trash_empty_result** out_trash_empty_result, holder_error** out_error);

typedef enum holder_delete_trash_item_type {
    HOLDER_DELETE_TRASH_ITEM_TYPE_CARD = 1,
    HOLDER_DELETE_TRASH_ITEM_TYPE_AI_MESSAGE = 2,
    HOLDER_DELETE_TRASH_ITEM_TYPE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_delete_trash_item_type;

/* Permanently remove a trashed item by its global ID (legacy route).
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_NOT_TRASHED, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_delete_trash_item(holder_context* context, holder_delete_trash_item_type type, const char* id, holder_arena* arena, const holder_trash_delete_result** out_trash_delete_result, holder_error** out_error);

typedef enum holder_delete_project_trash_item_type {
    HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_CARD = 1,
    HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_AI_MESSAGE = 2,
    HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_MAX_ENUM = 0x7FFFFFFF /* Makes every int a valid value, so bad ones can be rejected. */
} holder_delete_project_trash_item_type;

/* Permanently remove a trashed item from the named project.
 * Old Git revisions of cards remain available.
 * Errors: HOLDER_ERROR_NOT_FOUND, HOLDER_ERROR_NOT_TRASHED, HOLDER_ERROR_KEYRING_UNAVAILABLE, HOLDER_ERROR_KEY_MISSING, HOLDER_ERROR_DATA_CORRUPTED.
 */
int holder_delete_project_trash_item(holder_context* context, const char* project_id, holder_delete_project_trash_item_type type, const char* id, holder_arena* arena, const holder_trash_delete_result** out_trash_delete_result, holder_error** out_error);

/* Search a project's cards, ordered by relevance, best first.
 * project_id: The project to search.
 * q: An SQLite FTS5 query for card titles and bodies.
 * limit: NULL for 20.
 */
int holder_search_cards(holder_context* context, const char* project_id, const char* q, const int32_t* limit, int32_t offset, holder_arena* arena, const holder_search_card_list** out_search_card_list, holder_error** out_error);

/* Search a project's AI messages, ordered by relevance, best first.
 * project_id: The project to search.
 * q: An SQLite FTS5 query for AI message content.
 * limit: NULL for 20.
 */
int holder_search_ai_messages(holder_context* context, const char* project_id, const char* q, const int32_t* limit, int32_t offset, holder_arena* arena, const holder_search_message_list** out_search_message_list, holder_error** out_error);

#ifdef __cplusplus
}
#endif

#endif
