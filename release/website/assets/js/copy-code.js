const copyIcon = `<svg width="16" height="16" viewBox="0 0 24 24" fill="none"
  stroke="currentColor" stroke-width="2" stroke-linejoin="round" aria-hidden="true">
  <rect x="8" y="8" width="12" height="12" rx="2"/>
  <path d="M16 8V6a2 2 0 0 0-2-2H6a2 2 0 0 0-2 2v8a2 2 0 0 0 2 2h2"/>
</svg>`;

if (navigator.clipboard?.writeText) {
  for (const code of document.querySelectorAll("main pre > code")) {
    const block = code.parentElement;
    const wrapper = document.createElement("div");
    wrapper.className = "copyable-code";

    const button = document.createElement("button");
    button.type = "button";
    button.className = "code-copy-button";
    button.setAttribute("aria-label", "Copy code");
    button.title = "Copy code";
    button.innerHTML = copyIcon;

    const status = document.createElement("span");
    status.className = "code-copy-status";
    status.setAttribute("role", "status");

    let resetFeedback;
    button.addEventListener("click", async () => {
      try {
        await navigator.clipboard.writeText(code.textContent);
        button.textContent = "✓";
        status.textContent = "Copied to clipboard";
      } catch {
        button.textContent = "!";
        status.textContent = "Could not copy to clipboard";
      }

      button.title = status.textContent;
      clearTimeout(resetFeedback);
      resetFeedback = setTimeout(() => {
        button.innerHTML = copyIcon;
        button.title = "Copy code";
        status.textContent = "";
      }, 2000);
    });

    block.replaceWith(wrapper);
    wrapper.append(block, button, status);
  }
}
