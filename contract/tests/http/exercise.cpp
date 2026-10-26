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
  send(*context, "POST", "/cards", R"({"project_id": "p1", "title": "Thesis", "content": "Intro #draft"})");
  send(*context, "POST", "/cards", R"({"project_id": "p1", "title": "Child", "parent_card_id": "c1", "sort_key": 2.5,)"
                                   R"( "created_at": -5, "unknown": true})");
  send(*context, "GET", "/cards/c1");
  send(*context, "GET", "/cards/c%31");
  send(*context, "GET", "/projects/p1/cards");
  send(*context, "GET", "/projects/p1/cards?parent=roots&include_child_count=true&include_content=1");
  send(*context, "GET", "/projects/p1/cards?parent=c1&order=updated_desc&limit=1");
  send(*context, "GET", "/projects/p1/cards?limit=1&cursor=1");
  // Not found.
  send(*context, "GET", "/cards/c9");
  send(*context, "GET", "/projects/p2/cards");
  send(*context, "POST", "/cards", R"({"project_id": "p2", "title": "Lost"})");
  // Invalid arguments.
  send(*context, "POST", "/cards", "");
  send(*context, "POST", "/cards", "{not json");
  send(*context, "POST", "/cards", R"({"project_id": "p1"})");
  send(*context, "POST", "/cards", R"({"project_id": "p1", "title": ""})");
  send(*context, "POST", "/cards", R"({"project_id": "p1", "title": null})");
  send(*context, "POST", "/cards", R"({"project_id": "p1", "title": "x", "sort_key": null})");
  send(*context, "POST", "/cards", R"({"project_id": "p1", "title": "x", "sort_key": "high"})");
  send(*context, "POST", "/cards", R"({"project_id": "p1", "title": "x", "sort_key": 1e999})");
  send(*context, "POST", "/cards", R"({"project_id": "p1", "title": "a\u0000b"})");
  send(*context, "POST", "/cards", R"(["project_id"])");
  send(*context, "GET", "/projects/p1/cards?limit=0");
  send(*context, "GET", "/projects/p1/cards?limit=5001");
  send(*context, "GET", "/projects/p1/cards?limit=ten");
  send(*context, "GET", "/projects/p1/cards?limit=99999999999");
  send(*context, "GET", "/projects/p1/cards?order=title_asc");
  send(*context, "GET", "/projects/p1/cards?parent=");
  send(*context, "GET", "/projects/p1/cards?include_content=yes");
  send(*context, "GET", "/projects/p1/cards?limit=1&limit=2");
  send(*context, "GET", "/projects/p1/cards?tag=%ff");
  send(*context, "GET", "/cards/%zz");
  // Not the contract's.
  send(*context, "DELETE", "/cards/c1");
  send(*context, "GET", "/cards");
  send(*context, "GET", "/nowhere");
  stub_context_free(context);
}
