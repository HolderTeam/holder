#include <holder/holder_api.h>

int main() {
  holder_arena* arena = holder_arena_new();
  const holder_project_list_item_list* output = nullptr;
  holder_error* error = nullptr;
  const int result = holder_list_projects(
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      HOLDER_LIST_PROJECTS_ORDER_NONE,
      nullptr,
      0,
      false,
      arena,
      &output,
      &error
  );
  const bool valid = result == HOLDER_ERROR_INVALID_ARGUMENT && output == nullptr &&
                     error != nullptr;
  holder_error_destroy(error);
  holder_arena_free(arena);
  return valid ? 0 : 1;
}
