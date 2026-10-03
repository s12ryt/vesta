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
