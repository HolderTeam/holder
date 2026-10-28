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
    root.title = "Thesis";
    root.content = "Intro #draft";
    int status = holder_create_card(context, "p1", &root, arena, &card, &error);
    printf("create: status %d, %s %s, content '%s', tags %zu, first at %lld, updated_at == created_at: %d\n", status,
           card->card_id, card->title, card->content, card->tags.count,
           (long long)card->tag_occurrences.items[0].byte_start, card->updated_at == card->created_at);

    holder_new_card child = {0};
    child.title = "Chapter 1";
    child.parent_card_id = card->card_id;
    double sort_key = 2.5;
    child.sort_key = &sort_key;
    status = holder_create_card(context, "p1", &child, arena, &card, &error);
    printf("create child: status %d, parent %s, sort_key %.1f, content '%s'\n", status, card->parent_card_id,
           card->sort_key, card->content);

    holder_new_card untitled = {0};
    untitled.title = "";
    status = holder_create_card(context, "p1", &untitled, arena, &card, &error);
    report("create with an empty title", status, &error);
    holder_new_card orphan = {0};
    orphan.title = "Orphan";
    orphan.parent_card_id = "missing";
    status = holder_create_card(context, "p1", &orphan, arena, &card, &error);
    report("create under a missing parent", status, &error);

    status = holder_get_card(context, "p1", "c1", arena, &card, &error);
    printf("get: status %d, %s, deleted_at %s\n", status, card->title, card->deleted_at ? "set" : "NULL");
    status = holder_get_card(context, "p1", "c9", arena, &card, &error);
    report("get a missing card", status, &error);
    status = holder_get_card(context, "p2", "c1", arena, &card, &error);
    report("get a card in another project", status, &error);
    status = holder_get_card(context, "p1", "", arena, &card, &error);
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
    status = holder_update_card(context, "p1", "c1", &update, arena, &card, &error);
    printf("update: status %d, %s, content '%s', parent %s\n", status, card->title, card->content,
           card->parent_card_id);
    holder_card_update clear = {0};
    clear.parent_card_id.change = HOLDER_CLEAR;
    status = holder_update_card(context, "p1", "c1", &clear, arena, &card, &error);
    printf("clear the parent: status %d, %s, parent %s\n", status, card->title,
           card->parent_card_id ? card->parent_card_id : "null");
    clear.parent_card_id.change = (holder_change)9;
    status = holder_update_card(context, "p1", "c1", &clear, arena, &card, &error);
    report("update with an unknown change", status, &error);

    holder_card_move move = {0};
    move.intent = HOLDER_CARD_MOVE_INTENT_INTO;
    move.target_card_id = "c1";
    status = holder_move_card(context, "p1", "c1", &move, arena, &card, &error);
    report("move a card into itself", status, &error);
    move.intent = (holder_card_move_intent)0;
    status = holder_move_card(context, "p1", "c1", &move, arena, &card, &error);
    report("move with no intent", status, &error);

    status = holder_trash_card(context, "p1", "c1", HOLDER_TRASH_CARD_CHILDREN_DEFAULT, arena, &card, &error);
    printf("trash: status %d, deleted at %lld\n", status, (long long)*card->deleted_at);
    status = holder_get_card(context, "p1", "c1", arena, &card, &error);
    report("get a trashed card", status, &error);
    status = holder_restore_card(context, "p1", "c1", arena, &card, &error);
    printf("restore: status %d, deleted_at %s\n", status, card->deleted_at ? "set" : "null");
    status = holder_restore_card(context, "p1", "c1", arena, &card, &error);
    report("restore a live card", status, &error);

    status = holder_add_card_tag(context, "p1", "c1", "idea", arena, &card, &error);
    printf("add a tag: status %d, tags %zu, last '%s'\n", status, card->tags.count,
           card->tags.items[card->tags.count - 1]);
    status = holder_add_card_tag(context, "p1", "c1", "#idea", arena, &card, &error);
    report("add a tag with its #", status, &error);
    status = holder_remove_card_tag(context, "p1", "c1", "idea", arena, &card, &error);
    report("remove a tag in the prose", status, &error);
    const holder_tag_count_list* tags = NULL;
    status = holder_list_project_tags(context, "p1", arena, &tags, &error);
    printf("list tags: status %d, %zu tags, first %s on %d\n", status, tags->count, tags->items[0].tag,
           tags->items[0].card_count);
    status = holder_list_project_tags(context, "p2", arena, &tags, &error);
    report("list tags of a missing project", status, &error);

    holder_new_link link = {0};
    link.target_type = HOLDER_NEW_LINK_TARGET_TYPE_RESOURCE;
    link.target_id = "r1";
    link.kind = "cites";
    link.label = "a note";
    const holder_link* made = NULL;
    status = holder_add_card_link(context, "p1", "c1", &link, arena, &made, &error);
    printf("add a link: status %d, %s %s/%d/%s %s '%s'\n", status, made->source_id, made->target_project_id,
           (int)made->target_type, made->target_id, made->kind, made->label);
    link.target_type = (holder_new_link_target_type)9;
    status = holder_add_card_link(context, "p1", "c1", &link, arena, &made, &error);
    report("add a link to an unknown type", status, &error);
    const holder_link_list* links = NULL;
    status = holder_remove_card_link(context, "p1", "c1", HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_RESOURCE, "r1", "cites",
                                     NULL, arena, &links, &error);
    printf("remove a link: status %d, %zu left\n", status, links->count);

    holder_milestone_date dates[2] = {{0}};
    dates[0].role = "start";
    dates[0].date = "2026-10-16";
    dates[1].role = "due";
    dates[1].datetime = "2026-10-17T09:30:00+01:00";
    holder_new_milestone new_milestone = {0};
    new_milestone.dates.items = dates;
    new_milestone.dates.count = 2;
    new_milestone.kind = "release";
    const holder_milestone* milestone = NULL;
    status = holder_add_card_milestone(context, "p1", "c1", &new_milestone, arena, &milestone, &error);
    printf("add a milestone: status %d, %s %s %s=%s %s=%s %s\n", status, milestone->milestone_id,
           milestone->card_id, milestone->dates.items[0].role, milestone->dates.items[0].date,
           milestone->dates.items[1].role, milestone->dates.items[1].datetime, milestone->kind);
    const char* milestone_id = milestone->milestone_id;
    new_milestone.dates.items = NULL;
    status = holder_add_card_milestone(context, "p1", "c1", &new_milestone, arena, &milestone, &error);
    report("add a milestone whose dates are missing", status, &error);
    holder_milestone_date_list no_dates = {0};
    holder_milestone_update milestone_update = {0};
    milestone_update.dates = &no_dates;
    milestone_update.kind.change = HOLDER_CLEAR;
    status = holder_update_card_milestone(context, "p1", "c1", milestone_id, &milestone_update, arena,
                                          &milestone, &error);
    printf("clear a milestone's dates and kind: status %d, %zu dates, kind %s\n", status, milestone->dates.count,
           milestone->kind ? milestone->kind : "(null)");
    const holder_calendar* calendar = NULL;
    status = holder_get_calendar(context, "p1", 0, 1, "2026-10-01", "2026-10-31", arena, &calendar, &error);
    printf("calendar: status %d, %s on %s\n", status, calendar->milestones.items[0].milestone_id,
           calendar->milestones.items[0].card_title);
    const holder_milestone_list* milestones = NULL;
    status = holder_remove_card_milestone(context, "p1", "c1", milestone_id, arena, &milestones, &error);
    printf("remove a milestone: status %d, %zu left\n", status, milestones->count);

    const char* identifiers[] = {"https://example.com/a", "https://example.com/b"};
    holder_resource_metadata_entry entries[1] = {{0}};
    entries[0].key = "identifier";
    entries[0].values.items = identifiers;
    entries[0].values.count = 2;
    holder_resource_metadata metadata = {entries, 1};
    holder_new_resource new_resource = {0};
    new_resource.resource_id = "res-1";
    new_resource.type = "website";
    new_resource.label = "Example";
    new_resource.metadata = &metadata;
    const holder_resource* resource = NULL;
    status = holder_create_resource(context, "p1", &new_resource, arena, &resource, &error);
    printf("create a resource: status %d, %s %s %s=%s,%s\n", status, resource->resource_id, resource->type,
           resource->metadata.items[0].key, resource->metadata.items[0].values.items[0],
           resource->metadata.items[0].values.items[1]);
    holder_resource_metadata_entry repeated[2] = {{0}};
    repeated[0].key = "author";
    repeated[1].key = "author";
    holder_resource_metadata twice = {repeated, 2};
    holder_resource_update resource_update = {0};
    resource_update.metadata = &twice;
    status = holder_update_resource(context, "p1", "res-1", &resource_update, arena, &resource, &error);
    report("update a resource with a repeated name", status, &error);
    const holder_resource_list* attached = NULL;
    status = holder_attach_card_resource(context, "p1", "c1", "res-1", arena, &attached, &error);
    printf("attach a resource: status %d, %zu attached\n", status, attached->count);

    holder_new_project new_project = {0};
    new_project.name = "Garden";
    const holder_project* project = NULL;
    status = holder_create_project(context, &new_project, arena, &project, &error);
    printf("create a project: status %d, %s %s privacy %d\n", status, project->project_id, project->name,
           (int)project->privacy_mode);
    const char* project_id = project->project_id;
    new_project.privacy_mode = (holder_new_project_privacy_mode)9;
    status = holder_create_project(context, &new_project, arena, &project, &error);
    report("create a project with an unknown privacy mode", status, &error);
    const holder_project_list_item_list* projects = NULL;
    status = holder_list_projects(context, "Gard", NULL, NULL, HOLDER_LIST_PROJECTS_ORDER_NONE, NULL, 0, false, arena,
                                  &projects, &error);
    printf("list projects: status %d, %zu found\n", status, projects->count);
    status = holder_list_projects(context, NULL, NULL, NULL, (holder_list_projects_order)99, NULL, 0, false, arena,
                                  &projects, &error);
    report("list projects in an unknown order", status, &error);

    holder_push_options push_options = {0};
    const holder_sync_result* synced = NULL;
    status = holder_sync_project(context, project_id, &push_options, arena, &synced, &error);
    printf("sync a project with no remote: status %d, result %d, pull %d, push %d\n", status, (int)synced->status,
           (int)synced->pull.status, (int)synced->push.status);

    const holder_project_history_page* history = NULL;
    status = holder_list_project_history(context, project_id, NULL, NULL, HOLDER_LIST_PROJECT_HISTORY_KIND_NONE,
                                         arena, &history, &error);
    printf("list project history: status %d, %zu activities\n", status, history->activities.count);

    const holder_card_history_page* card_history = NULL;
    status = holder_list_card_history(context, project_id, "card-one", NULL, NULL, arena,
                                      &card_history, &error);
    printf("list card history: status %d, %zu entries\n", status, card_history->entries.count);

    const holder_card_history_comparison* comparison = NULL;
    status = holder_compare_card_history(context, project_id, "card-one", "deadbeef", NULL,
                                         HOLDER_COMPARE_CARD_HISTORY_MODE_CHANGE, arena, &comparison, &error);
    printf("compare card history: status %d, after %s\n", status, comparison->after.oid);

    const holder_card_history_snapshot* snapshot = NULL;
    status = holder_get_card_history_snapshot(context, project_id, "card-one", "deadbeef", arena,
                                              &snapshot, &error);
    printf("card history snapshot: status %d, exists %d\n", status, snapshot->snapshot.exists);

    const holder_card_history_restore_result* restored = NULL;
    status = holder_restore_card_history_snapshot(context, project_id, "card-one", "deadbeef", arena,
                                                  &restored, &error);
    printf("restore card history: status %d, title %s\n", status, restored->title);

    const holder_trash_item_list* trash = NULL;
    status = holder_list_trash(context, project_id, NULL, arena, &trash, &error);
    printf("list trash: status %d, %zu items\n", status, trash->count);

    const holder_trash_empty_result* emptied = NULL;
    status = holder_empty_trash(context, project_id, HOLDER_EMPTY_TRASH_TYPE_ALL, arena, &emptied, &error);
    printf("empty trash: status %d, project %s\n", status, emptied->project_id);

    const holder_trash_delete_result* removed = NULL;
    status = holder_delete_trash_item(context, "card", "card-one", arena, &removed, &error);
    printf("delete trash item: status %d, card %s\n", status, removed->card_id);

    status = holder_delete_project_trash_item(context, project_id, "ai_message", "message-one", arena,
                                             &removed, &error);
    printf("delete project trash item: status %d, message %s\n", status, removed->message_id);

    holder_arena_free(arena);
    stub_context_free(context);
    return 0;
}
