#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <holder/holder.h>

#include <string.h>

#ifndef PyCFunction_CAST
#define PyCFunction_CAST(function) ((PyCFunction)(void (*)(void))(function))
#endif

typedef struct {
    PyObject *holder_error;
} holder_module_state;

typedef struct {
    PyObject_HEAD
    holder_context *context;
} ContextObject;

static holder_module_state *
get_module_state_from_context(ContextObject *self)
{
    PyObject *module = PyType_GetModule(Py_TYPE(self));
    if (module == NULL) {
        return NULL;
    }
    return PyModule_GetState(module);
}

static int
raise_holder_error(ContextObject *self, int result, holder_error *error)
{
    const char *message = error != NULL ? holder_error_message(error) : "libholder operation failed";

    if (result == HOLDER_ERROR_ALLOCATION) {
        holder_error_destroy(error);
        PyErr_NoMemory();
        return -1;
    }

    PyObject *exception = PyExc_RuntimeError;
    if (result == HOLDER_ERROR_INVALID_ARGUMENT) {
        exception = PyExc_ValueError;
    } else {
        holder_module_state *state = get_module_state_from_context(self);
        if (state == NULL) {
            holder_error_destroy(error);
            return -1;
        }
        exception = state->holder_error;
    }

    PyErr_SetString(exception, message);
    holder_error_destroy(error);
    return -1;
}

static int
ensure_context_open(ContextObject *self)
{
    if (self->context == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Holder context is closed");
        return 0;
    }
    return 1;
}

static int
put_text(PyObject *record, const char *name, const char *value)
{
    PyObject *text = value == NULL ? Py_NewRef(Py_None) : PyUnicode_FromString(value);
    if (text == NULL) return -1;
    const int result = PyDict_SetItemString(record, name, text);
    Py_DECREF(text);
    return result;
}

static int
put_epoch(PyObject *record, const char *name, int64_t value)
{
    PyObject *number = PyLong_FromLongLong(value);
    if (number == NULL) return -1;
    const int result = PyDict_SetItemString(record, name, number);
    Py_DECREF(number);
    return result;
}

static int
put_optional_epoch(PyObject *record, const char *name, const int64_t *value)
{
    if (value == NULL) return put_text(record, name, NULL);
    return put_epoch(record, name, *value);
}

static int
put_double(PyObject *record, const char *name, double value)
{
    PyObject *number = PyFloat_FromDouble(value);
    if (number == NULL) return -1;
    const int result = PyDict_SetItemString(record, name, number);
    Py_DECREF(number);
    return result;
}

static PyObject *
card_record(const char *card_id, const char *project_id, const char *title,
            const char *rel_path, const char *parent_card_id, double sort_key,
            int64_t created_at, int64_t updated_at, const int64_t *deleted_at,
            const char *content)
{
    PyObject *record = PyDict_New();
    if (record == NULL) return NULL;
    if (put_text(record, "card_id", card_id) < 0 ||
        put_text(record, "project_id", project_id) < 0 ||
        put_text(record, "title", title) < 0 ||
        put_text(record, "rel_path", rel_path) < 0 ||
        put_text(record, "parent_card_id", parent_card_id) < 0 ||
        put_double(record, "sort_key", sort_key) < 0 ||
        put_epoch(record, "created_at", created_at) < 0 ||
        put_epoch(record, "updated_at", updated_at) < 0 ||
        put_optional_epoch(record, "deleted_at", deleted_at) < 0 ||
        (content != NULL && put_text(record, "content", content) < 0)) {
        Py_DECREF(record);
        return NULL;
    }
    return record;
}

static PyObject *
reference_candidate_record(const holder_card_reference_candidate *candidate)
{
    PyObject *record = PyDict_New();
    if (record == NULL) return NULL;
    if (put_text(record, "card_id", candidate->card_id) < 0 ||
        put_text(record, "title", candidate->title) < 0 ||
        put_optional_epoch(record, "deleted_at", candidate->deleted_at) < 0) {
        Py_DECREF(record);
        return NULL;
    }
    return record;
}

static PyObject *
project_record(const char *project_id, const char *name, const char *root_path,
               const char *git_remote_url, const char *git_provider,
               holder_project_privacy_mode privacy_mode, const char *project_key_id,
               int64_t created_at, int64_t updated_at)
{
    PyObject *record = PyDict_New();
    if (record == NULL) return NULL;
    if (put_text(record, "project_id", project_id) < 0 ||
        put_text(record, "name", name) < 0 ||
        put_text(record, "root_path", root_path) < 0 ||
        put_text(record, "git_remote_url", git_remote_url) < 0 ||
        put_text(record, "git_provider", git_provider) < 0 ||
        put_text(record, "privacy_mode", privacy_mode == HOLDER_PROJECT_PRIVACY_MODE_PLAIN
                                      ? "plain" : "encrypted_git") < 0 ||
        put_text(record, "project_key_id", project_key_id) < 0 ||
        put_epoch(record, "created_at", created_at) < 0 ||
        put_epoch(record, "updated_at", updated_at) < 0) {
        Py_DECREF(record);
        return NULL;
    }
    return record;
}

static int
Context_init(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *data_dir = NULL;
    const char *schema_sql = NULL;
    static char *keywords[] = {"data_dir", "schema_sql", NULL};

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "ss:Context",
            keywords,
            &data_dir,
            &schema_sql)) {
        return -1;
    }

    holder_context *new_context = NULL;
    holder_error *error = NULL;
    const int result = holder_context_open(data_dir, schema_sql, &new_context, &error);
    if (result != HOLDER_OK) {
        return raise_holder_error(self, result, error);
    }
    holder_error_destroy(error);

    holder_context *old_context = self->context;
    self->context = new_context;
    holder_context_destroy(old_context);
    return 0;
}

static void
Context_dealloc(ContextObject *self)
{
    holder_context_destroy(self->context);
    self->context = NULL;
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
Context_close(ContextObject *self, PyObject *Py_UNUSED(ignored))
{
    holder_context *context = self->context;
    self->context = NULL;
    holder_context_destroy(context);
    Py_RETURN_NONE;
}

static PyObject *
Context_enter(ContextObject *self, PyObject *Py_UNUSED(ignored))
{
    if (!ensure_context_open(self)) {
        return NULL;
    }
    return Py_NewRef(self);
}

static PyObject *
Context_exit(ContextObject *self, PyObject *Py_UNUSED(args))
{
    return Context_close(self, NULL);
}

static PyObject *
Context_create_project(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *name = NULL;
    static char *keywords[] = {"name", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:create_project", keywords, &name)) {
        return NULL;
    }
    if (!ensure_context_open(self)) {
        return NULL;
    }

    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    holder_error *error = NULL;
    holder_new_project request = {0};
    request.name = name;
    request.privacy_mode = HOLDER_NEW_PROJECT_PRIVACY_MODE_PLAIN;
    const holder_project *project = NULL;
    const int result = holder_create_project(self->context, &request, arena, &project, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *record = project_record(project->project_id, project->name, project->root_path,
        project->git_remote_url, project->git_provider, project->privacy_mode,
        project->project_key_id, project->created_at, project->updated_at);
    holder_arena_free(arena);
    return record;
}

static PyObject *
Context_list_projects(ContextObject *self, PyObject *Py_UNUSED(ignored))
{
    if (!ensure_context_open(self)) {
        return NULL;
    }

    PyObject *records = PyList_New(0);
    if (records == NULL) return NULL;
    for (int32_t offset = 0; ; ) {
        holder_arena *arena = holder_arena_new();
        if (arena == NULL) {
            Py_DECREF(records);
            return PyErr_NoMemory();
        }
        holder_error *error = NULL;
        const holder_project_list_item_list *page = NULL;
        const int result = holder_list_projects(self->context, NULL, NULL, NULL,
            HOLDER_LIST_PROJECTS_ORDER_NONE, NULL, offset, false, arena, &page, &error);
        if (result != HOLDER_OK) {
            raise_holder_error(self, result, error);
            holder_arena_free(arena);
            Py_DECREF(records);
            return NULL;
        }
        holder_error_destroy(error);
        for (size_t i = 0; i < page->count; ++i) {
            const holder_project_list_item *project = &page->items[i];
            PyObject *record = project_record(project->project_id, project->name,
                project->root_path, project->git_remote_url, project->git_provider,
                project->privacy_mode, project->project_key_id, project->created_at,
                project->updated_at);
            if (record == NULL || PyList_Append(records, record) < 0) {
                Py_XDECREF(record);
                holder_arena_free(arena);
                Py_DECREF(records);
                return NULL;
            }
            Py_DECREF(record);
        }
        const size_t count = page->count;
        holder_arena_free(arena);
        if (count < 100) return records;
        if (offset > INT32_MAX - 100) {
            Py_DECREF(records);
            PyErr_SetString(PyExc_OverflowError, "too many projects to list");
            return NULL;
        }
        offset += 100;
    }
}

static PyObject *
Context_create_card(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *title = NULL;
    const char *content = NULL;
    const char *parent_card_id = NULL;
    static char *keywords[] = {
        "project_id", "title", "content", "parent_card_id", NULL
    };

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "ss|zz:create_card",
            keywords,
            &project_id,
            &title,
            &content,
            &parent_card_id)) {
        return NULL;
    }
    if (!ensure_context_open(self)) {
        return NULL;
    }

    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    holder_new_card request = {0};
    request.title = title;
    request.content = content;
    request.parent_card_id = parent_card_id;
    const holder_card *card = NULL;
    holder_error *error = NULL;
    const int result = holder_create_card(self->context, project_id, &request,
        arena, &card, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *record = card_record(card->card_id, card->project_id, card->title,
        card->rel_path, card->parent_card_id, card->sort_key, card->created_at,
        card->updated_at, card->deleted_at, NULL);
    holder_arena_free(arena);
    return record;
}

/* Milestone dates and records in the contract's shape: each date has a role and exactly one of a
 * day (YYYY-MM-DD) or an instant (RFC 3339). */
static PyObject *
milestone_dates(const holder_milestone_date_list *dates)
{
    PyObject *out = PyList_New(0);
    if (out == NULL) return NULL;
    for (size_t i = 0; i < dates->count; ++i) {
        const holder_milestone_date *value = &dates->items[i];
        PyObject *date = PyDict_New();
        if (date == NULL || put_text(date, "role", value->role) < 0 ||
            (value->date != NULL && put_text(date, "date", value->date) < 0) ||
            (value->datetime != NULL && put_text(date, "datetime", value->datetime) < 0) ||
            PyList_Append(out, date) < 0) {
            Py_XDECREF(date);
            Py_DECREF(out);
            return NULL;
        }
        Py_DECREF(date);
    }
    return out;
}

static PyObject *
milestone_record(const char *milestone_id, const char *project_id, const char *card_id,
                 const holder_milestone_date_list *dates, const char *kind, const char *description,
                 int64_t created_at, int64_t updated_at, const char *card_title, int with_title)
{
    PyObject *record = PyDict_New();
    if (record == NULL) return NULL;
    PyObject *date_list = milestone_dates(dates);
    if (date_list == NULL || PyDict_SetItemString(record, "dates", date_list) < 0 ||
        put_text(record, "milestone_id", milestone_id) < 0 ||
        put_text(record, "project_id", project_id) < 0 ||
        put_text(record, "card_id", card_id) < 0 ||
        put_text(record, "kind", kind) < 0 ||
        put_text(record, "description", description) < 0 ||
        put_epoch(record, "created_at", created_at) < 0 ||
        put_epoch(record, "updated_at", updated_at) < 0 ||
        (with_title && put_text(record, "card_title", card_title) < 0)) {
        Py_XDECREF(date_list);
        Py_DECREF(record);
        return NULL;
    }
    Py_DECREF(date_list);
    return record;
}

static PyObject *
milestone_list_record(const holder_milestone_list *list)
{
    PyObject *out = PyList_New(0);
    if (out == NULL) return NULL;
    for (size_t i = 0; i < list->count; ++i) {
        const holder_milestone *m = &list->items[i];
        PyObject *record = milestone_record(m->milestone_id, m->project_id, m->card_id, &m->dates, m->kind,
                                            m->description, m->created_at, m->updated_at, NULL, 0);
        if (record == NULL || PyList_Append(out, record) < 0) {
            Py_XDECREF(record);
            Py_DECREF(out);
            return NULL;
        }
        Py_DECREF(record);
    }
    return out;
}

/* The dates a caller passes: a sequence of dicts with "role" and "date" or "datetime". The strings
 * are borrowed from those dicts, which the caller keeps alive for the call. */
static int
dates_from_python(PyObject *value, holder_milestone_date **out_items, size_t *out_count)
{
    PyObject *sequence = PySequence_Fast(value, "dates must be a sequence of dicts");
    if (sequence == NULL) return 0;
    const Py_ssize_t count = PySequence_Fast_GET_SIZE(sequence);
    holder_milestone_date *items = count == 0 ? NULL : PyMem_Calloc((size_t)count, sizeof *items);
    if (count != 0 && items == NULL) {
        Py_DECREF(sequence);
        PyErr_NoMemory();
        return 0;
    }
    for (Py_ssize_t i = 0; i < count; ++i) {
        PyObject *date = PySequence_Fast_GET_ITEM(sequence, i);
        const char *names[] = {"role", "date", "datetime"};
        const char **fields[] = {&items[i].role, &items[i].date, &items[i].datetime};
        if (!PyDict_Check(date)) {
            PyErr_SetString(PyExc_TypeError, "each date must be a dict");
            goto fail;
        }
        for (int f = 0; f < 3; ++f) {
            /* Borrowed: the dict keeps the string alive for the call. */
            PyObject *field = PyDict_GetItemString(date, names[f]);
            if (field == NULL) {
                if (f == 0) {
                    PyErr_SetString(PyExc_KeyError, "role");
                    goto fail;
                }
                continue;
            }
            if (field == Py_None && f != 0) continue;
            *fields[f] = PyUnicode_AsUTF8(field);
            if (*fields[f] == NULL) goto fail;
        }
    }
    Py_DECREF(sequence);
    *out_items = items;
    *out_count = (size_t)count;
    return 1;
fail:
    PyMem_Free(items);
    Py_DECREF(sequence);
    return 0;
}

/* A partial update's text field: absent keeps it, None clears it, a string sets it. */
static int
change_from_python(PyObject *changes, const char *name, holder_string_change *out)
{
    PyObject *value = PyDict_GetItemString(changes, name);  /* Borrowed. */
    if (value == NULL) {
        out->change = HOLDER_KEEP;
        return 1;
    }
    if (value == Py_None) {
        out->change = HOLDER_CLEAR;
        return 1;
    }
    out->change = HOLDER_SET;
    out->value = PyUnicode_AsUTF8(value);
    return out->value != NULL;
}

static PyObject *
link_list_record(const holder_link_list *list)
{
    PyObject *out = PyList_New(0);
    if (out == NULL) return NULL;
    static const char *source_types[] = {"", "card", "ai_message"};
    static const char *target_types[] = {"", "card", "ai_message", "ai_thread", "resource"};
    for (size_t i = 0; i < list->count; ++i) {
        const holder_link *link = &list->items[i];
        PyObject *record = PyDict_New();
        const unsigned source = (unsigned)link->source_type;
        const unsigned target = (unsigned)link->target_type;
        if (record == NULL ||
            put_text(record, "source_project_id", link->source_project_id) < 0 ||
            put_text(record, "source_type", source < 3 ? source_types[source] : NULL) < 0 ||
            put_text(record, "source_id", link->source_id) < 0 ||
            put_text(record, "target_project_id", link->target_project_id) < 0 ||
            put_text(record, "target_type", target < 5 ? target_types[target] : NULL) < 0 ||
            put_text(record, "target_id", link->target_id) < 0 ||
            put_text(record, "kind", link->kind) < 0 ||
            put_text(record, "label", link->label) < 0 ||
            put_epoch(record, "created_at", link->created_at) < 0 ||
            PyList_Append(out, record) < 0) {
            Py_XDECREF(record);
            Py_DECREF(out);
            return NULL;
        }
        Py_DECREF(record);
    }
    return out;
}

static PyObject *
Context_list_links(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss:list_links", keywords, &project_id, &card_id) ||
        !ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_link_list *links = NULL;
    holder_error *error = NULL;
    const int result = holder_list_card_links(self->context, project_id, card_id, false, arena, &links, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *out = link_list_record(links);
    holder_arena_free(arena);
    return out;
}

static PyObject *
Context_resolve_card(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss:resolve_card", keywords,
                                    &project_id, &card_id) || !ensure_context_open(self)) {
        return NULL;
    }
    holder_card_reference_resolve_request request = {0};
    request.project_id = project_id;
    request.reference = card_id;
    request.scope = HOLDER_CARD_REFERENCE_RESOLVE_REQUEST_SCOPE_EITHER;
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card_reference_resolve_data *resolved = NULL;
    holder_error *error = NULL;
    const int result = holder_resolve_card_reference(self->context, &request,
        arena, &resolved, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    const char *status = resolved->status == HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_RESOLVED
        ? "resolved" : resolved->status == HOLDER_CARD_REFERENCE_RESOLVE_DATA_STATUS_AMBIGUOUS
        ? "ambiguous" : "not_found";
    PyObject *record = PyDict_New();
    if (record == NULL || put_text(record, "status", status) < 0 ||
        put_text(record, "match_kind", resolved->match_kind) < 0) {
        Py_XDECREF(record);
        holder_arena_free(arena);
        return NULL;
    }
    if (resolved->card != NULL) {
        PyObject *card = reference_candidate_record(resolved->card);
        if (card == NULL || PyDict_SetItemString(record, "card", card) < 0) {
            Py_XDECREF(card);
            Py_DECREF(record);
            holder_arena_free(arena);
            return NULL;
        }
        Py_DECREF(card);
    }
    if (resolved->candidates != NULL) {
        PyObject *candidates = PyList_New(0);
        if (candidates == NULL) {
            Py_DECREF(record);
            holder_arena_free(arena);
            return NULL;
        }
        for (size_t i = 0; i < resolved->candidates->count; ++i) {
            PyObject *candidate = reference_candidate_record(&resolved->candidates->items[i]);
            if (candidate == NULL || PyList_Append(candidates, candidate) < 0) {
                Py_XDECREF(candidate);
                Py_DECREF(candidates);
                Py_DECREF(record);
                holder_arena_free(arena);
                return NULL;
            }
            Py_DECREF(candidate);
        }
        if (PyDict_SetItemString(record, "candidates", candidates) < 0) {
            Py_DECREF(candidates);
            Py_DECREF(record);
            holder_arena_free(arena);
            return NULL;
        }
        Py_DECREF(candidates);
    }
    holder_arena_free(arena);
    return record;
}

static PyObject *
Context_get_card(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss:get_card", keywords,
                                    &project_id, &card_id) || !ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card_metadata *card = NULL;
    holder_error *error = NULL;
    const int result = holder_get_card_any_state(self->context, project_id, card_id,
                                                  arena, &card, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *record = card_record(card->card_id, card->project_id, card->title,
        card->rel_path, card->parent_card_id, card->sort_key, card->created_at,
        card->updated_at, card->deleted_at, NULL);
    holder_arena_free(arena);
    return record;
}

static PyObject *
Context_get_project(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    static char *keywords[] = {"project_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:get_project", keywords,
                                    &project_id) || !ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_project *project = NULL;
    holder_error *error = NULL;
    const int result = holder_get_project(self->context, project_id, arena, &project, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *record = project_record(project->project_id, project->name, project->root_path,
        project->git_remote_url, project->git_provider, project->privacy_mode,
        project->project_key_id, project->created_at, project->updated_at);
    holder_arena_free(arena);
    return record;
}

static PyObject *
Context_move_card(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    const char *intent = NULL;
    const char *target_card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", "intent", "target_card_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ssss:move_card", keywords,
                                    &project_id, &card_id, &intent, &target_card_id) ||
        !ensure_context_open(self)) {
        return NULL;
    }
    holder_card_move move = {0};
    if (strcmp(intent, "into") == 0) move.intent = HOLDER_CARD_MOVE_INTENT_INTO;
    else if (strcmp(intent, "before") == 0) move.intent = HOLDER_CARD_MOVE_INTENT_BEFORE;
    else if (strcmp(intent, "after") == 0) move.intent = HOLDER_CARD_MOVE_INTENT_AFTER;
    else {
        PyErr_SetString(PyExc_ValueError, "intent must be into, before or after");
        return NULL;
    }
    move.target_card_id = target_card_id;
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card *card = NULL;
    holder_error *error = NULL;
    const int result = holder_move_card(self->context, project_id, card_id,
                                        &move, arena, &card, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *record = card_record(card->card_id, card->project_id, card->title,
        card->rel_path, card->parent_card_id, card->sort_key, card->created_at,
        card->updated_at, card->deleted_at, NULL);
    holder_arena_free(arena);
    return record;
}

static PyObject *
Context_list_trashed_cards(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    static char *keywords[] = {"project_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:list_trashed_cards", keywords,
                                    &project_id) || !ensure_context_open(self)) return NULL;
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_trash_item_list *items = NULL;
    holder_error *error = NULL;
    const int result = holder_list_trash(self->context, project_id, "card", arena, &items, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *records = PyList_New(0);
    if (records != NULL) for (size_t i = 0; i < items->count; ++i) {
        const holder_trash_item *item = &items->items[i];
        PyObject *record = PyDict_New();
        if (record == NULL || put_text(record, "project_id", item->project_id) < 0 ||
            put_text(record, "card_id", item->card_id) < 0 ||
            PyList_Append(records, record) < 0) {
            Py_XDECREF(record);
            Py_CLEAR(records);
            break;
        }
        Py_DECREF(record);
    }
    holder_arena_free(arena);
    return records;
}

static PyObject *
Context_restore_card(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL, *card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss:restore_card", keywords,
                                    &project_id, &card_id) || !ensure_context_open(self)) return NULL;
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card *card = NULL;
    holder_error *error = NULL;
    const int result = holder_restore_card(self->context, project_id, card_id, arena, &card, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *record = card_record(card->card_id, card->project_id, card->title,
        card->rel_path, card->parent_card_id, card->sort_key, card->created_at,
        card->updated_at, card->deleted_at, NULL);
    holder_arena_free(arena);
    return record;
}

static PyObject *
Context_trash_card(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL, *card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss:trash_card", keywords,
                                    &project_id, &card_id) || !ensure_context_open(self)) return NULL;
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card *card = NULL;
    holder_error *error = NULL;
    const int result = holder_trash_card(self->context, project_id, card_id,
        HOLDER_TRASH_CARD_CHILDREN_DEFAULT, arena, &card, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    holder_arena_free(arena);
    Py_RETURN_NONE;
}

static PyObject *
Context_purge_card(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL, *card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss:purge_card", keywords,
                                    &project_id, &card_id) || !ensure_context_open(self)) return NULL;
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_trash_delete_result *deleted = NULL;
    holder_error *error = NULL;
    const int result = holder_delete_project_trash_item(self->context, project_id,
        HOLDER_DELETE_PROJECT_TRASH_ITEM_TYPE_CARD, card_id, arena, &deleted, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    holder_arena_free(arena);
    Py_RETURN_NONE;
}

static PyObject *
Context_list_tags(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL, *card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss:list_tags", keywords,
                                    &project_id, &card_id) || !ensure_context_open(self)) return NULL;
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card *card = NULL;
    holder_error *error = NULL;
    const int result = holder_get_card(self->context, project_id, card_id, arena, &card, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *tags = PyList_New((Py_ssize_t)card->tags.count);
    if (tags != NULL) for (size_t i = 0; i < card->tags.count; ++i) {
        PyObject *tag = PyUnicode_FromString(card->tags.items[i]);
        if (tag == NULL) { Py_CLEAR(tags); break; }
        PyList_SET_ITEM(tags, i, tag);
    }
    holder_arena_free(arena);
    return tags;
}

static PyObject *
Context_import_project(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_root = NULL;
    static char *keywords[] = {"project_root", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:import_project", keywords,
                                    &project_root) || !ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_project *project = NULL;
    holder_error *error = NULL;
    const int result = holder_import_project(self->context, project_root, arena, &project, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *record = project_record(project->project_id, project->name, project->root_path,
        project->git_remote_url, project->git_provider, project->privacy_mode,
        project->project_key_id, project->created_at, project->updated_at);
    holder_arena_free(arena);
    return record;
}

static PyObject *
Context_list_editable_tags(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL, *card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss:list_editable_tags", keywords,
                                    &project_id, &card_id) || !ensure_context_open(self)) return NULL;
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card *card = NULL;
    holder_error *error = NULL;
    const int result = holder_get_card(self->context, project_id, card_id, arena, &card, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *tags = PyList_New((Py_ssize_t)card->removable_tags.count);
    if (tags != NULL) for (size_t i = 0; i < card->removable_tags.count; ++i) {
        PyObject *tag = PyUnicode_FromString(card->removable_tags.items[i]);
        if (tag == NULL) { Py_CLEAR(tags); break; }
        PyList_SET_ITEM(tags, i, tag);
    }
    holder_arena_free(arena);
    return tags;
}

static PyObject *
Context_list_project_tags(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    static char *keywords[] = {"project_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:list_project_tags", keywords,
                                    &project_id) || !ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_tag_count_list *tags = NULL;
    holder_error *error = NULL;
    const int result = holder_list_project_tags(self->context, project_id, arena, &tags, &error);
    if (result == HOLDER_ERROR_NOT_FOUND) {
        holder_error_destroy(error);
        holder_arena_free(arena);
        return PyList_New(0);
    }
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *records = PyList_New(0);
    if (records == NULL) {
        holder_arena_free(arena);
        return NULL;
    }
    for (size_t i = 0; i < tags->count; ++i) {
        PyObject *record = Py_BuildValue("{s:s,s:i}", "tag", tags->items[i].tag,
                                         "card_count", tags->items[i].card_count);
        if (record == NULL || PyList_Append(records, record) < 0) {
            Py_XDECREF(record);
            Py_DECREF(records);
            holder_arena_free(arena);
            return NULL;
        }
        Py_DECREF(record);
    }
    holder_arena_free(arena);
    return records;
}

static PyObject *
Context_list_milestones(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss:list_milestones", keywords, &project_id, &card_id) ||
        !ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_milestone_list *list = NULL;
    holder_error *error = NULL;
    const int result = holder_list_card_milestones(self->context, project_id, card_id, arena, &list, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *out = milestone_list_record(list);
    holder_arena_free(arena);
    return out;
}

static PyObject *
Context_add_milestone(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    PyObject *dates_object = NULL;
    const char *kind = NULL;
    const char *description = NULL;
    static char *keywords[] = {"project_id", "card_id", "dates", "kind", "description", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ssO|zz:add_milestone", keywords,
                                    &project_id, &card_id, &dates_object, &kind, &description) ||
        !ensure_context_open(self)) {
        return NULL;
    }
    holder_new_milestone request = {0};
    holder_milestone_date *dates = NULL;
    size_t count = 0;
    if (!dates_from_python(dates_object, &dates, &count)) return NULL;
    request.dates.items = dates;
    request.dates.count = count;
    request.kind = kind;
    request.description = description;
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) {
        PyMem_Free(dates);
        return PyErr_NoMemory();
    }
    const holder_milestone *m = NULL;
    holder_error *error = NULL;
    const int result = holder_add_card_milestone(self->context, project_id, card_id, &request, arena, &m, &error);
    PyMem_Free(dates);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *out = milestone_record(m->milestone_id, m->project_id, m->card_id, &m->dates, m->kind,
                                     m->description, m->created_at, m->updated_at, NULL, 0);
    holder_arena_free(arena);
    return out;
}

static PyObject *
Context_update_milestone(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    const char *milestone_id = NULL;
    PyObject *changes = NULL;
    static char *keywords[] = {"project_id", "card_id", "milestone_id", "changes", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sssO:update_milestone", keywords,
                                    &project_id, &card_id, &milestone_id, &changes) ||
        !ensure_context_open(self)) {
        return NULL;
    }
    if (!PyDict_Check(changes)) {
        PyErr_SetString(PyExc_TypeError, "changes must be a dict");
        return NULL;
    }
    holder_milestone_update update = {0};
    holder_milestone_date_list date_list = {0};
    holder_milestone_date *dates = NULL;
    PyObject *dates_object = PyDict_GetItemString(changes, "dates");  /* Borrowed. */
    if (dates_object != NULL) {
        size_t count = 0;
        if (!dates_from_python(dates_object, &dates, &count)) return NULL;
        date_list.items = dates;
        date_list.count = count;
        update.dates = &date_list;
    }
    if (!change_from_python(changes, "kind", &update.kind) ||
        !change_from_python(changes, "description", &update.description)) {
        PyMem_Free(dates);
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) {
        PyMem_Free(dates);
        return PyErr_NoMemory();
    }
    const holder_milestone *m = NULL;
    holder_error *error = NULL;
    const int result = holder_update_card_milestone(self->context, project_id, card_id, milestone_id, &update,
                                                    arena, &m, &error);
    PyMem_Free(dates);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *out = milestone_record(m->milestone_id, m->project_id, m->card_id, &m->dates, m->kind,
                                     m->description, m->created_at, m->updated_at, NULL, 0);
    holder_arena_free(arena);
    return out;
}

static PyObject *
Context_remove_milestone(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    const char *milestone_id = NULL;
    static char *keywords[] = {"project_id", "card_id", "milestone_id", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sss:remove_milestone", keywords,
                                    &project_id, &card_id, &milestone_id) || !ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_milestone_list *list = NULL;
    holder_error *error = NULL;
    const int result = holder_remove_card_milestone(self->context, project_id, card_id, milestone_id, arena,
                                                    &list, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *out = milestone_list_record(list);
    holder_arena_free(arena);
    return out;
}

static PyObject *
Context_calendar_milestones(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *first_day = NULL;
    const char *last_day = NULL;
    const char *time_zone = NULL;
    static char *keywords[] = {"project_id", "first_day", "last_day", "time_zone", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sss|z:calendar_milestones", keywords,
                                    &project_id, &first_day, &last_day, &time_zone) ||
        !ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_calendar *calendar = NULL;
    holder_error *error = NULL;
    const int result = holder_get_calendar(self->context, project_id, first_day, last_day, time_zone, arena,
                                           &calendar, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *out = PyList_New(0);
    for (size_t i = 0; out != NULL && i < calendar->milestones.count; ++i) {
        const holder_calendar_milestone *m = &calendar->milestones.items[i];
        PyObject *record = milestone_record(m->milestone_id, m->project_id, m->card_id, &m->dates, m->kind,
                                            m->description, m->created_at, m->updated_at, m->card_title, 1);
        if (record == NULL || PyList_Append(out, record) < 0) {
            Py_XDECREF(record);
            Py_CLEAR(out);
            break;
        }
        Py_DECREF(record);
    }
    holder_arena_free(arena);
    return out;
}

typedef int (*tag_mutation)(holder_context *, const char *, const char *, const char *,
                            holder_arena *, const holder_card **, holder_error **);

static PyObject *
mutate_tag(ContextObject *self, PyObject *args, PyObject *kwargs, tag_mutation operation)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    const char *tag = NULL;
    static char *keywords[] = {"project_id", "card_id", "tag", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sss", keywords,
                                    &project_id, &card_id, &tag) ||
        !ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card *card = NULL;
    holder_error *error = NULL;
    const int result = operation(self->context, project_id, card_id, tag, arena, &card, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    holder_arena_free(arena);
    Py_RETURN_NONE;
}

static PyObject *
Context_add_tag(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    return mutate_tag(self, args, kwargs, holder_add_card_tag);
}

static PyObject *
Context_remove_tag(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    return mutate_tag(self, args, kwargs, holder_remove_card_tag);
}

static PyObject *
Context_cards_with_tag(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *tag = NULL;
    static char *keywords[] = {"project_id", "tag", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss:cards_with_tag", keywords,
                                    &project_id, &tag) || !ensure_context_open(self)) {
        return NULL;
    }
    PyObject *records = PyList_New(0);
    if (records == NULL) return NULL;
    PyObject *cursor = NULL;
    for (;;) {
        holder_arena *arena = holder_arena_new();
        if (arena == NULL) {
            Py_XDECREF(cursor);
            Py_DECREF(records);
            return PyErr_NoMemory();
        }
        holder_list_cards_parent parent = {0};
        const holder_card_list_item_page *page = NULL;
        holder_error *error = NULL;
        const int result = holder_list_cards(self->context, project_id, parent, tag,
            HOLDER_LIST_CARDS_ORDER_DEFAULT, NULL,
            cursor == NULL ? NULL : PyBytes_AsString(cursor), false, false,
            arena, &page, &error);
        if (result == HOLDER_ERROR_NOT_FOUND) {
            holder_error_destroy(error);
            holder_arena_free(arena);
            Py_XDECREF(cursor);
            return records;
        }
        if (result != HOLDER_OK) {
            raise_holder_error(self, result, error);
            holder_arena_free(arena);
            Py_XDECREF(cursor);
            Py_DECREF(records);
            return NULL;
        }
        holder_error_destroy(error);
        for (size_t i = 0; i < page->items.count; ++i) {
            const holder_card_list_item *card = &page->items.items[i];
            PyObject *record = Py_BuildValue("{s:s,s:s}", "card_id", card->card_id,
                                              "title", card->title);
            if (record == NULL || PyList_Append(records, record) < 0) {
                Py_XDECREF(record);
                holder_arena_free(arena);
                Py_XDECREF(cursor);
                Py_DECREF(records);
                return NULL;
            }
            Py_DECREF(record);
        }
        PyObject *next_cursor = page->next_cursor == NULL ? NULL : PyBytes_FromString(page->next_cursor);
        holder_arena_free(arena);
        Py_XDECREF(cursor);
        if (next_cursor == NULL) {
            if (PyErr_Occurred()) {
                Py_DECREF(records);
                return NULL;
            }
            return records;
        }
        cursor = next_cursor;
    }
}

static PyObject *
Context_add_link(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *from_card_id = NULL;
    const char *target_project_id = NULL;
    const char *to_card_id = NULL;
    const char *kind = NULL;
    const char *label = NULL;
    static char *keywords[] = {"project_id", "from_card_id", "target_project_id", "to_card_id", "kind", "label", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sszss|z:add_link", keywords,
                                    &project_id, &from_card_id, &target_project_id,
                                    &to_card_id, &kind, &label)) {
        return NULL;
    }
    if (!ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    holder_new_link link = {0};
    link.target_type = HOLDER_NEW_LINK_TARGET_TYPE_CARD;
    link.target_id = to_card_id;
    link.target_project_id = target_project_id;
    link.kind = kind;
    link.label = label;
    const holder_link *created = NULL;
    holder_error *error = NULL;
    const int result = holder_add_card_link(self->context, project_id, from_card_id,
        &link, arena, &created, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    holder_arena_free(arena);
    Py_RETURN_NONE;
}

static PyObject *
Context_remove_link(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *from_card_id = NULL;
    const char *target_project_id = NULL;
    const char *to_card_id = NULL;
    const char *kind = NULL;
    static char *keywords[] = {"project_id", "from_card_id", "target_project_id", "to_card_id", "kind", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sszss:remove_link", keywords,
                                    &project_id, &from_card_id, &target_project_id,
                                    &to_card_id, &kind)) {
        return NULL;
    }
    if (!ensure_context_open(self)) {
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_link_list *remaining = NULL;
    holder_error *error = NULL;
    const int result = holder_remove_card_link(self->context, project_id,
        from_card_id, HOLDER_REMOVE_CARD_LINK_TARGET_TYPE_CARD, to_card_id,
        kind, target_project_id, arena, &remaining, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    holder_arena_free(arena);
    Py_RETURN_NONE;
}

static PyObject *
Context_list_cards(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    static char *keywords[] = {"project_id", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s:list_cards", keywords, &project_id)) {
        return NULL;
    }
    if (!ensure_context_open(self)) {
        return NULL;
    }

    PyObject *records = PyList_New(0);
    if (records == NULL) return NULL;
    PyObject *cursor = NULL;
    for (;;) {
        holder_arena *arena = holder_arena_new();
        if (arena == NULL) {
            Py_XDECREF(cursor);
            Py_DECREF(records);
            return PyErr_NoMemory();
        }
        holder_list_cards_parent parent = {0};
        const holder_card_list_item_page *page = NULL;
        holder_error *error = NULL;
        const int result = holder_list_cards(self->context, project_id, parent, NULL,
            HOLDER_LIST_CARDS_ORDER_DEFAULT, NULL,
            cursor == NULL ? NULL : PyBytes_AsString(cursor), false, false,
            arena, &page, &error);
        if (result == HOLDER_ERROR_NOT_FOUND) {
            holder_error_destroy(error);
            holder_arena_free(arena);
            Py_XDECREF(cursor);
            return records;
        }
        if (result != HOLDER_OK) {
            raise_holder_error(self, result, error);
            holder_arena_free(arena);
            Py_XDECREF(cursor);
            Py_DECREF(records);
            return NULL;
        }
        holder_error_destroy(error);
        for (size_t i = 0; i < page->items.count; ++i) {
            const holder_card_list_item *card = &page->items.items[i];
            PyObject *record = card_record(card->card_id, card->project_id,
                card->title, card->rel_path, card->parent_card_id, card->sort_key,
                card->created_at, card->updated_at, card->deleted_at, NULL);
            if (record == NULL || PyList_Append(records, record) < 0) {
                Py_XDECREF(record);
                holder_arena_free(arena);
                Py_XDECREF(cursor);
                Py_DECREF(records);
                return NULL;
            }
            Py_DECREF(record);
        }
        PyObject *next_cursor = page->next_cursor == NULL ? NULL : PyBytes_FromString(page->next_cursor);
        holder_arena_free(arena);
        Py_XDECREF(cursor);
        if (next_cursor == NULL) {
            if (PyErr_Occurred()) {
                Py_DECREF(records);
                return NULL;
            }
            return records;
        }
        cursor = next_cursor;
    }
}

static PyObject *
Context_list_cards_page(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *parent_card_id = NULL;
    int roots = 0;
    const char *tag = NULL;
    const char *order = "card_id";
    int limit = 256;
    const char *cursor = NULL;
    int include_content = 0;
    static char *keywords[] = {
        "project_id", "parent_card_id", "roots", "tag", "order", "limit", "cursor", "include_content", NULL
    };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|$zpzsizp:list_cards_page", keywords, &project_id,
                                    &parent_card_id, &roots, &tag, &order, &limit, &cursor, &include_content) ||
        !ensure_context_open(self)) {
        return NULL;
    }
    holder_list_cards_parent parent = {0};
    if (parent_card_id != NULL) {
        parent.kind = HOLDER_LIST_CARDS_PARENT_CARD_ID;
        parent.card_id = parent_card_id;
    } else if (roots) {
        parent.kind = HOLDER_LIST_CARDS_PARENT_ROOTS;
    }
    holder_list_cards_order card_order;
    if (strcmp(order, "card_id") == 0) {
        card_order = HOLDER_LIST_CARDS_ORDER_CARD_ID_ASC;
    } else if (strcmp(order, "updated") == 0) {
        card_order = HOLDER_LIST_CARDS_ORDER_UPDATED_DESC;
    } else if (strcmp(order, "sort_key") == 0) {
        card_order = HOLDER_LIST_CARDS_ORDER_SORT_KEY_ASC;
    } else {
        PyErr_SetString(PyExc_ValueError, "order must be 'card_id', 'updated' or 'sort_key'");
        return NULL;
    }
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card_list_item_page *page = NULL;
    holder_error *error = NULL;
    const int32_t requested_limit = limit;
    const int result = holder_list_cards(self->context, project_id, parent, tag, card_order, &requested_limit,
        cursor, include_content != 0, false, arena, &page, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *cards = PyList_New(0);
    if (cards == NULL) {
        holder_arena_free(arena);
        return NULL;
    }
    for (size_t i = 0; i < page->items.count; ++i) {
        const holder_card_list_item *card = &page->items.items[i];
        PyObject *record = card_record(card->card_id, card->project_id, card->title,
            card->rel_path, card->parent_card_id, card->sort_key, card->created_at,
            card->updated_at, card->deleted_at, include_content ? card->content : NULL);
        if (record == NULL || PyList_Append(cards, record) < 0) {
            Py_XDECREF(record);
            Py_DECREF(cards);
            holder_arena_free(arena);
            return NULL;
        }
        Py_DECREF(record);
    }
    PyObject *next_cursor = page->next_cursor == NULL
        ? Py_NewRef(Py_None) : PyUnicode_FromString(page->next_cursor);
    holder_arena_free(arena);
    if (next_cursor == NULL) {
        Py_DECREF(cards);
        return NULL;
    }
    PyObject *response = Py_BuildValue("{s:O,s:O}", "cards", cards, "next_cursor", next_cursor);
    Py_DECREF(cards);
    Py_DECREF(next_cursor);
    return response;
}

static PyObject *
Context_get_card_content(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    static char *keywords[] = {"project_id", "card_id", NULL};

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "ss:get_card_content",
            keywords,
            &project_id,
            &card_id)) {
        return NULL;
    }
    if (!ensure_context_open(self)) {
        return NULL;
    }

    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card *card = NULL;
    holder_error *error = NULL;
    const int result = holder_get_card(self->context, project_id, card_id,
                                       arena, &card, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *content = PyUnicode_FromString(card->content);
    holder_arena_free(arena);
    return content;
}

static PyObject *
Context_update_card(ContextObject *self, PyObject *args, PyObject *kwargs)
{
    const char *project_id = NULL;
    const char *card_id = NULL;
    const char *content = NULL;
    const char *title = NULL;
    static char *keywords[] = {"project_id", "card_id", "content", "title", NULL};

    if (!PyArg_ParseTupleAndKeywords(
            args,
            kwargs,
            "sss|z:update_card",
            keywords,
            &project_id,
            &card_id,
            &content,
            &title)) {
        return NULL;
    }
    if (!ensure_context_open(self)) {
        return NULL;
    }

    holder_card_update update = {0};
    update.content = content;
    update.title = title;
    holder_arena *arena = holder_arena_new();
    if (arena == NULL) return PyErr_NoMemory();
    const holder_card *card = NULL;
    holder_error *error = NULL;
    const int result = holder_update_card(self->context, project_id, card_id,
                                          &update, arena, &card, &error);
    if (result != HOLDER_OK) {
        raise_holder_error(self, result, error);
        holder_arena_free(arena);
        return NULL;
    }
    holder_error_destroy(error);
    PyObject *record = card_record(card->card_id, card->project_id, card->title,
        card->rel_path, card->parent_card_id, card->sort_key, card->created_at,
        card->updated_at, card->deleted_at, NULL);
    holder_arena_free(arena);
    return record;
}

static PyObject *
Context_get_closed(ContextObject *self, void *Py_UNUSED(closure))
{
    return PyBool_FromLong(self->context == NULL);
}

PyDoc_STRVAR(
    Context_doc,
    "Context(data_dir, schema_sql)\n--\n\n"
    "Own an embedded libholder context. Prefer holderkit.Context in public code."
);

static PyMethodDef Context_methods[] = {
    {"import_project", PyCFunction_CAST(Context_import_project), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("Import a durable plain project into an empty context database.")},
    {"list_milestones", PyCFunction_CAST(Context_list_milestones), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("List a card's milestones.")},
    {"add_milestone", PyCFunction_CAST(Context_add_milestone), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("Add a milestone with labelled dates and return it.")},
    {"update_milestone", PyCFunction_CAST(Context_update_milestone), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("Partially update a milestone: absent keeps a field, None clears it.")},
    {"remove_milestone", PyCFunction_CAST(Context_remove_milestone), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("Remove a milestone and return the card's remaining milestones.")},
    {"calendar_milestones", PyCFunction_CAST(Context_calendar_milestones), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("List a project's live milestones with a date in days in a time zone.")},
    {"list_tags", PyCFunction_CAST(Context_list_tags), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("List a card's normalized tags.")},
    {"list_editable_tags", PyCFunction_CAST(Context_list_editable_tags), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("List tags on a card's editable trailing tag line.")},
    {"list_project_tags", PyCFunction_CAST(Context_list_project_tags), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("List project tags and live-card counts.")},
    {"cards_with_tag", PyCFunction_CAST(Context_cards_with_tag), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("Find live cards carrying a tag in a project.")},
    {"add_tag", PyCFunction_CAST(Context_add_tag), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("Semantically add a tag and return core's status.")},
    {"remove_tag", PyCFunction_CAST(Context_remove_tag), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("Remove a trailing tag and return core's status.")},
    {"list_links", PyCFunction_CAST(Context_list_links),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Read a card's connections and hierarchy.")},
    {"add_link", PyCFunction_CAST(Context_add_link),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Add or update an explicit card connection.")},
    {"remove_link", PyCFunction_CAST(Context_remove_link),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Remove an explicit card connection.")},
    {"close", (PyCFunction)Context_close, METH_NOARGS,
     PyDoc_STR("Release the native Holder context. Safe to call repeatedly.")},
    {"__enter__", (PyCFunction)Context_enter, METH_NOARGS, NULL},
    {"__exit__", (PyCFunction)Context_exit, METH_VARARGS, NULL},
    {"create_project", PyCFunction_CAST(Context_create_project),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create a plain Holder project.")},
    {"list_projects", (PyCFunction)Context_list_projects, METH_NOARGS,
     PyDoc_STR("List Holder projects.")},
    {"get_project", PyCFunction_CAST(Context_get_project),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Get one project by exact ID.")},
    {"create_card", PyCFunction_CAST(Context_create_card),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Create a card and return its metadata.")},
    {"list_cards", PyCFunction_CAST(Context_list_cards),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("List live cards in a project.")},
    {"get_card", PyCFunction_CAST(Context_get_card),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Get one card by exact ID and scope.")},
    {"resolve_card", PyCFunction_CAST(Context_resolve_card),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Resolve a live or trashed card through Core.")},
    {"move_card", PyCFunction_CAST(Context_move_card),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Move a card using Core's placement rules.")},
    {"list_trashed_cards", PyCFunction_CAST(Context_list_trashed_cards),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("List trashed card metadata in a project.")},
    {"trash_card", PyCFunction_CAST(Context_trash_card),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Move a card to Trash and promote its live children.")},
    {"restore_card", PyCFunction_CAST(Context_restore_card),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Restore one trashed card through Core.")},
    {"purge_card", PyCFunction_CAST(Context_purge_card),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Permanently remove a card that is already in Trash.")},
    {"list_cards_page", PyCFunction_CAST(Context_list_cards_page), METH_VARARGS | METH_KEYWORDS,
     PyDoc_STR("Return one page of a project's live cards, by parent, tag and order.")},
    {"get_card_content", PyCFunction_CAST(Context_get_card_content),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Return a card's Markdown body.")},
    {"update_card", PyCFunction_CAST(Context_update_card),
     METH_VARARGS | METH_KEYWORDS, PyDoc_STR("Replace a card's content and optionally title.")},
    {NULL, NULL, 0, NULL}
};

static PyGetSetDef Context_getset[] = {
    {"closed", (getter)Context_get_closed, NULL,
     PyDoc_STR("Whether the native context has been released."), NULL},
    {NULL, NULL, NULL, NULL, NULL}
};

static PyType_Slot Context_slots[] = {
    {Py_tp_doc, (void *)Context_doc},
    {Py_tp_dealloc, (void *)Context_dealloc},
    {Py_tp_init, (void *)Context_init},
    {Py_tp_new, (void *)PyType_GenericNew},
    {Py_tp_methods, Context_methods},
    {Py_tp_getset, Context_getset},
    {0, NULL}
};

static PyType_Spec Context_spec = {
    .name = "holderkit._native.Context",
    .basicsize = sizeof(ContextObject),
    .itemsize = 0,
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Context_slots,
};

static int
holder_module_traverse(PyObject *module, visitproc visit, void *arg)
{
    holder_module_state *state = PyModule_GetState(module);
    Py_VISIT(state->holder_error);
    return 0;
}

static int
holder_module_clear(PyObject *module)
{
    holder_module_state *state = PyModule_GetState(module);
    Py_CLEAR(state->holder_error);
    return 0;
}

static int
holder_module_exec(PyObject *module)
{
    if (PyModule_AddIntConstant(module, "CARD_PAGE_MAX_LIMIT", 1000) < 0) {
        return -1;
    }
    holder_module_state *state = PyModule_GetState(module);
    state->holder_error = PyErr_NewException(
        "holderkit._native.HolderError",
        PyExc_RuntimeError,
        NULL
    );
    if (state->holder_error == NULL) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "HolderError", state->holder_error) < 0) {
        return -1;
    }

    PyObject *context_type = PyType_FromModuleAndSpec(module, &Context_spec, NULL);
    if (context_type == NULL) {
        return -1;
    }
    if (PyModule_AddObject(module, "Context", context_type) < 0) {
        Py_DECREF(context_type);
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot holder_module_slots[] = {
    {Py_mod_exec, holder_module_exec},
    {0, NULL}
};

static struct PyModuleDef holder_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "holderkit._native",
    .m_doc = "Native CPython bindings for embedded libholder.",
    .m_size = sizeof(holder_module_state),
    .m_methods = NULL,
    .m_slots = holder_module_slots,
    .m_traverse = holder_module_traverse,
    .m_clear = holder_module_clear,
    .m_free = NULL,
};

PyMODINIT_FUNC
PyInit__native(void)
{
    return PyModuleDef_Init(&holder_module);
}
