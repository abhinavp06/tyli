# Feature: Theme (v1): OLED black, minimal green

| | |
|---|---|
| Status | **Shipped in `v0.1.0`** (2026-10-02): built in QML from the approved mockups (round 2, trimmed to implemented features), reviewed by the user over VNC and on Windows (the GitHub Actions build) |
| Branch | `abhinavp06/FILE_SCANNER_INIT` (decided, §8) |
| Ships in | TYLI `v0.1.0`, with the File Sync UI |
| hoardor feature | none: UI only |
| Mockup | `DOCUMENTATION/mockups/` (live at `http://localhost:8000` through the SSH tunnel, `tools/mockup_server.sh`): music section, Settings → Folders, Settings → Sync, dialogs, palette and controls. The first draft was an Artifact canvas (https://claude.ai/artifact/JADRbzxMUPA2qoo21fp9D3); the user asked for the mockups in the repo instead, so the repo copy is the one that's kept current |

## 1. Goal

The user's direction (2026-10-02): *"minimal dark + green, hacker-like, but very minimal green accents for text etc. Everything else pure black, something that pops on an OLED screen."*

- **Pure black (`#000000`) everywhere:** window, panels, lists, dialogs, inputs. On OLED, black pixels are off, so the UI looks like it floats on the glass.
- **Green is rare and means something:** what's selected, what's active, what you can act on, what's online. Never large green areas.
- **Text is mostly neutral grey,** with green as the exception. That's what keeps it calm and not a "Matrix" parody.
- **Monospace type** gives the terminal feel.
- **No gradients, shadows, or rounded "cards".** Structure comes from spacing and thin dark hairlines.

**Not in this feature:** artwork or cover images (they come with metadata), animations beyond simple fades, a light theme.

## 2. Palette (one `Theme` singleton; every color comes from it)

| Token | Value | Used for |
|---|---|---|
| `background` | `#000000` | everything |
| `hairline` | `#1A1A1A` | separators, input and list borders (barely visible, enough to show structure) |
| `hover` | `#0D0D0D` | the row or control under the mouse (the only non-black fill) |
| `text` | `#C8C8C8` | normal text |
| `textDim` | `#767676` | secondary text: paths, counts, help lines, defaults |
| `textFaint` | `#3C3C3C` | disabled text, placeholders |
| `accent` | `#39FF6A` | **the green**: the selected item, the active tab, the focused field, primary actions, progress, "online" |
| `accentDim` | `#1E8A3C` | the selection marker's line, a focused border, the hover on accent text |
| `warning` | `#FFB000` | held removals, "settling" (terminal amber) |
| `error` | `#FF4D4D` | errors, failed syncs, destructive confirmations |

- **The green appears only as text, 1–2 px lines, or a small glyph.** Never as a filled background. On OLED that also limits burn-in: nothing bright stays large and static.
- **Contrast:** `text` on black is about 12:1 and `textDim` (`#767676`) about 4.6:1 (WCAG AA for normal text); `accent` on black is about 15.7:1.

## 3. Type

- **One monospace family for the whole app: IBM Plex Mono** (Regular and SemiBold; the user's choice, 2026-10-02), bundled with the app as a Qt resource (SIL Open Font License 1.1). Bundling makes it look the same on Windows, Linux, and macOS. System monospace fonts differ (Consolas, DejaVu Sans Mono, Menlo).
  - **A new dependency (a font asset), approved by the user** (2026-10-02). Rejected: JetBrains Mono (the user preferred Plex), the system monospace (looks different on each OS).
- **Sizes:** 13 px base, 11 px for small dim lines, 18 px for a section title. Lowercase section labels (`folders`, `files in "music (e:)"`).

## 4. Look of each element

```
 tyli                                                     12,408 files · 2 folders
 ───────────────────────────────────────────────────────────────────────────────
 › music          │ music                                              [ sync ]
   movies         │ ■■■■■■■■■■■■■■■■ syncing "music (e:)" · 1/2 · 5,120 files  [ cancel ]
   shows          │
   books          │ folders                  │ files in "music (e:)"
                  │ ● music (e:)     online  │ artist a/album 1/01 intro.flac   34.2 mb  audio
                  │   e:/media/music         │ artist a/album 1/cover.jpg        1.1 mb  image
                  │   8,201 files · 2m ago   │ artist a/album 2/01 song.mp3      8.0 mb  audio  settling
                  │ ○ music (f:)    offline  │
                  │   f:/music               │
   settings       │ last sync 2 min ago: +120 · ~3 · −0 · 2 errors          [ details ]
```

- **Navigation:** a left sidebar replaces the top tabs (decided, §8). Superseded by §4a: icons next to the names, collapsible, and Settings moved to a gear in the top bar.
- **Buttons:** text in brackets, `[ sync ]`. Grey text by default, green on hover and for the main action of a screen. Disabled is faint. There's no button box.
- **Lists:** no zebra stripes and no boxes. The selected row has green text plus a 2 px green line on its left edge. Hovering a row gives `#0D0D0D`.
- **Inputs (text fields, spin boxes, combo boxes):** a black box with a `hairline` border that turns green when focused. A green caret and green selected text.
- **Check boxes:** `[x]` / `[ ]` drawn as text: green `x` when on.
- **Progress:** a 2 px green line. While the scan total is unknown, a short green segment slides along it.
- **Scroll bars:** a 3 px `textFaint` thumb, shown while scrolling or hovering.
- **Dialogs:** black with a 1 px `hairline` border, and the rest of the window dimmed to 60% black behind them.
- **Status dots:** `●` green = online, `○` grey = offline or not synced yet.
- **Warnings** (held removals) are amber text with a 2 px amber left line, not a filled banner.

## 4a. Layout after the user's first review (2026-10-02, mockups round 2)

The user's notes on the first mockups, and what changed:

- **Top bar** across the window: the sidebar toggle and `tyli_` on the left; the **sync status** (a 2 px progress line plus "syncing music (e:) · 5,120 files", or "synced 2 min ago" when idle) and a **gear icon** on the right. Clicking either opens Settings → Library. The word "settings" is gone.
- **Sidebar:** categories only, each with a small stroke icon left of its name and a count on the right. It **collapses** to icons only (the toggle in the top bar).
- **A category in the sidebar shows its media only.** Music shows a list of albums (cover, album, artist, year, tracks, length; search, sort, list or grid). Folders, sync, and file lists move out of the category views into Settings. The album list needs tag reading in hoardor, which doesn't exist yet (§4b).
- **Settings has two tabs, `library` and `general`:**
  - **library:** category **pills** (`all`, one per category with its icon and folder count, and `+`). Under the selected category: its name, accepted kinds, totals, and actions (sync, edit, delete); the sync progress with a stop icon; its folders as rows (status, name, path, files, last sync, marker), each with icon actions (show last sync, move to another category, relocate, remove). A folder's last-sync details and scan errors expand inline under its row. Held removals appear as an amber line under the folder, with remove/keep icons. **Add folder** is an inline row (path field, browse, name, marker toggle, confirm/cancel), so there are no dialogs.
  - **general:** one scrolling page with sections: **appearance** (accent color swatches, the palette, a preview of controls), **scanning** (extensions as chips grouped by kind, plus the names to skip), **sync** (the main tunables, with the rest under "advanced"), and **about** (versions, database path).
- **Text buttons became icons** everywhere, each with a tooltip that says what it does.
- **"case-sensitive" left the folder row:** hoardor detects it when a folder is added (`detect_case_sensitive`), so it's shown only as a detail ("case-insensitive drive (detected)").
- **Settings save as you change them** (validated by hoardor, with the reason shown in red next to an invalid value). Each changed value gets a reset icon. No Save button.
- **Animations stay minimal** (the user, 2026-10-02): the blinking cursor and the progress line only. Deeper UI work comes once the basic features are done, based on daily use.

## 4b. Decisions (round 2, the user, 2026-10-02)

1. **Albums need tags, so metadata comes next, on this same branch** (`abhinavp06/FILE_SCANNER_INIT`): a tag-reading feature in hoardor (its own feature doc, and a library such as TagLib to agree on), then the album view in TYLI.
   - **Superseded (the user, 2026-10-02):** this branch merges as `v0.1.0` without metadata. Next comes media listing on new branches (Music grouped by album *folder*, Movies and Books as lists, Shows by show and season folders), then a first player draft, then metadata (ARCHITECTURE decision log).
2. **Icons: our own simple stroke icons** (left to Claude; the user doesn't mind). They're a handful of 24×24 SVG paths, drawn to match the theme, with no dependency. An icon set can replace them later if the count grows a lot.
3. **The mockups show only what's implemented.** The round-2 pages were committed as the agreed direction (`bc07914`), then trimmed to features that exist; features come back as they're built:
   - **Music** lists the category's files across its folders (file, folder, size, kind, settling; an offline folder's files are dimmed) instead of albums. Albums return with metadata.
   - **Settings → General** has no Appearance section (there's no accent setting), and Scanning shows hoardor's real defaults. Sync shows every hoardor setting, with batch size, batch time, and progress interval under "advanced".
   - The sidebar counts are files per category.
   - A flat list across folders needs a model that pages folder by folder (`Library::entries` per root, in folder order). That's a TYLI-side change; hoardor's API already allows it.

## 5. How it's built

- **`qml/Theme.qml`**: a QML singleton (`pragma Singleton`) holding the palette, the font family, and the sizes. Screens use `Theme.accent`, never a color literal. A future "accent color" setting changes only this file's values.
- **The Qt Quick Controls style changes from Fusion to Basic.** Basic is the style Qt designed for customization: every control's `background` and `contentItem` can be replaced without fighting the style. Fusion draws grey gradients and bevels, which can't be made pure black through the palette alone.
- **Restyled controls in their own files** (`qml/controls/`: `TyButton`, `TyTextField`, `TySpinBox`, `TyComboBox`, `TyCheckBox`, `TyProgressBar`, `TyScrollBar`, `TyDialog`, `SidebarItem`). The screens use them in place of the stock controls. That's one small file per control, not a whole custom style plugin.
- **`ApplicationWindow.palette`** is set from `Theme` as well, so anything left unstyled (menus, tooltips, the folder dialog) is dark rather than white.
- **Font:** `qml/fonts/IBMPlexMono-{Regular,SemiBold}.ttf` in the QML module's resources, loaded with `FontLoader`. Its license file sits next to it.
- **ARCHITECTURE §4 ("Look")** changes from "Fusion, follows the system's light or dark mode" to this theme, with a decision-log row.

## 5a. As built (2026-10-02)

- **`qml/Theme.qml`** (a singleton through `QT_QML_SINGLETON_TYPE`): the palette, font, sizes, and metrics. No color literal appears in any screen.
- **`qml/Icon.qml`**: our stroke icons as SVG path strings drawn with Qt Quick Shapes (`PathSvg`), so an icon's color is just a binding. `Shape.CurveRenderer` antialiases them on the GPU without multisampling.
- **`qml/controls/`**: `TyIconButton`, `TyField`, `TyCheckBox` (`[x]`), `TyMenu` + `TyMenuItem`, `TyPill`, `TyProgress`, `TyScrollBar`, `TyTab`, all built on `QtQuick.Templates` (behavior only, no look). Tabs and pills follow a bound `checked` property instead of toggling themselves.
- **`src/app_setup.cpp`** (`setupApplication()`, used by `main()` and the QML smoke test): the Basic style, the bundled fonts (`qml/fonts/`, loaded from the module's resources), and IBM Plex Mono 13 px as the app's default font.
- **Screens:** `Main.qml` (top bar, sidebar, category view or Settings, status line), `TopBar.qml`, `Sidebar.qml`, `CategoryView.qml`, `SettingsView.qml` → `LibrarySettings.qml` + `GeneralSettings.qml`, `StatusLine.qml`. The old `SectionView`, `SyncDetailsDialog`, and four Settings sub-tab files are gone.
- **No popups:** results and errors go to `StatusLine` at the bottom (results clear after 8 s, errors stay until dismissed). Removal confirmations, adding and relocating folders, and editing categories all happen inline.
- **Category view = `CategoryFilesModel`** (C++), which replaced `FolderFilesModel`: it pages through the category's folders one after another (keyset paging per folder) and reloads by itself when the folders change.
- **The bridge gained `folderReports`** (each folder's latest report, keyed by id) for the inline last-sync details.
- **Category names show in lowercase** (they're labels); folder names, paths, and file names show exactly as they are.
- **Category icons** come from the name when it's a familiar one (music, movies, shows, books, …), otherwise from the accepted kinds (`Format.categoryIcon`).

## 6. Edge cases

| Case | Behavior |
|---|---|
| Windows is in light mode | TYLI stays dark (it doesn't follow the OS) |
| A non-OLED (LCD) screen | Same look. Pure black on LCD is dark grey, which still looks fine |
| A very long name or path | Elided in the middle, as now |
| Keyboard focus | The focused control shows a green border or green text, so keyboard use stays visible |
| Folder dialog (Qt's own on Linux, native on Windows) | Dark through the window palette on Linux. The native Windows dialog follows Windows' own theme |
| Font fails to load | Falls back to the system monospace font (no crash, no blank text) |

## 7. Tests

- The QML smoke test keeps loading every screen with zero QML warnings, and its screenshots are the visual check.
- **New check in the smoke test:** sample pixels of the window background and every panel's background, and fail if any isn't `#000000`.
- A check that `Theme.fontFamily` resolved to the bundled font.

## 8. Decisions (the user, 2026-10-02)

1. **Font: IBM Plex Mono**, bundled. Rejected: JetBrains Mono, the system monospace.
2. **Navigation: left sidebar.** Rejected: top tabs.
3. **Branch: `abhinavp06/FILE_SCANNER_INIT`**, so the theme ships with File Sync in TYLI `v0.1.0` (one merge, and the Windows test happens on the final look). Rejected: a separate `abhinavp06/THEME` branch.
4. **An HTML mockup first:** the user reviews the look in a browser and suggests changes before any QML is written.
