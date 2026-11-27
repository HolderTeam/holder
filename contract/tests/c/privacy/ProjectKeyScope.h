#pragma once

// Stands in for core's ProjectKeyScope (core/src/privacy/ProjectKeyScope.h) when the generated
// glue is compiled against stub_core.cpp: the glue opens one per C API call.
namespace holder::privacy {

class ProjectKeyScope {
 public:
  ProjectKeyScope() {}
  ~ProjectKeyScope() {}
  ProjectKeyScope(const ProjectKeyScope&) = delete;
  ProjectKeyScope& operator=(const ProjectKeyScope&) = delete;
};

} // namespace holder::privacy
