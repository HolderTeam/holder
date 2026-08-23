---
---
(() => {
  const catalog = {{ site.data.downloads | jsonify }};
  const releasePageUrl = catalog.release_page;
  const baseUrl = document.documentElement.dataset.baseUrl || "";

  function platformHint() {
    const uaDataPlatform = navigator.userAgentData && navigator.userAgentData.platform;
    return `${uaDataPlatform || ""} ${navigator.platform || ""} ${navigator.userAgent || ""}`.toLowerCase();
  }

  function detectPlatform() {
    const hint = platformHint();

    if (hint.includes("windows") || hint.includes("win32") || hint.includes("win64")) {
      return "windows";
    }

    if (hint.includes("mac") || hint.includes("darwin")) {
      return "macos";
    }

    if (hint.includes("ubuntu")) {
      return "ubuntu";
    }

    if (hint.includes("linux") || hint.includes("x11")) {
      return "linux";
    }

    return "unknown";
  }

  function localUrl(path) {
    return `${baseUrl}${path}`;
  }

  function downloadUrl(platform) {
    if (!platform) return releasePageUrl;
    return platform.local ? localUrl(platform.url) : platform.url;
  }

  function setDownloadState(platformKey) {
    const platform = catalog.platforms[platformKey] || null;
    const platformName = platform?.label || "your platform";

    document.querySelectorAll("[data-download-primary]").forEach((link) => {
      link.href = downloadUrl(platform);
      link.textContent = platform?.button_label || "View downloads";
    });

    document.querySelectorAll("[data-download-platform]").forEach((node) => {
      node.textContent = platformName;
    });

    document.querySelectorAll("[data-download-detail]").forEach((node) => {
      node.textContent = platform?.detail || "Choose a platform below to see the available builds.";
    });

    document.querySelectorAll("[data-platform-card]").forEach((card) => {
      card.classList.toggle("is-current", card.dataset.platformCard === platformKey);
    });

    Object.entries(catalog.platforms).forEach(([key, download]) => {
      document.querySelectorAll(`[data-download-asset="${key}"]`).forEach((link) => {
        link.href = downloadUrl(download);
      });
    });
  }

  setDownloadState(detectPlatform());
})();
