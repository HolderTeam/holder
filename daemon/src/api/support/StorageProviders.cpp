#include "api/support/StorageProviders.h"

#include "resource/LocalDirectoryProvider.h"
#include "storage/S3CompatibleProvider.h"
#include "storage/google/GoogleDriveProvider.h"

namespace holder::api::support {
namespace {

holder::resource::StorageError invalid_configuration(const std::string& message) {
  return {holder::resource::StorageErrorCode::InvalidConfiguration, message};
}

} // namespace

std::unique_ptr<holder::resource::StorageProvider> storage_provider(
    const holder::model::Location& location,
    const holder::resource::LocationBinding& binding
) {
  if (location.provider != binding.provider) {
    throw invalid_configuration("storage declaration and private binding provider mismatch");
  }
  if (location.provider == "local_directory") {
    const auto root = binding.values.find("root_path");
    if (root == binding.values.end() || root->second.empty()) {
      throw invalid_configuration("local directory binding requires root_path");
    }
    return std::make_unique<holder::resource::LocalDirectoryProvider>(root->second);
  }
  if (location.provider == "s3_compatible") {
    auto required_config = [&](const std::string& name) -> std::string {
      const auto value = location.configuration.find(name);
      if (value == location.configuration.end() || value->second.empty()) {
        throw invalid_configuration("S3 location is missing " + name);
      }
      return value->second;
    };
    auto required_secret = [&](const std::string& name) -> std::string {
      const auto value = binding.values.find(name);
      if (value == binding.values.end() || value->second.empty()) {
        throw invalid_configuration("S3 binding is missing " + name);
      }
      return value->second;
    };
    holder::storage::S3CompatibleConfig config;
    config.endpoint = binding.values.contains("endpoint") ? binding.values.at("endpoint")
                                                          : required_config("endpoint");
    config.region = required_config("region");
    config.bucket = required_config("bucket");
    if (location.configuration.contains("addressing_style")) {
      config.addressing_style = location.configuration.at("addressing_style");
    }
    config.allow_insecure_localhost = location.configuration.contains("allow_insecure_localhost") &&
                                      location.configuration.at("allow_insecure_localhost") ==
                                          "true";
    holder::storage::S3Credentials credentials;
    credentials.access_key_id = required_secret("access_key_id");
    credentials.secret_access_key = required_secret("secret_access_key");
    if (binding.values.contains("session_token") && !binding.values.at("session_token").empty()) {
      credentials.session_token = binding.values.at("session_token");
    }
    return std::make_unique<holder::storage::S3CompatibleProvider>(config, credentials);
  }
  if (location.provider == "google-drive") {
    const auto folder_id = location.configuration.find("folder_id");
    if (folder_id == location.configuration.end() || folder_id->second.empty()) {
      throw invalid_configuration("Google Drive location is missing folder_id");
    }
    const auto refresh_token = binding.values.find("refresh_token");
    if (refresh_token == binding.values.end() || refresh_token->second.empty()) {
      throw invalid_configuration("Google Drive binding is missing refresh_token");
    }
    holder::storage::google::GoogleDriveConfig config;
    config.folder_id = folder_id->second;
    holder::storage::google::GoogleDriveCredentials credentials;
    credentials.refresh_token = refresh_token->second;
    return std::make_unique<holder::storage::google::GoogleDriveProvider>(
        config,
        credentials,
        holder::storage::google::google_oauth_client_from_env()
    );
  }
  throw invalid_configuration("unsupported storage provider");
}

std::string location_object_key(
    const holder::model::Location& location,
    const std::string& relative_key
) {
  auto prefix = location.configuration.contains("prefix") ? location.configuration.at("prefix")
                                                          : std::string();
  while (!prefix.empty() && prefix.front() == '/')
    prefix.erase(prefix.begin());
  while (!prefix.empty() && prefix.back() == '/')
    prefix.pop_back();
  return prefix.empty() ? relative_key : prefix + "/" + relative_key;
}

} // namespace holder::api::support
