# Pixel Reader Fork for Miyoo Mini Plus / OnionOS

## Base project

- Fork `ealang/pixel-reader`.
- Do not rewrite the app from scratch.
- Preserve existing Pixel Reader behavior wherever possible.
- Add features incrementally.
- Target first: improve EPUB experience.
- FB2 support is **not** part of the current scope.

---

## Target platforms

### 1. Primary target

- Miyoo Mini Plus
- OnionOS
- 640x480 screen
- Button-only navigation
- App should remain integrated into OnionOS like the current Pixel Reader.

### 2. Optional development/test target

- Local SDL desktop build may be used if it helps development.
- Desktop build is not a release requirement.
- Device behavior on Miyoo Mini Plus / OnionOS is authoritative.

---

## Core principle

Pixel Reader remains the reading engine.

We add a better library/home screen and better EPUB rendering on top of the existing codebase.

---

## Important rules

- Before changing state/progress/book-position logic, inspect the current implementation.
- Do not invent a parallel reading-position system if Pixel Reader already has one.
- New library metadata must synchronize with existing Pixel Reader state.
- Do not hardcode Miyoo-only paths into core logic.
- Keep platform-specific paths behind configuration.
- Prefer testing on device or in the existing Miyoo build/package flow before considering a feature done.

---

# Roadmap

1. Build setup, Miyoo build/package flow, and codebase inspection
2. EPUB Library Home Screen
3. Library metadata and per-book state synchronization
4. EPUB rich text rendering and font fixes
5. Later: EPUB covers and visual polish
6. Later: FB2 support, not now

---

# 0. Build Setup, Miyoo Build, Packaging, and Codebase Inspection

## Goal

Make sure original Pixel Reader builds and runs before adding features.

## Required build targets

### Miyoo / OnionOS build

- Must run on Miyoo Mini Plus.
- Must preserve current OnionOS-style packaging.
- Must use existing Pixel Reader paths where appropriate.
- Must run fullscreen on the device.
- Must use physical Miyoo controls.

---

## Tasks

- Fork `ealang/pixel-reader`.
- Clone the fork locally.
- Build the original project for Miyoo Mini Plus / OnionOS.
- Package it in the same way the original Pixel Reader is packaged.
- Copy it to SD card and confirm it launches on the device.
- Only after this start feature work.

---

## Codebase inspection

Inspect and document:

- App entry point
- Renderer
- Input handling
- Platform-specific input mapping
- File explorer
- EPUB parser
- TXT parser
- Book/document model
- Page/layout model
- Progress calculation
- Current reading-position saving/loading
- Where app settings are stored
- How recently opened files are handled, if present
- How OnionOS app packaging currently works
- Which paths are platform-specific
- Which code should stay platform-independent

---

## Beginner-friendly setup and build manual

Create a complete beginner-friendly setup and build manual.

The manual must be written as if the reader has never built a Miyoo Mini / OnionOS app before.

### Required manual sections

#### 1. Prerequisites

Include:

- What OS is assumed
- What tools are needed
- Git installation
- Docker installation if required
- Any package manager steps
- Any required SDK/toolchain
- Any required environment variables

#### 2. Repository setup

Include:

- How to fork the project
- How to clone the fork
- How to initialize submodules if the project uses them
- How to verify the folder structure is correct

#### 3. Miyoo / OnionOS cross-compilation

Include:

- Exact commands to build for Miyoo Mini Plus
- Which toolchain is used
- How Docker is used, if applicable
- How source files are mounted into Docker
- How to clean and rebuild
- What binary/output files should appear after successful build

#### 4. Packaging for OnionOS

Include:

- Exact folder structure on the SD card
- Where `launch.sh` goes
- Where `config.json` goes, if used
- Where `icon.png` goes
- Where the app binary goes
- Where books should be placed
- Where saves/config/cache should be placed
- Required file permissions
- Required Unix line endings for shell scripts

#### 5. Installing on the device

Include:

- How to copy files to the SD card
- How to safely eject the SD card
- How to launch the app from OnionOS
- How to refresh Apps/Package Manager if the app does not appear
- How to test with a sample EPUB

#### 6. Debugging on device

Include:

- Where logs should be written
- How `launch.sh` should redirect stdout/stderr to a log file
- How to inspect logs after a crash
- Common launch failures:
  - binary not executable
  - wrong architecture
  - missing shared libraries
  - bad line endings in `launch.sh`
  - incorrect working directory
  - missing font files
  - missing save/cache folders

#### 7. Build troubleshooting

Include:

- Common compiler errors
- Missing dependency errors
- Permission issues
- SDL / SDL_ttf linking issues
- libxml / libzip issues
- How to verify the produced binary architecture

#### 8. Development workflow

Include:

- Make a small change
- Build Miyoo version
- Copy to SD
- Test on device
- Read logs
- Repeat

#### 9. Clean rebuild and reset

Include:

- How to clean build artifacts
- How to delete generated saves/cache safely on Miyoo
- How to restore original Pixel Reader behavior
- How to recover if the app stops launching

---

## Manual requirements

- Use exact shell commands.
- Prefer copy-pasteable commands.
- Explain what each command does in simple language.
- Include expected outputs or files after each major step.
- Do not skip “obvious” steps.
- Do not say “install dependencies” without listing them.
- Do not say “build normally”; provide exact commands.

---

## Step 0 return format

Return a short project map:

- Important files
- Important classes/functions
- Current state persistence mechanism
- Miyoo build approach
- Best extension points for Library screen
- Best extension points for rich text rendering

---

## Step 0 success criteria

- Original Pixel Reader launches on Miyoo Mini Plus.
- Existing EPUB/TXT reading still works on the device.
- Existing controls still work on Miyoo.
- Current state/progress implementation is understood.
- Beginner-friendly build/deploy/debug manual is complete and copy-pasteable.

---

## Step 0 do not

- Do not add Library before both original builds work.
- Do not add FB2.
- Do not replace the existing reader engine.
- Do not redesign renderer/input before understanding current architecture.
- Do not hardcode Miyoo-only paths into shared/core code.

---

# 1. EPUB Library Home Screen

## Feature

Add a Library screen as the default home screen.

## Current behavior

Pixel Reader starts from a file/tree explorer.

## Desired behavior

On launch, show a Library screen.

## Required

- Must work on Miyoo / OnionOS build.
- Must use platform-aware paths.

---

## Empty library behavior

If library is empty:

- Show empty state.
- Show `+ Add books`.
- Pressing A opens the add flow.
- Pressing Select opens the library menu.

---

## Non-empty library behavior

If library has books, show a vertical list of books.

Each row should show:

- Author
- Title
- Progress/status on the right

---

## Progress/status display

| Progress | Display |
|---|---|
| `0` | `NEW` |
| `> 0` and `< 98` | `37%` style progress |
| `>= 98` | `DONE` |

---

## Default sorting

Sort by:

1. `last_opened_at` descending
2. `added_at` descending
3. `title` ascending as fallback

---

## Library row target layout

- Screen: `640x480`
- Header: about `32 px`
- Footer/help bar: about `24-28 px`
- Row height: about `72-76 px`
- Visible rows: about `5`

---

## Controls on Library screen

| Control | Action |
|---|---|
| Up/Down | Select book |
| Left/Right | Page through list |
| A | Open selected book |
| B | Back/exit according to existing app behavior |
| Select | Open library menu |
| Menu | Exit to OnionOS / close window according to platform |

---

## Library menu initial version

- Add to library
- Search

Search can be a placeholder or disabled initially.

Do not implement complex search yet.

---

## Add to library flow

When user selects `Add to library`, show:

- Add file
- Add folder

### Add file

- Open existing file explorer in file-pick mode.
- User selects one supported book file.
- App extracts metadata.
- App adds it to `library.json`.
- App returns to Library screen.

### Add folder

- Open existing file explorer in folder-pick mode.
- User navigates directories.
- A enters folder.
- Select confirms current folder.
- App scans selected folder for supported files.
- Recursive scanning is optional; choose the simplest safe implementation.
- App adds supported files to `library.json`.
- App returns to Library screen.

---

## Supported formats for current scope

- `.epub`
- `.txt`

## Not in current scope

- `.fb2`
- `.fb2.zip`

---

## Important

Keep the old file explorer.

Do not delete old functionality.

Refactor it into FilePicker mode if needed.

---

## Suggested app modes

- `Library`
- `Reader`
- `FilePickerFile`
- `FilePickerFolder`
- `PopupMenu`

---

## Step 1 success criteria

- App launches into Library screen on Miyoo.
- Empty library state works.
- User can add one EPUB file.
- User can add a folder with EPUB files.
- Added books appear in Library.
- Opening a book from Library uses the existing reader flow.
- Progress updates after reading.
- Library sorts by last opened.

---

## Step 1 do not

- Do not implement covers in Library MVP, not even placeholders.
- Do not implement complex search in v1.
- Do not implement FB2.
- Do not copy books into the app folder.
- Do not remove the old file explorer.

---

# 2. Library Metadata and Per-Book State Synchronization

## Goal

Store library metadata cleanly while staying compatible with existing Pixel Reader progress/state logic.

---

## Important

First inspect how Pixel Reader currently saves:

- Current book path
- Current page/location
- Progress
- Recent files
- Settings
- Any per-book state

Then adapt the design below to synchronize with that existing mechanism.

---

## Storage model

Use:

- One global `library.json`
- One per-book state JSON file for each opened/added book

Suggested structure:

```text
saves/
  library.json
  book-state/
    <book_id>.json
```

---

## Platform paths

### On Miyoo/OnionOS

- Use existing Pixel Reader / OnionOS paths where appropriate.
- Do not hardcode these paths into shared/core code.
- Use a platform/path configuration layer if needed.

---

## Storage rules

- Do not use SQLite.
- Do not copy book files.
- The library is an index of existing files.

---

## `library.json` purpose

Fast index for drawing the Library screen.

---

## `book-state/<book_id>.json` purpose

Detailed state for a single book.

---

## Suggested `library.json` schema

```json
{
  "version": 1,
  "books": [
    {
      "id": "a1b2c3d4",
      "path": "/mnt/SDCARD/Media/Books/book.epub",
      "title": "Book Title",
      "author": "Author Name",
      "format": "epub",

      "progress": 37.4,
      "status": "reading",

      "added_at": 1730000000,
      "last_opened_at": 1730100000,

      "file_size": 1234567,
      "last_modified": 1729000000
    }
  ]
}
```

---

## Suggested `book-state/<book_id>.json` schema

```json
{
  "version": 1,
  "book_id": "a1b2c3d4",
  "path": "/mnt/SDCARD/Media/Books/book.epub",

  "progress": 37.4,
  "status": "reading",

  "current_chapter": 4,
  "current_page": 128,
  "total_pages": 301,

  "reader_location": {
    "type": "existing-pixel-reader-state",
    "value": "use-current-reader-location-format-if-available"
  },

  "last_opened_at": 1730100000,
  "last_saved_at": 1730100200
}
```

---

## Critical synchronization rule

If Pixel Reader already has a save format for reading position, reuse it or wrap it.

Do not break compatibility with existing saved progress.

---

## Preferred behavior

- Existing reader state remains authoritative for exact reading position.
- `book-state/<book_id>.json` may store a reference/copy of that exact location.
- `library.json` stores quick UI fields only:
  - `progress`
  - `status`
  - `last_opened_at`

---

## On add book

- Generate stable `book_id`.
- Extract EPUB metadata once.
- Store `title`, `author`, `path`, `format`, `file_size`, `last_modified` in `library.json`.
- Create book-state file only when needed, or create it immediately with default progress `0`.

---

## Stable ID rule

Use a stable ID based on normalized path plus file identity.

Acceptable:

```text
hash(normalized_path + file_size + last_modified)
```

Do not use only title/author because duplicates exist.

Do not use random ID if it makes re-scanning create duplicates.

---

## On launch

- Load `library.json`.
- Verify that listed files still exist.
- Do not parse all EPUB files on every launch.
- Show missing status if file path is gone.
- Sort by `last_opened_at` descending.

---

## On opening a book from Library

- Load exact reading position using current Pixel Reader mechanism.
- If new book-state exists, synchronize it.
- Open through existing reader flow.
- Update `last_opened_at` in `library.json`.

---

## On saving progress

- Save exact reading position using existing Pixel Reader state logic.
- Save detailed per-book state in `book-state/<book_id>.json` if needed.
- Update quick fields in `library.json`:
  - `progress`
  - `status`
  - `last_opened_at`

---

## Status rules

| Condition | Status |
|---|---|
| `progress <= 0` | `new` |
| `progress > 0` and `progress < 98` | `reading` |
| `progress >= 98` | `finished` |
| File missing | `missing` |
| Parse/open error | `error` |

---

## Best practice

Do not read every book-state JSON when drawing Library if `library.json` already has enough quick fields.

The Library screen should be fast.

---

## Metadata extraction from EPUB

EPUB is a ZIP.

Find:

- `META-INF/container.xml`
- OPF package file

Extract:

- `dc:title`
- `dc:creator`
- `language` if easy
- cover reference later, outside the Library MVP

---

## Fallback

- If title missing, use filename without extension.
- If author missing, use empty string or `Unknown`.

---

## Step 2 success criteria

- `library.json` stores quick metadata.
- Each opened book can have its own book-state JSON.
- Progress shown in Library matches actual reading progress.
- Reopening book resumes at correct position.
- No duplicate entries when adding the same file twice.
- Missing files are handled gracefully.
- Same logic works on Miyoo with platform-specific paths.

---

## Step 2 do not

- Do not rewrite existing progress logic blindly.
- Do not parse every EPUB on every launch.
- Do not store all detailed state only in `library.json`.
- Do not copy book files.
- Do not create a database.
- Do not hardcode Miyoo paths into shared/core logic.

---

# 3. EPUB Rich Text Rendering and Font Fixes

## Feature

Improve EPUB rendering with bold, italic, bold italic, and font fixes.

---

## Goal

EPUB should look more like a real book, not plain text.

---

## Scope

- Add styled text runs.
- Add `FontManager`.
- Render mixed styles in the same paragraph.
- Keep existing EPUB/TXT support working.
- Fix Cyrillic/UTF-8/font rendering issues.
- Must be testable on the target Miyoo build before it is considered done.

---

## Important

Before implementing, inspect current document model and renderer.

If Pixel Reader currently flattens EPUB into plain text too early, identify the smallest safe refactor.

---

## Document model target

Instead of treating a paragraph only as a plain string, introduce styled text runs.

Suggested structures:

```cpp
struct TextRun {
    std::string text;
    bool bold = false;
    bool italic = false;
};

struct Paragraph {
    std::vector<TextRun> runs;
};
```

---

## Nested styles

Bold + italic should combine.

Example:

```html
<strong><em>text</em></strong>
```

should become:

```cpp
bold = true;
italic = true;
```

---

## Font files

Use real font files instead of fake bold/italic if possible.

Suggested font folder:

```text
fonts/
  NotoSerif-Regular.ttf
  NotoSerif-Bold.ttf
  NotoSerif-Italic.ttf
  NotoSerif-BoldItalic.ttf
```

Alternative:

```text
fonts/
  Literata-Regular.ttf
  Literata-Bold.ttf
  Literata-Italic.ttf
  Literata-BoldItalic.ttf
```

---

## Font requirements

- Font must support Cyrillic.
- Font must support quotes, em dash, en dash, ellipsis.
- Font loading must work on Miyoo.
- Do not rely on system fonts that may not exist on Miyoo.
- Bundle required fonts with the app if licenses allow.

---

## `FontManager`

Add a `FontManager` module.

Responsibilities:

- Load regular font.
- Load bold font.
- Load italic font.
- Load bold italic font.
- Return correct font by style.
- Gracefully fallback if a style font is missing.

Suggested API:

```cpp
class FontManager {
public:
    bool load(const std::string& fontDir, int size);
    TTF_Font* get(bool bold, bool italic, int size);
};
```

Fallback:

```text
bold italic -> bold -> italic -> regular
bold        -> regular
italic      -> regular
```

---

## EPUB parsing

Map:

| EPUB tag | Style |
|---|---|
| `<b>` | bold |
| `<strong>` | bold |
| `<i>` | italic |
| `<em>` | italic |

---

## TXT behavior

TXT remains regular text only.

---

## Layout

Line wrapping must measure text using the correct font for each `TextRun`.

Wrong:

```text
Measure all text with regular font and render some parts bold later.
```

Correct:

```text
Measure each run with its actual font.
Preserve styles after wrapping.
Lines should contain styled segments.
```

---

## Renderer

- Render each line segment using the correct font.
- Keep baseline alignment stable.
- Avoid vertical jumps between regular/bold/italic.
- Use stable line height based on max font line skip.

---

## UTF-8/font fixes

- Use UTF-8 rendering APIs.
- Normalize non-breaking spaces to normal spaces.
- Decode or preserve common XML/HTML entities correctly:
  - `&amp;`
  - `&lt;`
  - `&gt;`
  - `&quot;`
  - `&apos;`
- Avoid broken Cyrillic.
- Avoid broken punctuation:
  - `« »`
  - `“ ”`
  - `—`
  - `–`
  - `…`

---

## Whitespace

- Collapse repeated spaces inside normal paragraphs.
- Preserve paragraph breaks.
- Do not implement complex hyphenation yet.
- Word wrap by spaces for v1.

---

## Test text

Use a test EPUB/page containing:

- Regular text
- Bold text
- Italic text
- Bold italic text
- Cyrillic:

```text
Привет, мир! Ёжик, подъезд, съешь ещё этих мягких французских булок.
```

- Punctuation:

```text
Кавычки: «ёлка», “quote”, тире —, многоточие …
```

---

## Step 3 success criteria

- Regular, bold, italic, and bold italic render correctly on Miyoo.
- Cyrillic renders correctly.
- Mixed-style paragraph wraps correctly.
- No broken characters.
- Existing EPUB/TXT reading still works.
- Existing controls still work.
- Existing saved progress still works.

---

## Step 3 do not

- Do not implement full CSS.
- Do not implement colors.
- Do not implement underline.
- Do not implement links.
- Do not implement footnotes.
- Do not implement tables.
- Do not implement full EPUB styling.
- Do not implement hyphenation in this step.

---

# 4. EPUB Covers and Visual Polish

## Goal

Improve Library visuals after the Library MVP is stable.

---

## Tasks

- Extract EPUB cover image.
- Resize/cache cover in low resolution.
- Show cached cover in Library rows.
- Keep placeholder if cover is missing or extraction fails.
- Add error screens:
  - file not found
  - unsupported format
  - parse failed
  - empty library
- Add remove-from-library action.
- Add rescan folder action later if needed.
- Ensure safe save/sync on exit.
- Test all visual changes on Miyoo.

---

## Cover cache

```text
cache/
  covers/
    <book_id>.png
```

---

## Cover behavior

- Extract cover once when adding book.
- Store low-res cached image.
- Do not extract covers on every launch.
- If cache missing, regenerate lazily if possible.

---

## Step 4 success criteria

- Library remains fast.
- Covers appear for EPUB files that have them.
- Missing covers use placeholder.
- Removing book from library does not delete the actual book file unless explicitly implemented later.
- Works on Miyoo.

---

## Step 4 do not

- Do not make cover extraction required for adding a book.
- Do not block library loading because of cover errors.

---

# 5. Later / Out of Current Scope

Not now:

- FB2 support
- FB2.zip support
- Android port
- Footnotes
- Annotations
- Sync/cloud
- Full-text search
- Advanced CSS
- Hyphenation
- Dictionary
- Text-to-speech

---

# First Codex Task

Do only Step 0 first.

## Task

Inspect the forked Pixel Reader project and report:

1. How to build the original project.
2. How to package it for Miyoo Mini Plus / OnionOS.
3. Where the app entry point is.
4. Where rendering happens.
5. Where input/buttons are handled.
6. Where the file explorer is implemented.
7. Where EPUB/TXT parsing happens.
8. Where page layout happens.
9. Where progress/current position is calculated.
10. Where progress/current position is saved and loaded.
11. What existing save/state files look like.
12. Where a Library screen should be integrated.
13. What minimal changes are needed for the Library feature.
14. What risks exist for rich text rendering.
15. Write a complete beginner-friendly setup, build, packaging, install, and debugging manual.

---

## Manual must include

- Git setup
- Docker/toolchain setup
- Miyoo cross-compilation
- OnionOS packaging
- SD card install
- Launch testing
- Logging/debugging on Miyoo
- Common errors and fixes
- Clean rebuild/reset workflow

---

## Manual requirements

- Use exact shell commands.
- Prefer copy-pasteable commands.
- Explain what each command does in simple language.
- Include expected outputs or files after each major step.
- Do not skip “obvious” steps.
- Do not say “install dependencies” without listing them.
- Do not say “build normally”; provide exact commands.

---

## First task restrictions

- Do not change code yet.
- Do not implement features yet.
- Do not add FB2.

---

## First task return format

Return:

- Project map
- Miyoo build/package plan
- Current state/progress analysis
- Beginner-friendly build/package/install/debug manual
- Recommended next steps

---

# Second Codex Task

After Step 0 succeeds:

Implement only the Library Home Screen MVP.

## Scope

- Keep Miyoo build working.
- Add Library screen.
- Add `library.json`.
- Add per-book state file structure only if needed.
- Reuse existing reading-position system.
- Add file/folder to library using existing explorer.
- Show author, title, progress/status.
- Sort by `last_opened_at` descending.
- Open EPUB/TXT from library using existing reader flow.

## Do not

- Do not add covers.
- Do not add FB2.
- Do not add rich text.
- Do not rewrite reader engine.
- Do not break existing file explorer.
- Do not hardcode Miyoo-only paths into shared logic.

---

# Third Codex Task

After Library MVP works:

Implement EPUB rich text rendering and font fixes.

## Scope

- Keep Miyoo build working.
- Add `TextRun` / `Paragraph` model if needed.
- Add `FontManager`.
- Support bold, italic, bold italic.
- Map EPUB `b` / `strong` / `i` / `em`.
- Fix Cyrillic and UTF-8 rendering issues.
- Keep EPUB/TXT and saved progress working.

## Do not

- Do not implement full CSS.
- Do not add FB2.
- Do not add footnotes.
- Do not break current controls.
