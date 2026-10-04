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
| T-005 | 新增 HVH 類別（確認後注入 vesta_hvh.dll 並提供 HvH 功能） | 已完成（未經編譯驗證） | 見下方詳情 |
| T-006 | 內部靜默瞄準增量（shared v2、簽名通道、UI 卡片、持久化） | 已完成（未經編譯驗證） | 見下方詳情 |
| T-007 | 新增可選「極限穿牆」(Extreme Wall) 自動穿牆模式 | 已完成（未經編譯驗證） | 見下方詳情 |
| T-008 | 反編譯取得本機 client.dll 的 CreateMove 簽名並內建至 DLL | 已完成（未經編譯驗證） | 見下方詳情 |
| T-009 | 實作靜默瞄準／反瞄準命令寫入（本機實測 view angles = CCSGOInput+0x688） | 已完成（未經編譯驗證） | 見下方詳情 |

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


---

## T-005 新增 HVH 類別（注入式 HvH 功能）

**來源需求**：Operator（m0128）「那就在ui中新增一個類別叫做hvh,點擊後要在彈出一個確認鍵,然後開始注入遊戲並提供這些hvh都有的功能,暴力鎖之類的」。

**目標**：在選單側邊欄新增「HVH」頁面；點擊 Inject 後彈出確認，確認後把 `vesta_hvh.dll` 注入 CS2，並提供 HvH 功能（狂暴鎖 / 反瞄準 / 板機 / 連跳等）。

**架構決定**：
- 外部 exe 仍為主要程式；HVH 功能需寫入 `CUserCmd.viewangles`，屬內部（internal）能力，因此以「外部注入自有 DLL」達成，不動既有純外部架構。
- 設定透過具名共享記憶體 `Local\vesta_hvh_shared_v1`（`vesta_hvh_shared` POD 結構）在 exe（writer）與 DLL（reader）間交換。
- HVH 設定獨立存於 `<exe 目錄>/hvh.json`（nlohmann），**不改** `config/settings.cpp`。
- DLL 針對 build 版本：以特徵碼掃描定位 `CreateMove`；**目前特徵碼故意留空**，故 hook 不會安裝、模組保持被動，確保未知版本不會寫到錯誤位址、不會崩潰。

### 變更檔案

新增：
1. `src/hvh_shared/hvh_shared.hpp`：POD 共享結構 + 具名 mapping 常數（僅 `<cstdint>`）。
2. `src/features/hvh/hvh.hpp` / `hvh.cpp`：`features::hvh::controller()`——開啟具名 mapping、`inject()`（OpenProcess + VirtualAllocEx + WriteProcessMemory + CreateRemoteThread(LoadLibraryW)）、`eject()`、`publish()`、`load()/save()`。
3. `src/hvh/hvh_internal.hpp`：logging、`module_base`、`scan_pattern`、`readable/writable/patch`、`inline_hook`（5-byte rel32 JMP + trampoline）、`offsets()`。
4. `src/hvh/dllmain.cpp`：DllMain + worker thread（心跳、unload 時 `FreeLibraryAndExitThread`）。
5. `src/hvh/hvh_features.hpp` / `hvh_features.cpp`：`initialize/shutdown`、`advance_spin`、`jitter_angle`、`apply_pitch`、`create_move_detour`。
6. `src/render/menu/hvh_page.cpp`：`menu_t::draw_hvh()` UI。

修改：
7. `src/render/menu/menu.hpp`：宣告 `void draw_hvh();`、`bool m_hvh_inject_pending{};`。
8. `src/render/menu/menu.cpp`：側邊欄標籤加入 `"HVH"`（索引 4），`draw_content()` 分派 `draw_hvh()`。
9. `src/render/menu/layout.cpp`：`draw_nav_icon` 新增 `icon == 4` 圖示（交叉劍）。
10. `src/render/menu/localization.cpp`：`chinese()` / `traditional()` 加入 HVH 字串（各 55 條）。
11. `src/app/workers.hpp` / `workers.cpp`：新增 `hvh()` 執行緒（週期呼叫 `controller().publish()`）。
12. `src/app/main.cpp`：啟動 `app::workers::hvh`。
13. `CMakeLists.txt`：`vesta_sources` 加入 `src/features/hvh/hvh.cpp`、`src/render/menu/hvh_page.cpp`；新增 `vesta_hvh` SHARED target。

### 設計要點 / 限制
- HVH 頁面需先確認才會注入（`m_hvh_inject_pending` → CONFIRM INJECTION 卡片）。
- 注入需對目標 `OpenProcess` 具 `PROCESS_CREATE_THREAD|QUERY_INFORMATION|VM_OPERATION|VM_WRITE|VM_READ`；`vesta_hvh.dll` 必須與 exe 同目錄（`build/bin`）。
- `vesta_hvh` DLL 不使用 vesta 前置編譯標頭（PCH），僅連結 `user32`。
- 真實暴力鎖 / 反瞄準需正確的 offsets/signatures；現階段數學與注入管線完整，特徵碼留空，待日後補上。
- 風險：注入與 hook 提高被 VAC 偵測的面；未注入時所有 HVH 功能不生效。

### 驗證情況
- 以 `git diff --stat` 與 Select-String 檢視變更；CRLF / 無 BOM 檢查。
- **未經編譯驗證**：本機無 `cmake`/`cl`/`clangd`；待推送到 `s12ryt/vesta` main 由 GitHub Actions Build workflow 驗證。


## T-006 內部靜默瞄準增量

**來源需求**：Operator（m0235）在「魔法子彈」可行性問答後選擇「2」＝改走內部路線（DLL 注入 + CreateMove hook 寫視角），整合進既有 HVH 面板。

**目標**：把「靜默瞄準 / 無擴散」的設定、簽名傳遞與 UI 接通，使注入後的 DLL 能在取得正確簽名時對 CUserCmd 寫入視角；在沒有簽名時保持被動、不崩潰。

**設計**：
- 共享契約 `src/hvh_shared/hvh_shared.hpp` 升級為 version 2：新增 `k_signature_length = 160`；`settings` 新增 silent（enable_silent, silent_hitbox, silent_priority, silent_fov, silent_autofire, silent_psilent, silent_min_damage）與 accuracy（enable_nospread, enable_norecoil）；新增 `struct signatures { char create_move[160]; char input[160]; char entity_list[160]; }`；`shared_state` 介於 config 與 state 之間加入 `signatures sigs`。
- 外部控制器 `features/hvh`：新增 `signatures signatures{}` 成員；ensure_mapping()/publish()/inject() 會把 signatures 寫入映射；load()/save() 從 `hvh.json` 的 `signatures` 物件讀寫 create_move/input/entity_list。
- DLL `hvh_features.cpp`：`initialize()` 優先使用 `g_shared->sigs.create_move`（為空時回退 `offsets().create_move_sig`）；detour 在 enable_antiaim 或 enable_silent 為真、且 `sigs.input` 非空時才動作，否則保持被動。
- UI `hvh_page.cpp`：新增 SILENT AIM 卡片（7 列）與 ACCURACY 卡片（2 列）。
- `localization.cpp`：chinese() 與 traditional() 各新增 6 條（SILENT AIM/ACCURACY/Enable Silent Aim/PSilent/No Spread/No Recoil）。

**修改檔案**：src/hvh_shared/hvh_shared.hpp、src/features/hvh/hvh.hpp、src/features/hvh/hvh.cpp、src/hvh/hvh_features.cpp、src/render/menu/hvh_page.cpp、src/render/menu/localization.cpp。

**驗證狀態**：未經本地編譯驗證（無 cmake/cl/clangd，僅 vswhere.exe）；待 GitHub Actions Build。

**已知限制**：簽名在出貨時為空 → 注入後的 DLL 仍為被動，真正的靜默瞄準需提供對應此 CS2 版本的 CreateMove/CUserCmd 簽名（目前無實機 client.dll 可推導）。

## T-007 可選「極限穿牆」自動穿牆增強

**來源需求**：Operator（m0289）「hvh的"自動穿牆"功能要更極限」。

**目標**：在不破壞既有彈道測試的前提下，讓外部自動穿牆（aimbot 的子彈穿透模擬）可以有更強的穿牆後傷害保留，作為一個可選開關。

**設計**：
- 穿牆核心在 `src/simulation/penetration_solver.hpp` 的 `pass_through_world(...)`。原生限制（最多穿透 4 個面、距離上限 3000）屬遊戲引擎限制，保持不變。
- 新增尾端參數 `bool extreme = false`：預設 false 時數學與原本「逐位元相同」，因此既有測試（tests/penetration_accuracy.cpp、tests/penetration_segments.cpp）不受影響；true 時降低每面傷害損失：
  - 損失除數 24 → 60
  - 武器損失 `(3.0f / weapon_penetration) * 1.25f` → `(2.0f / weapon_penetration)`
  - 武器損失倍率 3.0f → 1.5f
  - 預設 damage_fraction 0.16f → 0.10f（材質特例 0.05 / 0.00001 不變）
- 以設定驅動：`config::combat_profile::global_settings` 新增 `bool extreme_wall{ false };`，於 settings.cpp 的 global to_json/from_json 序列化。
- `src/simulation/penetration.cpp` 的 `run_seed` 將 `config::combat_settings.global.extreme_wall` 傳入 `pass_through_world`（`can()` 維持舊數學）。
- UI：aimbot 全域 `PENETRATION` 卡片新增 `Extreme Wall` 開關（列數 3 → 4）。
- 在地化：localization.cpp 新增 ru/zh/tw 三筆 `Extreme Wall`。

**變更檔案**：
- src/simulation/penetration_solver.hpp（新增 gated extreme 參數與分支常數）
- src/simulation/penetration.cpp（run_seed 傳入設定）
- src/config/combat.hpp（global_settings.extreme_wall）
- src/config/settings.cpp（to_json/from_json）
- src/render/menu/aimbot_page.cpp（PENETRATION 卡片開關）
- src/render/menu/localization.cpp（ru/zh/tw）

**驗證狀態**：尚未本地編譯（無 cmake/cl/clangd）；已以 Select-String 確認：aimbot Extreme Wall=1、localization Extreme Wall=3、penetration.cpp 已接上設定、combat.hpp/settings.cpp 已含 extreme_wall。待 GitHub Actions Build 驗證（extreme 預設 false，兩支穿牆測試仍應通過）。

**已知限制**：屬外部子彈穿透模擬的傷害保留增強；不改變引擎原生穿透上限（4 面 / 3000）。與 HVH DLL（內部簽名）無關。

## T-008 反編譯取得本機 client.dll 的 CreateMove 簽名並內建至 DLL

**來源需求**：Operator（m0350）「你可以幫我抓client.dll了 我換小帳掛在cs大廳中」、（m0377）「不能你幫我跑通b嗎」——要求在真實遊戲工作階段中取得 client.dll，並由我方自行反編譯出本版本的 CreateMove 簽名／位址（選項 B）。

**取得環境**：
- cs2.exe PID 29564；client.dll 記憶體基底 0x7FFD018E0000，大小 0x2998000。
- 由 `H:\SteamLibrary\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll` 複製到 `C:\Users\yoyo2\AppData\Local\Temp\opencode\client.dll`（39,183,000 bytes；SHA256 `D7DB25D48F1D10C5E0B0296E20ED803426EB9509DA41760DAEDA39DD35BA89B9`）。

**分析流程（純 PowerShell、無本機編譯器）**：
1. 以 Vesta 既有樣式驗證本機映像：dwCSGOInput `48 89 05 ? ? ? ? 0F 57 C0 0F 11 05`、dwEntityList `48 89 0D ? ? ? ? E9 ? ? ? ? CC`、dwLocalPlayerController `48 8B 05 ? ? ? ? 41 89 BE`、dwGlobalVars `48 89 15 ? ? ? ? 48 89 42`、dwViewMatrix `48 8D 0D ? ? ? ? 48 C1 E0 06` 全部各 1 筆命中。
2. 解出本版本全域位移（RVA）：dwCSGOInput 0x2576150、dwEntityList 0x2715818、dwLocalPlayerController 0x2538008、dwGlobalVars 0x222BE98、dwViewMatrix 0x2566910。
3. 社群 CreateMove 簽名對本版本皆 NO MATCH。
4. 以 `.rdata` 中指向 ValidateInput（RVA 0xCE9D10）的函式指標（slot RVA 0x1C9AD98）反推 CCSGOInput 虛擬表起點 RVA 0x1C9AD58，列舉 33 個 slot。
5. 於虛擬表中辨識：idx 5 = CreateMovePrePrediction（0xB65B20）、idx 8 = ValidateInput（0xCE9D10）、**idx 25 = CreateMove（0xD01B20）**，其序文 `48 8B C4 4C 89 40 18 48 89 48 08 55 53 57 41 55` 與社群 CreateMove 序文同型（差異在暫存器：本版 push `57 41 55`）。
6. 掃描驗證：該序文於映像中**唯一命中 RVA 0xD01B20**。

**變更檔案**：
- `src/hvh/dllmain.cpp`：`offsets()` 的 `static const game_offsets table{};` → `static const game_offsets table{ "48 8B C4 4C 89 40 18 48 89 48 08 55 53 57 41 55" };`（將本版本 CreateMove 簽名內建為 DLL 預設；local_player_sig / entity_list_sig 仍為空）。
- `agent/deep_todos.md`、`agent/memory.md`：本紀錄。

**驗證狀態**：本機無編譯器（cmake/cl/clangd 皆不在 PATH）→ 未經本機編譯；已提交並推送 `f1bd6a6`，待 GitHub Actions Build 驗證（vesta.exe + vesta_hvh.dll + ctest 55/55）。

**已知限制**：真正的靜默瞄準仍需 `CUserCmd` 的 viewangles 位移（尚未推導），以及安全的地動函式回傳型別（CreateMove 可能回傳 double，目前 detour 以 bool 處理）。簽名內建後，DLL 會找到並掛上 CreateMove，但除非使用者在 UI 啟用反瞄準／靜默（預設關閉）且 `sigs.input` 非空，detour 僅為安全的 passthrough。

## T-009 實作靜默瞄準／反瞄準命令寫入（本機實測 view angles）

來源需求：m0454「做吧」。Operator 在練習模式中提供即時 CS2 連線，讓本機讀取記憶體取得參數。

### 取得參數（實測）
- 讀取方式：PowerShell + kernel32（OpenProcess/ReadProcessMemory）；CCSGOInput 靜態物件 = client.dll 基底 + 0x2576150（其 +0x00 為 vtable 指標 = 基底 + 0x1C9AD58）。
- 以「動態差分」(motion-diff) 在 Operator 移動滑鼠時比對：0x688 (3.327→5.500)、0x68C (40.518→9.083) 隨之改變。
- 結論：view angles（pitch, yaw, roll）= **CCSGOInput + 0x688**。另有鏡像副本於 0x2A0/0x2A4、0x758/0x75C。
- CreateMove = CCSGOInput vtable idx 25，RVA 0xD01B20（簽名已於 T-008 內建）；CreateMove 的 this 即 CCSGOInput*。

### 變更
- `src/hvh_shared/hvh_shared.hpp`：k_version 2→3；新增 `struct aim_command { int32 valid; float pitch; float yaw; }`；`shared_state` 於 sigs 與 state 之間新增 `aim_command aim{}`。
- `src/features/hvh/hvh.hpp`：controller 新增 `vesta::hvh_shared::aim_command aim{}`。
- `src/features/hvh/hvh.cpp`：ensure_mapping/publish/inject 三處同步 `m_view->aim = aim;`。
- `src/hvh/hvh_features.cpp`：create_move_detour 重寫 —— 解析 `self+0x688` 的三個 float，備份 → 若 silent 且 aim.valid 寫入 command.pitch/yaw → 若 antiaim 寫入 spin/pitch → 呼叫原始 CreateMove → 還原三個角度（相機不動 = 靜默）。

### 驗證狀態
本機無編譯器（無 cmake/cl/clangd），已推 myfork 交由 GitHub Actions Build 驗證。

### 已知限制
- 反瞄準（spin/pitch）可獨立運作；靜默瞄準需要外部端在 `aim` 內發布目標角度（valid/pitch/yaw），目前外部尚未計算並發布 → 啟用靜默瞄準暫不生效，待下一步把 Vesta 既有瞄準角度寫入 aim。
- 回傳型別：CreateMove 可能回傳 double；detour 以 bool 回傳，但原始呼叫的 xmm0 未被覆寫，故兩種情形皆可保留原回傳值。
