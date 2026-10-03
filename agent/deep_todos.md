# Vesta — 深度任務紀錄 (deep_todos)

> 本檔案記錄項目的完整歷史任務。每次新增/變更任務時必須同步更新。

## 項目概覽

- 名稱：Vesta
- 定位：Windows x64 **外部 (external)** Counter-Strike 2 輔助程式（無注入、無驅動）
- 語言/建置：C++、CMake（`CMakePresets.json` / `CMakeLists.txt`），MSVC `/utf-8`
- 目前版本：tag `release: Vesta 1.1.9`
- 功能：戰鬥輔助（Aimbot/RCS/Triggerbot）、視覺（Player/Item/Projectile/Bomb ESP、Chams、World、Crosshair）、移動（Bunny Hop/Edge Jump）、投擲物站位、遊戲內雷達、沙箱化 Lua 5.4 API
- 既有介面語言：English (en)、Русский (ru)；漢化後新增 简体中文 (zh)、繁體中文 (zh_hant)

## 任務清單

| ID | 任務 | 狀態 | 備註 |
|----|------|------|------|
| T-001 | 專案漢化（新增簡體中文介面語言 `zh`） | 已完成（未經編譯驗證） | 見下方詳情 |
| T-002 | 新增繁體中文介面語言 `zh_hant`（含字型字形範圍） | 已完成（未經編譯驗證） | 見下方詳情 |
| T-003 | 提交並推送漢化成果至 Operator 帳號的 `vesta` 倉庫 `main` | 已完成 | commit `513eae8` → `s12ryt/vesta` |
| T-004 | 修復 GitHub Actions Release workflow（讓 tag 推送能正確產出／發佈 exe） | 已完成（待遠端驗證） | 見下方詳情 |

---

## T-001 專案漢化（簡體中文）

**來源需求**：Operator（m0003）「幫這個項目漢化」。

**目標**：在現有 English / Русский 之外，新增簡體中文 UI 語言，涵蓋選單與 HUD 文字。

**做法**：沿用既有 `render::localization` 機制（英文字串 → 對照表），新增 `zh` 語言與中文對照表，並為無 CJK 字形的字型合併中文字型回退。

### 變更檔案

1. `src/render/menu/localization.hpp`
   - `enum class id` 新增 `zh = 2`（`count` 隨之變為 3）。
   - 註解更新為 `"EN" / "RU" / "ZH"`。

2. `src/render/menu/localization.cpp`
   - 新增 `chinese()`：`unordered_map<string_view,const char*>`，~350 筆英文→簡體中文對照，依原 `russian()` 的分區註解（Navigation / Weapon group tabs / Combat cards / Combat rows / Interface / Shared option values / Visuals tabs / Player ESP / Sound ESP / Chams / Item ESP / Projectile ESP / Bomb ESP / Radar / Crosshair / Misc 各區 / ESP editor / Nade helper / Fallback tokens）。
   - 新增 `table_for(id)`：`zh` 回傳 `chinese()`，其餘回傳 `russian()`。
   - `code()` 改為 `value == id::ru ? "RU" : value == id::zh ? "ZH" : "EN"`。
   - 兩個 `tr()` 多載改為使用 `table_for(current())`（`current()==en` 時仍直接回傳原文，行為不變）。

3. `src/render/menu/misc_page.cpp`
   - 語言下拉 `languages[]` 由 `{"English", "Русский"}` 改為 `{"English", "Русский", "简体中文"}`。
   - 既有 `std::clamp(p.language, 0, id::count-1)` 與 `localization::set(...)` 因 `count` 增長自動支援 0..2，無需額外修改。

4. `src/render/overlay/overlay.cpp`
   - 建立 `menu_brand_30` 之後、`font_ready` 驗證之前，新增 CJK 字型合併區塊：
     - 候選字型（取第一個存在的）：
       `C:/Windows/Fonts/msyh.ttc`、`msyh.ttf`、`simhei.ttf`、`simsun.ttc`、`Deng.ttf`。
     - 範圍：`atlas->GetGlyphRangesChineseSimplifiedCommon()`。
     - 合併目標：`notosans_medium_12` (12.0f, smooth)、`esp_text_11` (11.0f, esp_text)、`menu_regular_12` (16.0f*dpi, smooth)、`menu_semibold_13` (16.0f*dpi, smooth)；`menu_brand_30` 僅 Latin 品牌字，略過。
     - 找不到候選字型時以 `diagnostics.warning("no Simplified Chinese font found; Chinese text may not render.")` 提示。
   - 既有 `font_ready` 檢查、`atlas->Build()`、`ImGui_ImplDX11_CreateDeviceObjects()` 不變。

5. `README.md`
   - Interface and automation 段落 bullet 由「English and Russian interface…」改為「English, Russian, and Simplified Chinese interface…」。

### 設計重點 / 風險

- 無新增 `.cpp`，`CMakeLists.txt` 為明確來源清單（非 glob），故**不需**改 CMake。
- `/utf-8` 已啟用，UTF-8 中文字面量可編譯。
- 測試未引用 `localization` 列舉/`count`，擴充列舉安全。
- 設定檔 `language` 欄位讀取時已 `clamp` 到 `[0, count-1]` 並呼叫 `set()`，舊設定不受影響。
- **字型為關鍵**：內嵌 `notosans_medium`（Latin+Cyrillic）與系統 Segoe UI 皆無 CJK，故必須合併中文字型，否則中文顯示為空白/豆腐字。
- 中文對照表存在**重複 key**（如 `Advanced`、`Smoke`、`Line Thickness` 等），與原 `russian()` 行為一致（`unordered_map` 後者覆蓋前者）。

### 驗證狀況

- 已完成：手動逐檔覆核、`git diff --stat` 對比（`localization.cpp` 599 insertions / 3 deletions）、CRLF/無 BOM/檔尾完整性檢查。
- **未完成**：編譯驗證。本環境 `cmake`、`cl`（MSVC）、`clangd` 均不在 PATH（僅 `vswhere.exe` 存在於 `C:\Program Files (x86)\Microsoft Visual Studio\Installer\`）。如需最終確認，可用 vswhere 找 VS 內建 CMake，或於具備 VS2022 + CMake 3.20+ 的機器執行：
  ```
  cmake --preset release
  cmake --build --preset release --parallel
  ctest --test-dir build/release -C Release --output-on-failure
  ```

### 後續可選事項

- 網站 `website/` 與 Lua 文件 `lua/docs/` 仍為英文/俄文；本次僅漢化程式中介面。如需文件漢化，另立任務。

---

## T-002 新增繁體中文（zh_hant）

**來源需求**：Operator（m0056）「把繁中也搞進去」。

**目標**：在 English / Русский / 简体中文 之外，再新增繁體中文 (`zh_hant`) UI 語言，並確保繁體字形可正確渲染。

**背景 / 關鍵問題**：
- ImGui 內建 `GetGlyphRangesChineseSimplifiedCommon()` 僅涵蓋約 2500 個「簡體常用字」，**不含多數繁體專用字形**（如 語、體、戰、鈕、設、準、緩…），直接沿用會使繁體字顯示為豆腐。
- `GetGlyphRangesChineseFull()` 可涵蓋，但字形數約 2 萬；四個字級合併後 atlas 可能需達 4096x8192（約 128MB RGBA），啟動時間與記憶體成本過高，不宜採用。

**做法**：以 `ImFontGlyphRangesBuilder` 建構「本 UI 實際會輸出的所有簡繁中文字串」+ `GetGlyphRangesChineseSimplifiedCommon()` 的精準聯集範圍，交由 `merge_font_from_file` 合併字型。既不漏字，也不炸 atlas。

### 變更檔案

1. `src/render/menu/localization.hpp`
   - `enum class id` 新增 `zh_hant = 3`（`count` 變為 4）。
   - 註解改為 `"EN" / "RU" / "ZH" / "TW"`。
   - 新增宣告 `[[nodiscard]] const char *cjk_glyph_text();`（串接所有簡繁中文譯文，供字型範圍建構）。

2. `src/render/menu/localization.cpp`
   - 頂部新增 `#include <string>`。
   - 新增 `traditional()`：與 `chinese()` 同 key 集合（~350 筆）之英文→繁體中文對照表，涵蓋相同分區。
   - `table_for(id)` 改為 `zh_hant` → `traditional()`、`zh` → `chinese()`、其餘 → `russian()`。
   - `code()` 改為 `ru→"RU" / zh→"ZH" / zh_hant→"TW" / else "EN"`。
   - 新增 `cjk_glyph_text()`：以 static std::string 串接 `chinese()` + `traditional()` 之所有譯文（值）並附加 `"简体中文繁體中文"`（語言下拉標籤本身不在對照表內，需一併預載），回傳 `.c_str()`。

3. `src/render/menu/misc_page.cpp`
   - 語言下拉 `languages[]` 改為 `{"English", "Русский", "简体中文", "繁體中文"}`。

4. `src/render/overlay/overlay.cpp`
   - 候選字型新增 `C:/Windows/Fonts/msjh.ttc`（微軟正黑體，繁體字型，僅在標配 msyh 缺失時備援）；清單為 `msyh.ttc`、`msyh.ttf`、`msjh.ttc`、`simhei.ttf`、`simsun.ttc`、`Deng.ttf`。
   - 範圍由原本直接的 `GetGlyphRangesChineseSimplifiedCommon()` 改為：
     ```
     ImFontGlyphRangesBuilder cjk_builder;
     cjk_builder.AddText( render::localization::cjk_glyph_text( ) );
     cjk_builder.AddRanges( atlas->GetGlyphRangesChineseSimplifiedCommon( ) );
     static ImVector<ImWchar> cjk_range_storage;
     cjk_builder.BuildRanges( &cjk_range_storage );
     const ImWchar* cjk_ranges = cjk_range_storage.Data;
     ```
   - 合併目標不變（notosans_medium_12 / esp_text_11 / menu_regular_12 / menu_semibold_13）；警告文字改為 `"no Chinese font found; Chinese text may not render."`。

5. `README.md`
   - bullet 再改為「English, Russian, Simplified Chinese, and Traditional Chinese interface, DPI scaling, custom palette, and portable layouts.」。

### 設計重點 / 風險

- `code()` 仍無任何呼叫點（grep 確認），其 TW 標籤僅為完備性。
- `cjk_glyph_text()` 回傳指向函式內 `static` `std::string` 的指標，生命週期至程式結束，安全。
- `ImFontGlyphRangesBuilder` / `ImVector` 由 `imgui.h` 提供（`overlay.cpp` 已 `#include "imgui.h"`）；`overlay.cpp` 已 `#include <render/menu/localization.hpp>`，無新相依。
- 未新增 `.cpp`，CMake 明確來源清單**不需**修改。
- 遊戲內動態中文（如玩家 Steam 名稱）若含未列入 UI 譯文的字，仍可能缺字；本次範圍以介面字串為準。

### 驗證狀況

- 以 `git diff --stat` 及逐檔覆核確認變更。
- **未完成**：編譯驗證（環境同 T-001，缺 `cmake`/`cl`/`clangd`）。

---

## T-003 提交並推送至 Operator 帳號倉庫

**來源需求**：Operator（m0071）「先推到我帳號下的vesta倉庫的main」。

**目標**：將 T-001/T-002 全部未提交變更提交，並推送到 Operator GitHub 帳號（`s12ryt`）下的 `vesta` 倉庫 `main` 分支。

**倉庫關係**：
- `origin` = `https://github.com/Read1dno/vesta.git`（上游，無寫入權）。
- 新增遠端 `myfork` = `https://github.com/s12ryt/vesta.git`（Operator 帳號的 fork，PUBLIC，預設分支 `main`）。
- 推送前 `myfork/main` 落後上游數個 commit（在 `29bf273`）；本次推送為 fast-forward。

**操作**：
1. `gh auth status` 確認登入帳號 `s12ryt`（token scopes 含 `repo`、`workflow`）。
2. `git add -A`（5 個修改檔 + 新增 `agent/` 三檔）。
3. `git commit` → commit `513eae8`「feat(i18n): add Simplified and Traditional Chinese interface」（8 files changed, 1557 insertions(+), 7 deletions(-)）。
4. `git remote add myfork https://github.com/s12ryt/vesta.git`。
5. `git push myfork main` → `29bf273..513eae8  main -> main`（成功）。

**結果**：
- `s12ryt/vesta` 的 `main` 已更新至 `513eae8`（含上游 `release: Vesta 1.1.9` 與本次漢化）。
- 本地 `main` 相對 `origin/main` 為 `ahead 1`。
- fork 上另有既存分支 `feature/zh-cn-zh-tw-localization`（非本次操作產生）。

**備註**：
- commit message 有輕微錯字 `English/RУсский`（混用拉丁 R 與西里爾 Усский），已推送故不重寫歷史。
- 仍未經編譯驗證（環境缺 `cmake`/`cl`/`clangd`）。

---

## T-004 修復 GitHub Actions Release workflow

**來源需求**：Operator（m0089）「請你去讓github-workflow正確產出exe」。

**診斷（關鍵發現）**：
- exe **本來就有正確產出**：`build.yml` 的 run `#37141266795`（workflow_dispatch）為 **SUCCESS**，artifact `vesta-windows-x64` 約 2.5MB（`build/bin/vesta.exe`）。⇒ 我們的中文原始碼**可通過編譯**。
- `push` 到 `main` 當時**沒有觸發 Build run**（可能 Actions 於推送當下尚未啟用，約 17:38Z 才啟用）。
- 三個 **Release run 全數 FAILURE**（tag `v1.1.9`、`v1.1.9-s12ryt`、`v1.1.9-s12rytCE`，皆指向 commit `996e859`）：
  1. 前兩者：`Release <tag> already exists and is published. Immutable releases cannot be replaced.`（tag 已被推過且有已發佈 release）。
  2. 第三者：release 確實建立並發佈，但最後拋 `GitHub published <tag> without immutable protection.`

**根因**：
- **不可變發佈（immutable releases）是 repo 的 opt-in 設定，fork 上為關閉**：
  `gh api repos/s12ryt/vesta/immutable-releases` → `{"enabled":false,"enforced_by_owner":false}`；release 物件 `immutable:false`。
  但原 workflow 於結尾硬性斷言 `$published.immutable` 必須為 true，故在 fork 上必失敗。
- 已發佈的 tag 被重推時，原 workflow 於開頭硬性拋出「不可取代」錯誤。
- 參考：`GET|PUT|DELETE /repos/{owner}/{repo}/immutable-releases`（check/enable/disable）。

### 變更檔案

1. `.github/workflows/release.yml`（重寫，改為自適應）
   - 觸發新增 `workflow_dispatch`，輸入 `tag`（必填字串），可手動 (re)publish 既存 tag。
   - job 層 `env: RELEASE_TAG: ${{ github.event.inputs.tag || github.ref_name }}`；checkout `with: ref: ${{ github.event.inputs.tag || github.ref }}`。
   - Package / checksum / publish 步驟改用 `$env:RELEASE_TAG`（不再直接用 `github.ref_name`）。
   - 發佈步驟更名 `Publish GitHub release`：
     - 先探測 `$immutableEnabled`（`gh api .../immutable-releases`，關閉時 `Write-Warning`）。
     - release 存在 + 已發佈 + immutable 啟用 → 仍硬性拋錯（維持嚴格行為）。
     - release 存在 + 已發佈 + immutable 未啟用 → `gh release upload --clobber` + `gh release edit --notes-file`（原地更新）。
     - release 不存在 → `gh release create --verify-tag --draft ...`。
     - 資產驗證僅在 `$missingAssets.Count -ne 0` 時拋錯（不再要求 draft 狀態）。
     - 僅在仍為 draft 時 `gh release edit --draft=false` 發佈。
     - 結尾 readback 僅在 `$immutableEnabled -and -not $readback.immutable` 時拋錯。

### 設計重點 / 風險

- 對**上游 `Read1dno/vesta`（immutable 啟用）**行為不變：仍嚴格拒絕覆蓋已發佈 release，且仍驗證 immutable。
- 對 **fork `s12ryt/vesta`（immutable 關閉）**：不再假設不可變，可正常建立／更新 release。
- 未新增/刪除其他 workflow；`build.yml`、`pages.yml` 未動。
- 若要以全新乾淨結果驗證，建議用**新 tag**（未發佈過）dispatch。

### 驗證狀況

- 本地：`git diff --stat`（1 file changed, 50 insertions(+), 16 deletions(-)）。
- **遠端已驗證（2026-10-04）**：
  - 推送 `main`（commit `7a7d51c`）**成功觸發 Build** run `#37146220497` → **SUCCESS**（先前 push 不觸發僅因當下 Actions 尚未啟用）。
  - 以 `gh workflow run release.yml -f tag=v1.1.9-s12rytCE` 觸發 Release run `#37146236857` → **SUCCESS**（修正前同一 tag 為 FAILURE）。
  - `gh release view v1.1.9-s12rytCE`：`draft:false`、`immutable:false`、資產含 `vesta.exe`、`vesta.pdb`、`Vesta-v1.1.9-s12rytCE-forum.zip`、`SHA256SUMS.txt`、`LICENSE`、`NOTICE`、`THIRD_PARTY_NOTICES.md`。
- 本機仍未編譯（環境缺 `cmake`/`cl`/`clangd`），惟 Build run 成功已證明原始碼可編譯。
