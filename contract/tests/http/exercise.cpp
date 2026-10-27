// Sends requests through the generated dispatcher (ContractRoutes.h) to the stand-in core in
// tests/c/stub_core.cpp, and prints each response, for tests/test_http.py to compare with
// exercise.expected.
#include "api/generated/ContractRoutes.h"

#include <iostream>
#include <string>

extern "C" holder_context* stub_context_new(void);
extern "C" void stub_context_free(holder_context* context);

namespace {

void send(holder_context& context, const std::string& method, const std::string& path, const std::string& body = "") {
  std::cout << method << ' ' << path;
  if (!body.empty()) std::cout << ' ' << body;
  std::cout << "\n  ";
  const auto response = contract_http::dispatch(method, path, body, context);
  if (!response) {
    std::cout << "not dispatched; allowed:";
    for (const auto allowed : contract_http::allowed_methods(path)) std::cout << ' ' << allowed;
    std::cout << "\n";
    return;
  }
  std::cout << response->status << ' ' << response->operation << ' ' << response->body.dump() << "\n";
}

} // namespace

int main() {
  holder_context* context = stub_context_new();
  send(*context, "POST", "/projects/p1/cards", R"({"title": "Thesis", "content": "Intro #draft"})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": "Child", "parent_card_id": "c1", "sort_key": 2.5,)"
                                               R"( "created_at": -5})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": "Named", "card_id": "mine"})");
  send(*context, "GET", "/projects/p1/cards/c1");
  send(*context, "GET", "/projects/p1/cards/c%31");
  send(*context, "GET", "/projects/p1/cards");
  send(*context, "GET", "/projects/p1/cards?parent=roots&include_child_count=true&include_content=1");
  send(*context, "GET", "/projects/p1/cards?parent=c1&order=updated_desc&limit=1");
  send(*context, "GET", "/projects/p1/cards?limit=1&cursor=1");
  // An integer written with a fraction or an exponent, down to int64's least.
  send(*context, "POST", "/projects/p1/cards", R"({"title": "Whole", "created_at": 691.0, "updated_at": 7e2})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": "Least", "created_at": -9223372036854775808.0})");
  // The card lifecycle.
  send(*context, "PATCH", "/projects/p1/cards/c1", R"({"title": "Thesis, revised", "parent_card_id": "c2"})");
  send(*context, "PATCH", "/projects/p1/cards/c1", R"({"parent_card_id": null})");
  send(*context, "PATCH", "/projects/p1/cards/c1", R"({"title": ""})");
  send(*context, "POST", "/projects/p1/cards/c1/move", R"({"intent": "into", "target_card_id": "c1"})");
  send(*context, "POST", "/projects/p1/cards/c1/move", R"({"intent": "sideways"})");
  send(*context, "POST", "/projects/p1/cards/c1/move", R"({"intent": "to_end", "project_id": "p1"})");
  send(*context, "DELETE", "/projects/p1/cards/c1?children=trash_subtree");
  send(*context, "DELETE", "/projects/p1/cards/c1?children=everything");
  send(*context, "POST", "/projects/p1/cards/c1/restore");
  send(*context, "POST", "/projects/p1/cards/c1/restore");
  // Tags.
  send(*context, "PUT", "/projects/p1/cards/c1/tags/idea");
  send(*context, "PUT", "/projects/p1/cards/c1/tags/idea");
  send(*context, "PUT", "/projects/p1/cards/c1/tags/a%2Fb");
  send(*context, "DELETE", "/projects/p1/cards/c1/tags/idea");
  send(*context, "DELETE", "/projects/p1/cards/c1/tags/absent");
  send(*context, "PUT", "/projects/p1/cards/c1/tags/%23idea");
  send(*context, "PUT", "/projects/p1/cards/c1/tags/");
  send(*context, "GET", "/projects/p1/tags");
  send(*context, "GET", "/projects/p2/tags");
  // Links.
  send(*context, "POST", "/projects/p1/cards/c1/links",
       R"({"target_type": "card", "target_id": "c2", "target_project_id": "p9", "label": null})");
  send(*context, "POST", "/projects/p1/cards/c1/links", R"({"target_type": "page", "target_id": "c2"})");
  send(*context, "POST", "/projects/p1/cards/c1/links", R"({"target_type": "card", "target_id": "c2", "kind": ""})");
  send(*context, "GET", "/projects/p1/cards/c1/links?include_deleted=true");
  send(*context, "GET", "/projects/p1/cards/c1/backlinks?scope=everywhere");
  send(*context, "DELETE", "/projects/p1/cards/c1/links?target_type=card&target_id=c2&target_project_id=p9");
  send(*context, "DELETE", "/projects/p1/cards/c1/links?target_id=c2");
  // Milestones.
  send(*context, "POST", "/projects/p1/cards/c1/milestones",
       R"({"dates": [{"role": "start", "date": "2026-10-16"}, {"role": "due", "datetime": "2026-10-17T09:30:00Z"}]})");
  send(*context, "POST", "/projects/p1/cards/c1/milestones", R"({"dates": [{"role": ""}]})");
  send(*context, "POST", "/projects/p1/cards/c1/milestones", R"({"kind": "release"})");
  send(*context, "GET", "/projects/p1/cards/c1/milestones");
  send(*context, "PATCH", "/projects/p1/cards/c1/milestones/m9", R"({"kind": null})");
  send(*context, "DELETE", "/projects/p1/cards/c1/milestones/m9");
  send(*context, "GET", "/projects/p1/milestones/days?first_day=2026-10-01&last_day=2026-10-31&role=start");
  send(*context, "GET", "/projects/p1/milestones/instants?first_instant=0&last_instant=x");
  send(*context, "GET", "/projects/p1/calendar?first_instant=0&last_instant=1&first_day=2026-10-01");
  // Not found.
  send(*context, "GET", "/projects/p1/cards/c9");
  send(*context, "GET", "/projects/p2/cards/c1");
  send(*context, "PATCH", "/projects/p2/cards/c1", R"({"title": "Elsewhere"})");
  send(*context, "GET", "/projects/p2/cards");
  send(*context, "POST", "/projects/p2/cards", R"({"title": "Lost"})");
  // Invalid arguments.
  send(*context, "POST", "/projects/p1/cards", "");
  send(*context, "POST", "/projects/p1/cards", "{not json");
  send(*context, "POST", "/projects/p1/cards", R"({})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": ""})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": null})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": "x", "sort_key": null})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": "x", "sort_key": "high"})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": "x", "sort_key": 1e999})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": "x", "created_at": 1.5})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": "x", "created_at": 9223372036854775808.0})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": "x", "created_at": 9223372036854775808})");
  send(*context, "POST", "/projects/p1/cards", R"({"title": "a\u0000b"})");
  send(*context, "POST", "/projects/p1/cards", R"({"project_id": "p1", "title": "x"})");
  send(*context, "POST", "/projects/p1/cards", R"(["title"])");
  send(*context, "GET", "/projects/p1/cards?limit=0");
  send(*context, "GET", "/projects/p1/cards?limit=5001");
  send(*context, "GET", "/projects/p1/cards?limit=ten");
  send(*context, "GET", "/projects/p1/cards?limit=99999999999");
  send(*context, "GET", "/projects/p1/cards?order=title_asc");
  send(*context, "GET", "/projects/p1/cards?parent=");
  send(*context, "GET", "/projects/p1/cards?include_content=yes");
  send(*context, "GET", "/projects/p1/cards?limit=1&limit=2");
  send(*context, "GET", "/projects/p1/cards?tag=%ff");
  send(*context, "GET", "/projects/p1/cards/%zz");
  // Not the contract's.
  send(*context, "PUT", "/projects/p1/cards/c1");
  send(*context, "GET", "/cards/c1");
  send(*context, "GET", "/cards");
  send(*context, "GET", "/nowhere");
  stub_context_free(context);
}
