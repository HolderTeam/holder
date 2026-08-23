const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");

const script = fs.readFileSync("_site/assets/js/downloads.js", "utf8");

function classList() {
  const classes = new Set();
  return {
    contains(name) {
      return classes.has(name);
    },
    toggle(name, enabled) {
      if (enabled) classes.add(name);
      else classes.delete(name);
    }
  };
}

function renderFor({ userAgent, platform = "", uaDataPlatform = "", baseUrl = "" }) {
  const primary = {};
  const platformLabel = {};
  const detail = {};
  const cards = ["ubuntu", "linux", "windows", "macos"].map((key) => ({
    dataset: { platformCard: key },
    classList: classList()
  }));
  const assetLinks = Object.fromEntries(
    ["ubuntu", "linux", "windows", "macos"].map((key) => [key, {}])
  );

  const document = {
    documentElement: { dataset: { baseUrl } },
    querySelectorAll(selector) {
      if (selector === "[data-download-primary]") return [primary];
      if (selector === "[data-download-platform]") return [platformLabel];
      if (selector === "[data-download-detail]") return [detail];
      if (selector === "[data-platform-card]") return cards;

      const assetMatch = selector.match(/^\[data-download-asset="([^"]+)"\]$/);
      return assetMatch ? [assetLinks[assetMatch[1]]] : [];
    }
  };

  const navigator = {
    platform,
    userAgent,
    userAgentData: uaDataPlatform ? { platform: uaDataPlatform } : undefined
  };

  vm.runInNewContext(script, { document, navigator });

  return {
    primary,
    platformLabel,
    detail,
    current: cards.find((card) => card.classList.contains("is-current"))?.dataset.platformCard,
    assetLinks
  };
}

const ubuntu = renderFor({
  baseUrl: "/preview",
  platform: "Linux x86_64",
  userAgent: "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:153.0) Gecko/20100101 Firefox/153.0"
});
assert.equal(ubuntu.current, "ubuntu");
assert.equal(ubuntu.platformLabel.textContent, "Ubuntu");
assert.equal(ubuntu.primary.href, "/preview/ubuntu/");
assert.equal(ubuntu.primary.textContent, "Install on Ubuntu");

const linux = renderFor({
  platform: "Linux x86_64",
  userAgent: "Mozilla/5.0 (X11; Fedora; Linux x86_64) AppleWebKit/537.36 Chrome/140.0 Safari/537.36"
});
assert.equal(linux.current, "linux");
assert.equal(linux.platformLabel.textContent, "Linux");
assert.match(linux.primary.href, /Holder-0\.2\.0-dev-x86_64\.AppImage$/);
assert.match(linux.detail.textContent, /0\.2\.0-dev AppImage preview/);

const windows = renderFor({
  platform: "Win32",
  userAgent: "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"
});
assert.equal(windows.current, "windows");
assert.match(windows.primary.href, /Holder-windows-0\.1\.7-dev-Setup\.exe$/);

const macos = renderFor({
  platform: "MacIntel",
  userAgent: "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)"
});
assert.equal(macos.current, "macos");
assert.match(macos.primary.href, /Holder-macos-0\.1\.7\.dmg$/);

const unknown = renderFor({ userAgent: "Holder test browser" });
assert.equal(unknown.current, undefined);
assert.equal(unknown.platformLabel.textContent, "your platform");
assert.equal(
  unknown.primary.href,
  "https://github.com/HolderTeam/holder-release/releases/tag/holder-v0.1.7"
);

console.log("Download detection smoke tests passed.");
