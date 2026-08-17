(() => {
  const releaseApiUrl = "https://api.github.com/repos/HolderTeam/holder-release/releases/tags/holder-v0.1.7";
  const releasePageUrl = "https://github.com/HolderTeam/holder-release/releases/tag/holder-v0.1.7";
  const baseUrl = document.documentElement.dataset.baseUrl || "";
  const fallbackAssets = {
    windows: {
      name: "Holder-windows-0.1.7-dev-Setup.exe",
      browser_download_url: "https://github.com/HolderTeam/holder-release/releases/download/holder-v0.1.7/Holder-windows-0.1.7-dev-Setup.exe"
    },
    macos: {
      name: "Holder-macos-0.1.7.dmg",
      browser_download_url: "https://github.com/HolderTeam/holder-release/releases/download/holder-v0.1.7/Holder-macos-0.1.7.dmg"
    }
  };

  const platformLabels = {
    windows: "Windows",
    macos: "macOS",
    linux: "Linux",
    unknown: "your platform"
  };

  function platformHint() {
    const uaDataPlatform = navigator.userAgentData && navigator.userAgentData.platform;
    return `${uaDataPlatform || ""} ${navigator.platform || ""} ${navigator.userAgent || ""}`.toLowerCase();
  }

  function detectPlatform() {
    const hint = platformHint();

    if (hint.includes("win")) {
      return "windows";
    }

    if (hint.includes("mac") || hint.includes("darwin")) {
      return "macos";
    }

    if (hint.includes("linux") || hint.includes("x11")) {
      return "linux";
    }

    return "unknown";
  }

  function assetScore(platform, name) {
    const lower = name.toLowerCase();

    if (platform === "windows") {
      if (lower.endsWith(".exe") && lower.includes("setup")) return 100;
      if (lower.endsWith(".msi")) return 90;
      if (lower.endsWith(".exe") && lower.includes("windows")) return 80;
    }

    if (platform === "macos") {
      if (lower.endsWith(".dmg") && lower.includes("macos")) return 100;
      if (lower.endsWith(".dmg") && lower.includes("darwin")) return 90;
      if (lower.endsWith(".dmg") && lower.includes("mac")) return 80;
    }

    if (platform === "linux") {
      if (lower.endsWith(".appimage")) return 100;
      if (lower.endsWith(".deb")) return 80;
      if (lower.endsWith(".tar.gz") && lower.includes("linux")) return 60;
    }

    return 0;
  }

  function findBestAsset(platform, assets) {
    return assets
      .map((asset) => ({ asset, score: assetScore(platform, asset.name || "") }))
      .filter((entry) => entry.score > 0)
      .sort((a, b) => b.score - a.score)[0]?.asset || null;
  }

  function localUrl(path) {
    return `${baseUrl}${path}`;
  }

  function setDownloadState(state) {
    const platformName = platformLabels[state.platform] || platformLabels.unknown;
    const linkText = state.asset
      ? `Download for ${platformName}`
      : state.platform === "linux"
        ? "Install on Linux"
        : "View downloads";

    document.querySelectorAll("[data-download-primary]").forEach((link) => {
      link.href = state.url;
      link.textContent = linkText;
      link.removeAttribute("aria-disabled");
    });

    document.querySelectorAll("[data-download-platform]").forEach((node) => {
      node.textContent = platformName;
    });

    document.querySelectorAll("[data-download-detail]").forEach((node) => {
      node.textContent = state.asset
        ? `Recommended file: ${state.asset.name}`
        : "No exact binary was found automatically, so this links to the latest release.";
    });

    document.querySelectorAll("[data-platform-card]").forEach((card) => {
      card.classList.toggle("is-current", card.dataset.platformCard === state.platform);
    });

    if (state.assets) {
      ["windows", "macos"].forEach((assetPlatform) => {
        const asset = findBestAsset(assetPlatform, state.assets);
        document.querySelectorAll(`[data-download-asset="${assetPlatform}"]`).forEach((link) => {
          if (asset) {
            link.href = asset.browser_download_url;
          }
        });
      });
    }
  }

  async function updateDownloadLinks() {
    const platform = detectPlatform();

    setDownloadState({
      platform,
      asset: fallbackAssets[platform] || null,
      url: platform === "linux" ? localUrl("/linux/") : fallbackAssets[platform]?.browser_download_url || releasePageUrl
    });

    try {
      const response = await fetch(releaseApiUrl, {
        headers: { "Accept": "application/vnd.github+json" }
      });

      if (!response.ok) {
        throw new Error(`GitHub release lookup failed: ${response.status}`);
      }

      const release = await response.json();
      const asset = findBestAsset(platform, release.assets || []);

      setDownloadState({
        platform,
        asset,
        assets: release.assets || [],
        url: asset ? asset.browser_download_url : release.html_url || releasePageUrl
      });
    } catch (_error) {
      setDownloadState({
        platform,
        asset: fallbackAssets[platform] || null,
        assets: Object.values(fallbackAssets),
        url: platform === "linux" ? localUrl("/linux/") : fallbackAssets[platform]?.browser_download_url || releasePageUrl
      });
    }
  }

  updateDownloadLinks();
})();
