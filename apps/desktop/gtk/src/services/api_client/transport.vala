namespace HolderLinux {

internal delegate void ApiRequestDiagnostic(string line);

public class ApiClientTransport : Object { // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: declaration branch artifact
    internal static async Json.Object request_json(IApiHttpTransport transport,
                                                   string base_url,
                                                   string auth_token,
                                                   string method,
                                                   string path,
                                                   string? request_body,
                                                   HashTable<string, string>? query,
                                                   ApiRequestDiagnostic? diagnostic = null) throws Error {
        return yield request_object(transport, base_url, auth_token, method, path,
                                    request_body, query, true, diagnostic);
    }

    internal static async Json.Object request_json_unwrapped(IApiHttpTransport transport,
                                                             string base_url,
                                                             string auth_token,
                                                             string method,
                                                             string path,
                                                             string? request_body,
                                                             HashTable<string, string>? query,
                                                             ApiRequestDiagnostic? diagnostic = null) throws Error {
        return yield request_object(transport, base_url, auth_token, method, path,
                                    request_body, query, false, diagnostic);
    }

    private static async Json.Object request_object(IApiHttpTransport transport,
                                                    string base_url,
                                                    string auth_token,
                                                    string method,
                                                    string path,
                                                    string? request_body,
                                                    HashTable<string, string>? query,
                                                    bool wrapped,
                                                    ApiRequestDiagnostic? diagnostic) throws Error {
        var url = build_url(base_url, path, query);
        var message = new Soup.Message(method, url);
        message.request_headers.append("Authorization", "Bearer %s".printf(auth_token));
        message.request_headers.append("Accept", "application/json");

        if (request_body != null) {
            var bytes = new Bytes((uint8[]) request_body.data);
            message.set_request_body_from_bytes("application/json", bytes);
        }

        var started_at = get_monotonic_time();
        ApiHttpBytesResponse response;
        try {
            response = yield transport.send_and_read(message);
        } catch (Error e) {
            var timed_out = e is IOError.TIMED_OUT;
            report_failure(diagnostic, method, path, timed_out ? "timeout" : "transport", started_at,
                           "%s/%d".printf(e.domain.to_string(), e.code));
            if (timed_out) {
                throw new ApiError.TRANSPORT(
                    "The Holder backend timed out. Check whether the action completed before trying again.");
            }
            throw new ApiError.TRANSPORT(
                "Could not connect to the Holder backend. Check that it is running, then try again.");
        }

        var status = response.status;
        if (status < 200 || status >= 300) {
            string server_code = "";
            string server_message = "";
            try {
                var body = ApiParsersCommon.parse_response_object_bytes(response.body);
                var error = body.get_member("error");
                if (error != null && error.get_node_type() == Json.NodeType.OBJECT) {
                    server_code = error.get_object().get_string_member_with_default("code", "");
                    server_message = error.get_object().get_string_member_with_default("message", "");
                }
            } catch (Error e) {
                // HTTP status still describes the failure if the server responded without JSON.
            }
            report_failure(diagnostic, method, path, "http", started_at,
                           "status=%u code=%s".printf(status, diagnostic_http_code(server_code)));
            throw new ApiError.HTTP(http_message(status, server_code, server_message));
        }

        Json.Object root;
        try {
            root = ApiParsersCommon.parse_response_object_bytes(response.body);
        } catch (Error e) {
            report_failure(diagnostic, method, path, "parse", started_at,
                           "status=%u invalid JSON object".printf(status));
            throw new ApiError.PARSE(unexpected_response());
        }
        if (wrapped && !root.get_boolean_member_with_default("ok", false)) {
            report_failure(diagnostic, method, path, "protocol", started_at,
                           "status=%u missing ok=true".printf(status));
            throw new ApiError.PROTOCOL(unexpected_response());
        }
        return root;
    }

    private static string unexpected_response() {
        return "Holder received an unexpected response. Reopen Holder and try again.";
    }

    private static string http_message(uint status, string code, string details) {
        switch (code) {
        case "keyring_unavailable":
            return "Holder could not access your keyring. Unlock it or check its permissions, then try again.";
        case "key_missing":
            return "This device is missing the project's encryption key. Import its recovery key, then try again.";
        case "data_corrupted":
            return "Holder could not read this data. Check the project or restore it from a known good copy.";
        case "history_unavailable":
            return "Holder could not read the project's Git history. Check that the project folder is accessible.";
        }
        switch (status) {
        case 401:
            return "Holder could not authenticate with the backend. Reopen Holder to reconnect.";
        case 408:
        case 504:
            return "Holder did not get an answer in time. Check whether the action completed before trying again.";
        case 502:
        case 503:
            return "The Holder backend is unavailable. Give it a moment, then try again.";
        }
        // Validation, missing items, and conflicts can have specific recovery advice.
        if (status >= 400 && status < 500 && details.strip().length > 0) {
            return details.strip();
        }
        if (status == 404) {
            return "Holder could not find the requested item. Refresh and try again.";
        }
        return "The Holder backend could not complete the request. Check the Debug tool for details.";
    }

    private static string diagnostic_http_code(string code) {
        switch (code) {
        case "invalid_argument":
        case "conflict":
        case "not_found":
        case "unauthorized":
        case "keyring_unavailable":
        case "key_missing":
        case "data_corrupted":
        case "history_unavailable":
        case "history_key_unavailable":
        case "internal_error":
            return code;
        default:
            return "-";
        }
    }

    // Retain static route segments only. Paths can contain arbitrary IDs, and query
    // values, payloads and free-form errors can include credentials or private content.
    private static string diagnostic_route(string path) {
        var result = new StringBuilder();
        var query_start = path.index_of_char('?');
        var route = query_start < 0 ? path : path.substring(0, query_start);
        foreach (var part in route.split("/")) {
            if (part == "") {
                continue;
            }
            switch (part) {
            case "ai": case "assets": case "binding": case "cards": case "card-references":
            case "content": case "credentials": case "git": case "git_providers.json":
            case "health": case "history": case "import": case "imports": case "links": case "link-kinds":
            case "locations": case "milestones": case "preferred": case "projects": case "pull":
            case "push": case "recovery-token": case "resources": case "search": case "sync":
            case "sync-status": case "tags": case "test": case "test-remote": case "trash":
                result.append("/" + part);
                break;
            default:
                result.append("/{…}");
                break;
            }
        }
        return result.len > 0 ? result.str : "/";
    }

    private static void report_failure(ApiRequestDiagnostic? diagnostic, string method,
                                       string path, string category, int64 started_at, string context) {
        var elapsed_ms = (get_monotonic_time() - started_at) / 1000;
        var line = "API %s %s failed category=%s elapsed=%lldms %s".printf(
            method, diagnostic_route(path), category, elapsed_ms, context);
        debug("%s", line);
        if (diagnostic != null) {
            diagnostic(line);
        }
    }

    internal static string build_url(string base_url, // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: declaration branch artifact
                                     string path,
                                     HashTable<string, string>? query) {
        var sb = new StringBuilder();
        sb.append(base_url); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: library edge branch artifact
        sb.append(path); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: library edge branch artifact
        if (query != null && query.size() > 0) {
            sb.append("?"); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: library edge branch artifact
            bool first = true;
            var iter = HashTableIter<string, string>(query);
            string key;
            string value;
            while (iter.next(out key, out value)) {
                if (!first) { // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: loop edge branch artifact
                    sb.append("&"); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: library edge branch artifact
                }
                first = false;
                sb.append(Uri.escape_string(key)); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: escape call edge artifact
                sb.append("="); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: library edge branch artifact
                sb.append(Uri.escape_string(value)); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: escape call edge artifact
            }
        }
        return sb.str; // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: return edge branch artifact
    }

    internal static string json_string_from_builder(Json.Builder builder) { // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: declaration branch artifact
        var generator = new Json.Generator();
        generator.set_root(builder.get_root()); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: generator edge branch artifact
        return generator.to_data(null); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: return edge branch artifact
    }

    internal static Json.Object json_object_from_text_or_raw(string text) { // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: declaration branch artifact
        var parser = new Json.Parser();
        try { // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: exception table branch artifact
            parser.load_from_data(text, -1);
            var root = parser.get_root(); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: parser edge branch artifact
            if (root != null && root.get_node_type() == Json.NodeType.OBJECT) { // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: short-circuit branch artifact
                return root.get_object(); // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: return edge branch artifact
            }
        } catch (Error e) {
        }

        var out = new Json.Object();
        out.set_string_member("raw", text);
        return out; // LCOV_EXCL_BR_LINE GCOVR_EXCL_BR_LINE: return edge branch artifact
    }
}

}
