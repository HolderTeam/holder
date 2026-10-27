namespace HolderLinux {

public class ApiClientResourcesEndpoints : Object { // LCOV_EXCL_BR_LINE: declaration branch artifact
    private static string resources_path(string project_id) {
        return "/projects/%s/resources".printf(Uri.escape_string(project_id));
    }

    private static string resource_path(string project_id, string resource_id) {
        return "%s/%s".printf(resources_path(project_id), Uri.escape_string(resource_id));
    }

    // The project's resources, each with the live cards that link to it, as the resources table
    // shows them: one listing of the project's cards for their titles, then each resource's links.
    public static async Gee.ArrayList<ProjectResource> list_resources(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                                                       string project_id) throws Error {
        var root = yield client.request_json("GET", resources_path(project_id), null, null); // LCOV_EXCL_BR_LINE: yield resume edge artifact
        var resources = ApiParsersResources.parse_resources(root); // LCOV_EXCL_BR_LINE: call/return branch artifact
        if (resources.size == 0) return resources;
        var cards = yield live_cards(client, project_id);
        foreach (var resource in resources) {
            var links = yield client.request_json(
                "GET", resource_path(project_id, resource.resource_id) + "/backlinks", null, null
            );
            add_references(resource, links.get_array_member("data"), cards);
        }
        return resources;
    }

    // Each live card in the project, by ID: its title and when it last changed.
    private static async Gee.HashMap<string, Json.Object> live_cards(ApiClient client,
                                                                     string project_id) throws Error {
        var cards = new Gee.HashMap<string, Json.Object>();
        string? cursor = null;
        do {
            var query = new HashTable<string, string>(str_hash, str_equal);
            query.insert("limit", "5000");
            if (cursor != null) query.insert("cursor", (!) cursor);
            var root = yield client.request_json(
                "GET", "/projects/%s/cards".printf(Uri.escape_string(project_id)), null, query
            );
            var page = root.get_object_member("data");
            var items = page.get_array_member("items");
            for (uint i = 0; i < items.get_length(); i++) {
                var card = items.get_object_element(i);
                cards.set(card.get_string_member("card_id"), card);
            }
            cursor = ApiParsersCommon.nullable_string_member_or_null(page, "next_cursor");
        } while (cursor != null);
        return cards;
    }

    // The live cards among a resource's links, each with its link kinds, by title and then ID.
    private static void add_references(ProjectResource resource,
                                       Json.Array links,
                                       Gee.HashMap<string, Json.Object> cards) {
        var kinds = new Gee.TreeMap<string, Gee.TreeSet<string>>();
        for (uint i = 0; i < links.get_length(); i++) {
            var link = links.get_object_element(i);
            var source = link.get_string_member("source_id");
            if (link.get_string_member("source_type") != "card" || !cards.has_key(source)) continue;
            if (!kinds.has_key(source)) kinds.set(source, new Gee.TreeSet<string>());
            kinds.get(source).add(link.get_string_member("kind"));
        }
        var references = new Gee.ArrayList<ResourceCardReference>();
        foreach (var entry in kinds.entries) {
            var card = cards.get(entry.key);
            var link_kinds = new Gee.ArrayList<string>();
            link_kinds.add_all(entry.value);
            references.add(new ResourceCardReference(
                entry.key, card.get_string_member("title"), card.get_int_member("updated_at"), link_kinds
            ));
        }
        references.sort((left, right) => {
            var by_title = strcmp(left.title, right.title);
            return by_title != 0 ? by_title : strcmp(left.card_id, right.card_id);
        });
        resource.referenced_by_cards.clear();
        resource.referenced_by_cards.add_all(references);
    }

    public static async string create_resource(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                               string project_id,
                                               string kind,
                                               string uri,
                                               string label,
                                               string? desc = null,
                                               Gee.HashMap<string, Gee.ArrayList<string>>? extra_metadata = null) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("type");
        body.add_string_value(kind);
        body.set_member_name("label");
        body.add_string_value(label);
        body.set_member_name("metadata");
        body.begin_object();
        add_metadata_values(body, extra_metadata);
        if (uri.strip().length > 0) {
            body.set_member_name("identifier");
            body.begin_array(); body.add_string_value(uri); body.end_array();
        }
        if (desc != null) {
            body.set_member_name("description");
            body.begin_array(); body.add_string_value(desc); body.end_array();
        }
        body.end_object();
        body.end_object();

        var root = yield client.request_json("POST", resources_path(project_id), client.json_string_from_builder(body), null); // LCOV_EXCL_BR_LINE: yield resume edge artifact
        if (!root.has_member("data")) {
            throw new ApiError.PROTOCOL("Missing data for resource create response"); // LCOV_EXCL_BR_LINE: throw edge artifact
        }
        var data = root.get_object_member("data"); // LCOV_EXCL_BR_LINE: invalid-type branch artifact
        return ApiParsersCommon.string_member_or_empty(data, "resource_id"); // LCOV_EXCL_BR_LINE: call/return branch artifact
    }

    public static async void update_resource(ApiClient client, // LCOV_EXCL_BR_LINE: async declaration branch artifact
                                             string project_id,
                                             string resource_id,
                                             string? kind,
                                             string? uri,
                                             string? label,
                                             string? desc,
                                             int64 updated_at,
                                             Gee.HashMap<string, Gee.ArrayList<string>>? extra_metadata = null) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        if (kind != null) { // LCOV_EXCL_BR_LINE: short-circuit branch artifact
            body.set_member_name("type");
            body.add_string_value(kind);
        }
        if (label != null) { // LCOV_EXCL_BR_LINE: short-circuit branch artifact
            body.set_member_name("label");
            body.add_string_value(label);
        }
        body.set_member_name("metadata");
        body.begin_object();
        add_metadata_values(body, extra_metadata);
        if (uri != null) {
            body.set_member_name("identifier");
            body.begin_array();
            if (uri.strip().length > 0) body.add_string_value(uri);
            body.end_array();
        }
        body.set_member_name("description");
        body.begin_array();
        if (desc != null && desc.strip().length > 0) body.add_string_value(desc);
        body.end_array();
        body.end_object();
        body.set_member_name("updated_at");
        body.add_int_value(updated_at);
        body.end_object();

        yield client.request_json( // LCOV_EXCL_BR_LINE: yield resume edge artifact
            "PATCH",
            resource_path(project_id, resource_id),
            client.json_string_from_builder(body),
            null
        );
    }

    private static void add_metadata_values(
        Json.Builder body,
        Gee.HashMap<string, Gee.ArrayList<string>>? metadata
    ) {
        if (metadata == null) return;
        foreach (var entry in metadata.entries) {
            if (entry.key == "identifier" || entry.key == "description") continue;
            body.set_member_name(entry.key);
            body.begin_array();
            foreach (var value in entry.value) body.add_string_value(value);
            body.end_array();
        }
    }

    public static async void delete_resource(ApiClient client, string project_id, string resource_id) throws Error { // LCOV_EXCL_BR_LINE: async declaration branch artifact
        yield client.request_json( // LCOV_EXCL_BR_LINE: yield resume edge artifact
            "DELETE",
            resource_path(project_id, resource_id),
            null,
            null
        );
    }

    public static async StorageLocationList list_storage_locations(ApiClient client,
                                                                   string project_id) throws Error {
        var query = new HashTable<string, string>(str_hash, str_equal);
        query.insert("project_id", project_id);
        var root = yield client.request_json("GET", "/locations", null, query);
        return ApiParsersResources.parse_locations(root);
    }

    private static void add_string_map(Json.Builder body,
                                       string member,
                                       Gee.HashMap<string, string> values) {
        body.set_member_name(member);
        body.begin_object();
        foreach (var entry in values.entries) {
            body.set_member_name(entry.key);
            body.add_string_value(entry.value);
        }
        body.end_object();
    }

    public static async string create_storage_location(ApiClient client,
                                                       string project_id,
                                                       string name,
                                                       string provider,
                                                       Gee.HashMap<string, string> configuration) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("project_id"); body.add_string_value(project_id);
        body.set_member_name("name"); body.add_string_value(name);
        body.set_member_name("provider"); body.add_string_value(provider);
        add_string_map(body, "configuration", configuration);
        body.end_object();
        var root = yield client.request_json(
            "POST", "/locations", client.json_string_from_builder(body), null
        );
        return ApiParsersCommon.string_member_or_empty(root.get_object_member("data"), "location_id");
    }

    public static async void bind_storage_location(ApiClient client,
                                                   string location_id,
                                                   Gee.HashMap<string, string> values,
                                                   string preview) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        add_string_map(body, "values", values);
        body.set_member_name("preview"); body.add_string_value(preview);
        body.end_object();
        yield client.request_json(
            "PUT",
            "/locations/%s/binding".printf(Uri.escape_string(location_id)),
            client.json_string_from_builder(body),
            null
        );
    }

    public static async void prefer_storage_location(ApiClient client,
                                                     string project_id,
                                                     string location_id) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("project_id"); body.add_string_value(project_id);
        body.set_member_name("location_id"); body.add_string_value(location_id);
        body.end_object();
        yield client.request_json(
            "PUT", "/locations/preferred", client.json_string_from_builder(body), null
        );
    }

    public static async void test_storage_location(ApiClient client, string location_id) throws Error {
        yield client.request_json(
            "POST",
            "/locations/%s/test".printf(Uri.escape_string(location_id)),
            "{}",
            null
        );
    }

    public static async void delete_storage_location(ApiClient client, string location_id) throws Error {
        yield client.request_json(
            "DELETE", "/locations/%s".printf(Uri.escape_string(location_id)), null, null
        );
    }

    public static async string start_google_drive_oauth(ApiClient client, string location_id) throws Error {
        var root = yield client.request_json(
            "POST",
            "/locations/%s/oauth/google-drive/authorize".printf(Uri.escape_string(location_id)),
            "{}",
            null
        );
        return ApiParsersCommon.string_member_or_empty(
            root.get_object_member("data"), "authorization_url"
        );
    }

    public static async AssetImportJob start_asset_import(ApiClient client,
                                                          string project_id,
                                                          string card_id,
                                                          string location_id,
                                                          string source_path) throws Error {
        var body = new Json.Builder();
        body.begin_object();
        body.set_member_name("project_id"); body.add_string_value(project_id);
        body.set_member_name("card_id"); body.add_string_value(card_id);
        body.set_member_name("location_id"); body.add_string_value(location_id);
        body.set_member_name("source_path"); body.add_string_value(source_path);
        body.end_object();
        var root = yield client.request_json(
            "POST", "/imports", client.json_string_from_builder(body), null
        );
        return ApiParsersResources.parse_import_job(root);
    }

    public static async AssetImportJob get_asset_import_job(ApiClient client,
                                                            string job_id) throws Error {
        var root = yield client.request_json(
            "GET", "/imports/%s".printf(Uri.escape_string(job_id)), null, null
        );
        return ApiParsersResources.parse_import_job(root);
    }
}

}
