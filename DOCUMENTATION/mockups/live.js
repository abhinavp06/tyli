// Live reload for the mockups: when this page or theme.css changes on disk, the page
// reloads. Works when served by tools/mockup_server.sh; does nothing when opened as a file.
(() => {
  if (location.protocol === "file:") return;
  const watched = [location.pathname, "theme.css"];
  const seen = {};
  async function check() {
    for (const path of watched) {
      try {
        const response = await fetch(path, { method: "HEAD", cache: "no-store" });
        const stamp = response.headers.get("Last-Modified");
        if (seen[path] && stamp && stamp !== seen[path]) return location.reload();
        seen[path] = stamp;
      } catch { /* server restarting: try again next tick */ }
    }
  }
  check();
  setInterval(check, 1000);
})();

// The page switcher above each window.
document.addEventListener("DOMContentLoaded", () => {
  const pages = [["music.html", "music (albums)"], ["settings-library.html", "settings · library"],
                 ["settings-general.html", "settings · general"]];
  const nav = document.createElement("nav");
  nav.className = "mock-nav";
  nav.innerHTML = '<span>tyli mockups</span>' + pages.map(([href, name]) =>
    `<a href="${href}"${location.pathname.endsWith(href) ? ' class="on"' : ""}>${name}</a>`).join("");
  document.body.prepend(nav);
});
