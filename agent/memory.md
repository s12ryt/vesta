# Vesta — 操作記憶 (memory)

> 記錄專案的各種操作（讀取 / 寫入 / 刪除）。由新至舊或依時序追加。

## 2026-10-04 — T-001 專案漢化（簡體中文）

### 讀取 (Read)
- 探索專案頂層結構、`git log`，確認 repo 與版本（tag `release: Vesta 1.1.9`）。
- 讀取在地化核心：`src/render/menu/localization.hpp`、`src/render/menu/localization.cpp`（653 行，讀至尾端公開函式）。
- 讀取消費端：`src/render/menu/layout.cpp`、`widgets.cpp`、`misc_page.cpp`、`visual_editor.cpp`、`src/features/misc/nade_helper.cpp`、`src/render/overlay/hud.cpp`（以 grep 定位 `tr(` 呼叫）。
- 讀取語言設定：`src/config/misc.hpp`（`general_profile.language`）、`src/config/settings.cpp`（`from_json`/`to_json` 的 `language` 處理）。
- 讀取字型系統：`src/render/draw.hpp`（`zdraw` 字型 API）、`src/render/overlay/overlay.hpp`（`loaded_fonts`）、`src/render/overlay/overlay.cpp`（字型初始化區塊）、`src/resources/fonts/notosans_medium.hpp`（無 CJK）。
- 讀取建置：`CMakeLists.txt`（確認 `/utf-8` 與明確來源清單）、`README.md`。

### 寫入 (Write / Edit)
1. `src/render/menu/localization.hpp` — `enum class id` 新增 `zh = 2`；註解改 `"EN" / "RU" / "ZH"`。
2. `src/render/menu/localization.cpp` — 新增 `chinese()` 對照表（~350 筆）與 `table_for(id)`；`code()` 加 `"ZH"`；兩個 `tr()` 改用 `table_for(current())`。另將新增區塊多餘的 4 空格縮排修正，對齊 `russian()` 樣式。
3. `src/render/menu/misc_page.cpp` — 語言下拉加入 `"简体中文"`。
4. `src/render/overlay/overlay.cpp` — 新增 CJK 字型合併區塊（候選 msyh/simhei/simsun/Deng，合併進 notosans_medium_12、esp_text_11、menu_regular_12、menu_semibold_13）。
5. `README.md` — Interface 段落 bullet 加入 Simplified Chinese。
6. 新增 `agent/deep_todos.md`、`agent/項目表.md`、`agent/memory.md`（本檔）。

### 刪除 (Delete)
- 無。

### 驗證 (Verify)
- `git diff --stat`：`src/render/menu/localization.cpp` 599 insertions(+) / 3 deletions(-)，符合預期。
- 檔案完整性：`localization.cpp` 無 BOM、1249→（縮排後仍）1249 行、全 CRLF、檔尾 `\r\n`、55616 bytes。
- 逐行抽查確認 `chinese()` / `table_for()` / `code()` / `tr()` 內容正確。
- **未驗證**：編譯。環境缺 `cmake` / `cl` / `clangd`（僅 `vswhere.exe` 存在）。

### 事故紀錄
- 曾嘗試用 PowerShell `-split "`r`n", -1` 重排縮排；因 `-split` 的 max 參數不當，回傳單一元素，導致索引為 `$null`、輸出「stripped lines: 0」，但 `WriteAllText` 寫回與原文**位元組相同**的內容，經核對 `git diff`、行數、CRLF 後確認**未造成損壞**。
- 後改用 `$text.Split([string[]]@("`r`n"), StringSplitOptions.None)` 並加護欄（行數=1250、錨點存在、起點 4 空格縮排、逐行前綴檢查），成功去除 567 行各 4 空格，`bad=0`。

## 2026-10-04 — T-002 新增繁體中文（zh_hant）

### 讀取 (Read)
- grep 確認 `ImFontGlyphRangesBuilder` 存在於 `src/external/imgui_vendor/imgui.h`（struct @3559、`AddText` @imgui_draw.cpp:5016）。
- grep 確認 `localization::code()` **零呼叫點**。
- 讀取 `overlay.cpp` 前 45 行 include，確認已 `#include "imgui.h"` 與 `#include <render/menu/localization.hpp>`。
- 讀取 `localization.cpp` 600-1214 行（完整 `chinese()` 表、`table_for`、`set`）與 1207-1249 行（`code()`/`tr()` 尾段）；讀取 `localization.hpp` 全文（27 行）。

### 寫入 (Write / Edit)
1. `src/render/menu/localization.hpp` — 新增 `zh_hant = 3`；註解加 `"TW"`；宣告 `cjk_glyph_text()`。
2. `src/render/menu/localization.cpp` — 加 `#include <string>`；新增 `traditional()`（~350 筆繁中對照）；`table_for` 支援 `zh_hant`；`code()` 加 `"TW"`；新增 `cjk_glyph_text()`。
3. `src/render/menu/misc_page.cpp` — 下拉加入 `"繁體中文"`。
4. `src/render/overlay/overlay.cpp` — 候選加 `msjh.ttc`；改以 `ImFontGlyphRangesBuilder`（`cjk_glyph_text()` + `GetGlyphRangesChineseSimplifiedCommon()`）建構精準字形範圍；警告文字改為 `no Chinese font found…`。
5. `README.md` — bullet 加入 Traditional Chinese。
6. 更新 `agent/deep_todos.md`（新增 T-002）、`agent/項目表.md`（依賴圖/語言表/CJK 說明）、`agent/memory.md`（本節）。

### 刪除 (Delete)
- 無。

### 驗證 (Verify)
- 待補：`git diff --stat` 覆核、編譯（環境缺 `cmake`/`cl`/`clangd`，同 T-001）。

## 2026-10-04 — T-003 提交並推送至 Operator 帳號倉庫

### 讀取 (Read)
- `git remote -v`：`origin` = `https://github.com/Read1dno/vesta.git`；分支 `main`；`git log` HEAD `466c634 release: Vesta 1.1.9`。
- `gh auth status`：已登入 `s12ryt`（scopes: gist, read:org, repo, workflow）。
- `gh repo view s12ryt/vesta`：PUBLIC fork，父層 `Read1dno/vesta`，預設分支 `main`。

### 寫入 (Write / Edit)
1. `git commit`（`git add -A` 全部變更）→ commit `513eae8`「feat(i18n): add Simplified and Traditional Chinese interface」，8 files changed、1557 insertions(+)、7 deletions(-)。
2. 新增 git 遠端 `myfork` = `https://github.com/s12ryt/vesta.git`。
3. `git push myfork main` → `29bf273..513eae8  main -> main`（成功 fast-forward）。
4. 更新 `agent/deep_todos.md`（新增 T-003）、`agent/memory.md`（本節）。

### 刪除 (Delete)
- 無。

### 驗證 (Verify)
- `gh api repos/s12ryt/vesta/commits/main`：SHA = `513eae84dc1c674f10e716a1ee8e5f234d2955ce`。
- `git rev-list --left-right --count myfork/main...origin/main` = `1  0`（本地相對上游 ahead 1）。
- `git status -sb`：`## main...origin/main [ahead 1]`。
- fork 分支列表：`main`、`feature/zh-cn-zh-tw-localization`（後者非本次產生）。

### 備註
- commit message 輕微錯字 `English/RУсский`（已推送，不重寫歷史）。
- 仍未編譯驗證。

## 2026-10-04 — T-004 修復 GitHub Actions Release workflow

### 讀取 (Read)
- `gh run list`：Build run `#37141266795`（workflow_dispatch）SUCCESS，artifact `vesta-windows-x64` 約 2.5MB；三個 Release run（`#37141683112` v1.1.9、`#37142225368` v1.1.9-s12ryt、`#37143008695` v1.1.9-s12rytCE）全 FAILURE。
- `gh run view --log`：確認失敗訊息為 immutable release 相關（前兩者「already exists and is published」，第三者「published … without immutable protection」）。
- `gh api repos/s12ryt/vesta/immutable-releases` → `{"enabled":false,"enforced_by_owner":false}`（fork 未啟用不可變發佈，即根因）。
- `gh api repos/s12ryt/vesta/actions/permissions` → enabled:true。
- 讀取 `.github/workflows/release.yml`（124 行）、`build.yml`、`pages.yml`、`CMakePresets.json`。

### 寫入 (Write / Edit)
1. `.github/workflows/release.yml`（重寫，50 insertions(+) / 16 deletions(-)）— 觸發新增 `workflow_dispatch`（input `tag`）；job `env RELEASE_TAG`；checkout ref 動態；發佈步驟改為自適應（探測 immutable；已發佈 release 在非 immutable 時原地更新；資產驗證不再要求 draft；readback 僅在 immutable 啟用時嚴格檢查）。
2. 更新 `agent/deep_todos.md`（新增 T-004 列與詳情）、`agent/memory.md`（本節）。

### 刪除 (Delete)
- 無。

### 驗證 (Verify)
- 本地：`git diff --stat` = 1 file changed, 50 insertions(+), 16 deletions(-)。
- **待遠端驗證**：推送後確認 push 觸發 Build；`gh workflow run release.yml -f tag=<new-tag>` GREEN 且 release 含 `vesta.exe`。
- 本機仍缺 `cmake`/`cl`/`clangd`；惟 Build run 成功已證明原始碼可編譯。
