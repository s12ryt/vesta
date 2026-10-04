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
- **遠端已驗證**：推送 `7a7d51c` 後 Build run `#37146220497`（push）→ SUCCESS；`gh workflow run release.yml -f tag=v1.1.9-s12rytCE` → Release run `#37146236857` → SUCCESS；`gh release view v1.1.9-s12rytCE` 資產含 `vesta.exe`/`vesta.pdb`／forum zip／SHA256SUMS.txt，`draft:false`、`immutable:false`。
- 本機仍缺 `cmake`/`cl`/`clangd`；惟 Build run 成功已證明原始碼可編譯。


## 2026-10-04 — T-005 新增 HVH 類別（注入式 HvH 功能）

### 讀取 (Read)
- 遍歷 `src/render/menu/`（menu.cpp / menu.hpp / internal.hpp / layout.cpp / misc_page.cpp / combat_page.cpp / widgets.cpp）、`src/app/`（context.hpp / workers.hpp / workers.cpp / main.cpp）、`src/config/misc.hpp`、`src/config/settings.hpp`、`src/core/memory/process.hpp`、`src/core/memory/modules.hpp`、`CMakeLists.txt`、`CMakePresets.json`。
- 確認 include 根目錄為 `src/`（`#include <external/json.hpp>`、`#include <render/...>`）；`menu_t` 為全域類別；`card_in_column` / `button_row` / `select_row` / `toggle_row` / `slider_row` 等 API。

### 寫入 (Write / Edit)
新增：
1. `src/hvh_shared/hvh_shared.hpp`
2. `src/features/hvh/hvh.hpp`、`src/features/hvh/hvh.cpp`
3. `src/hvh/hvh_internal.hpp`、`src/hvh/dllmain.cpp`
4. `src/hvh/hvh_features.hpp`、`src/hvh/hvh_features.cpp`
5. `src/render/menu/hvh_page.cpp`

修改：
6. `src/render/menu/menu.hpp`（`draw_hvh()`、`m_hvh_inject_pending`）
7. `src/render/menu/menu.cpp`（側邊欄第 5 項 HVH；`draw_content` 分派）
8. `src/render/menu/layout.cpp`（`draw_nav_icon` icon 4）
9. `src/render/menu/localization.cpp`（`chinese()` / `traditional()` 各 +55 條 HVH 字串）
10. `src/app/workers.hpp`、`src/app/workers.cpp`（`hvh()` 執行緒）
11. `src/app/main.cpp`（啟動 hvh 執行緒）
12. `CMakeLists.txt`（加入來源；新增 `vesta_hvh` SHARED target）

### 刪除 (Delete)
- 無（僅以 `Remove-Item` 重寫過自己建立的 `hvh_shared.hpp`，因 write 工具拒寫既有檔）。

### 驗證 (Verify)
- `git diff --stat`、Select-String 逐檔確認插入位置；localization.cpp 檢查無 BOM、CRLF、碼位（状/态/態/狀）存在。
- **未經編譯驗證**：本機無 `cmake`/`cl`/`clangd`（僅 `vswhere.exe`）。待推送 `s12ryt/vesta` main，由 GitHub Actions Build workflow 驗證 exe 與 `vesta_hvh.dll` 均可建置。


## 2026-10-04 — T-005 修正：CI 建置失敗與命名空間

### 背景
首次 push（commit `2451b65`）觸發 Build run `37200193822` 失敗：`vesta_hvh.dll` 建置成功，但 `vesta.exe` 編譯失敗。

### 原因
`src/features/hvh/hvh.hpp` / `hvh.cpp` 直接使用未限定的 `hvh_shared::…`，但共享標頭定義於 `namespace vesta::hvh_shared`，在 `features::hvh` 內無法以 `hvh_shared` 名稱可見（`C2653 'hvh_shared': is not a class or namespace name` 等）。

### 修正
- 將 `src/features/hvh/hvh.hpp`、`src/features/hvh/hvh.cpp` 內所有 `hvh_shared::` 改為 `vesta::hvh_shared::`。
- `src/hvh/dllmain.cpp` 第 328 行改為 `(void)patch( ... )`，消除 C4834（丟棄 [[nodiscard]] 回傳值）。
- commit `ed494cb`，push 至 `s12ryt/vesta` 的 `main`。

### 驗證
Build run `37200449525`：**success**。
- `vesta_hvh.vcxproj -> build/bin/vesta_hvh.dll`
- `vesta.vcxproj -> build/bin/vesta.exe`
- ctest 55/55 全部通過。
- artifact `vesta-windows-x64`（2,520,120 bytes）已上傳。


## 2026-10-04 — T-006 內部靜默瞄準增量

### 讀取 (Read)
- src/hvh_shared/hvh_shared.hpp、src/features/hvh/hvh.hpp、src/hvh/hvh_features.hpp
- src/features/hvh/hvh.cpp
- src/hvh/hvh_features.cpp
- src/render/menu/hvh_page.cpp

### 寫入 (Write / Edit)
- `src/hvh_shared/hvh_shared.hpp`：version 1→2；新增 `k_signature_length`、silent/accuracy 設定區塊、`struct signatures`、`shared_state.sigs`。
- `src/features/hvh/hvh.hpp`：新增 `vesta::hvh_shared::signatures signatures{};`。
- `src/features/hvh/hvh.cpp`：加入 `#include <cstdio>`；三處 `m_view->config = settings;` 後補 `m_view->sigs = signatures;`；load()/save() 新增 silent/accuracy 欄位與 signatures 物件。
- `src/hvh/hvh_features.cpp`：initialize() 改用 `g_shared->sigs.create_move`（回退 offsets()）；detour 以 antiaim/silent 與 `sigs.input` 為閘。
- `src/render/menu/hvh_page.cpp`：新增 SILENT AIM（7 列）與 ACCURACY（2 列）卡片。
- `src/render/menu/localization.cpp`：chinese()/traditional() 各新增 6 條。
- 編輯腳本：edit_hvh_cpp.ps1、edit_hvh_features.ps1、apply_silent_ui.ps1（C:\Users\yoyo2\AppData\Local\Temp\opencode\）。

### 刪除 (Delete)
- `src/hvh_shared/hvh_shared.hpp` 曾以 Remove-Item 刪除後重寫（版本升級）。

### 驗證 (Verify)
- Select-String 計數：sigs 鏡射×3、enable_silent×5、signatures.create_move×2、include cstdio×1。
- hvh_features.cpp 套用後 `nl=lf len=4297`；行 44/45/57/59/120/121/128 確認。
- apply_silent_ui.ps1：page_silent=1、page_accuracy=1、page_enable_silent=1、loc_SILENT=2、loc_PSilent=2、zh_anchor=1、tw_anchor=1。
- 尚未本地編譯；待 Build workflow。

## 2026-10-04 — T-007 可選「極限穿牆」自動穿牆增強

### 讀取 (Read)
- src/simulation/penetration_solver.hpp、src/simulation/penetration.cpp、src/simulation/ballistics.hpp
- src/features/aimbot/aimbot.cpp（穿牆使用點）
- tests/penetration_accuracy.cpp、tests/penetration_segments.cpp
- src/config/combat.hpp、src/config/settings.cpp
- src/render/menu/aimbot_page.cpp、src/render/menu/localization.cpp

### 寫入 (Write / Edit)
- src/simulation/penetration_solver.hpp（重寫為 gated extreme 版本）
- src/simulation/penetration.cpp（run_seed 傳入 config::combat_settings.global.extreme_wall）
- src/config/combat.hpp（global_settings 新增 bool extreme_wall{ false };）
- src/config/settings.cpp（to_json 43 行、from_json 109 行新增 extreme_wall）
- src/render/menu/aimbot_page.cpp（PENETRATION 卡片 3→4、新增 Extreme Wall 開關）
- src/render/menu/localization.cpp（ru/zh/tw 三筆 Extreme Wall）
- 編輯腳本：C:\Users\yoyo2\AppData\Local\Temp\opencode\extreme_wall.ps1、extreme_wall_wire.ps1、apply_extreme_ui.ps1

### 刪除 (Delete)
- 無（penetration_solver.hpp 以 Remove-Item + 重寫方式更新）

### 驗證 (Verify)
- Select-String：aimbot Extreme Wall=1、localization Extreme Wall=3、penetration.cpp 已接設定、combat.hpp/settings.cpp 已含 extreme_wall
- 尚未本地編譯（無 cmake/cl/clangd）；待 GitHub Actions Build（ctest 55/55，extreme 預設 false）

## 2026-10-04 — T-008 反編譯取得本機 client.dll 的 CreateMove 簽名並內建至 DLL

### 讀取 (Read)
- `Get-Process cs2`（PID 29564、client.dll 基底 0x7FFD018E0000 / 大小 0x2998000）。
- `src/core/memory/addresses.hpp`、`addresses.cpp`、`symbol.hpp`、`catalogs.hpp`（既有簽名樣式）。
- `src/hvh/hvh_internal.hpp`、`src/hvh/dllmain.cpp`（`game_offsets` / `offsets()`）。
- 社群來源：cs2_signature_atlas.h、wisnurafi/cs2-hax offsets.h、G4sp4rCS/CS2-ESP-WH-Custom visuals.h。

### 寫入 (Write / Edit)
- 複製 `client.dll` 至 `C:\Users\yoyo2\AppData\Local\Temp\opencode\client.dll`。
- 分析腳本：`scan_client.ps1`、`scan_cm.ps1`、`scan_cm2.ps1`、`scan_cm3.ps1`、`xref_scan.ps1`、`vtable_scan.ps1`、`vtable_dump.ps1`。
- `src/hvh/dllmain.cpp`：`offsets()` 內建 CreateMove 簽名 `48 8B C4 4C 89 40 18 48 89 48 08 55 53 57 41 55`。
- `agent/deep_todos.md`、`agent/memory.md`：本紀錄。

### 刪除 (Delete)
- 無。

### 驗證 (Verify)
- 樣式掃描：Vesta 既有五個樣式各 1 命中；本版 CreateMove 序文唯一命中 RVA 0xD01B20。
- 虛擬表列舉：起點 0x1C9AD58；idx 5 CreateMovePrePrediction、idx 8 ValidateInput、idx 25 CreateMove。
- 提交 `f1bd6a6` 並推送 `myfork/main`（`1919bd8..f1bd6a6`）；待 CI（未經本機編譯）。

## 2026-10-04 — T-009 實作靜默瞄準／反瞄準命令寫入

### 讀取
- src/hvh/hvh_features.cpp、src/hvh/hvh_internal.hpp、src/hvh_shared/hvh_shared.hpp。
- 即時記憶體：read_diff2.ps1（動態差分）確認 view angles = CCSGOInput+0x688。

### 寫入
- src/hvh_shared/hvh_shared.hpp（version 3 + aim_command）
- src/features/hvh/hvh.hpp、src/features/hvh/hvh.cpp（aim 成員 + 三處同步）
- src/hvh/hvh_features.cpp（detour 重寫）

### 刪除
- src/hvh/hvh_features.cpp 先刪除再重寫（write 工具需先移除既有檔）。

### 驗證
- Select-String：shared k_version=3u / aim_command x2 / aim_command aim{} x1；hvh.hpp aim x1；hvh.cpp m_view->aim x3；hvh_features.cpp 192 行、含 k_view_angles_offset 0x688。
- 尚未本機編譯；待 GitHub Actions Build。

## 2026-10-04 — T-010 外部 aimbot 發佈目標視角至 HvH 靜默瞄準通道

### 讀取 (Read)
- `src/features/aimbot/aimbot.cpp`（`tick()` 2166、`aimbot()` 3148、`target_angle` / `desired` 3312-3319）
- `src/features/aimbot/aimbot.hpp`、`aim_control.hpp`、`src/core/math/vector.hpp`、`src/core/input/input.cpp`

### 寫入 (Write / Edit)
- `src/features/aimbot/aimbot.cpp`：新增 include、`tick()` 重置 `aim.valid`、`desired` 後發佈 pitch / yaw / valid
- `agent/deep_todos.md`、`agent/memory.md`

### 刪除 (Delete)
- 無

### 驗證 (Verify)
- Select-String：include=1、publish=1、reset=1；`aimbot.cpp` 4181 行
- GitHub Actions Build 37216036575 = success（`vesta.exe` + `vesta_hvh.dll` + ctest）
- commit f3ce88b 推至 `myfork/main`（ac1bf93..f3ce88b）

## 2026-10-04 — T-011 CI 產出並打包 vesta_hvh.dll

### 讀取 (Read)
- `.github/workflows/build.yml`（34 行；upload-artifact path 僅 `build/bin/vesta.exe` + 法律文件）
- `.github/workflows/release.yml`（124 行；forum zip、checksum、attest、assets 清單均未含 DLL）

### 寫入 (Write / Edit)
- 新增暫存腳本 `C:\Users\yoyo2\AppData\Local\Temp\opencode\ci_hvh.ps1`（UTF8 no BOM、保留原換行、每個 anchor 斷言出現次數）：
  - `build.yml` artifact 清單 +`vesta_hvh.dll`（1 處）
  - `release.yml` archive 清單 +`vesta_hvh.dll`（1 處）
  - `release.yml` checksum 與 `$assets` 清單 +`vesta_hvh.dll`（2 處）
  - `release.yml` attest `subject-path` 改多行（1 處）
- 執行輸出：`build.yml artifact replaced 1` / `release.yml archive replaced 1` / `release.yml arrays replaced 2` / `release.yml attest replaced 1`。
- 交易：commit **d492848**「ci: ship vesta_hvh.dll in build artifacts and releases」，push `d1a0d44..d492848`（myfork/main）。

### 刪除 (Delete)
- 無。

### 驗證 (Verify)
- `Select-String` 確認：`build.yml:31` = `build/bin/vesta_hvh.dll`；`release.yml:39`（archive）、`:47`（checksum）、`:56-58`（attest subject-path 多行）、`:68`（assets）。
- 待 Build run 完成後確認 artifact `vesta-windows-x64` 內含 `vesta_hvh.dll`。


## 2026-10-05 — T-012 修正注入即崩潰：vtable hook

### 讀取 (Read)
- `%TEMP%\vesta_hvh.log`（顯示 hook 安裝兩次後無卸載訊息）
- `src/hvh/hvh_internal.hpp`、`src/hvh/dllmain.cpp`、`src/hvh/hvh_features.cpp`

### 寫入 (Write / Edit)
- 取代區塊備份：`C:\Users\yoyo2\AppData\Local\Temp\opencode\blk_internal.txt`、`blk_dllmain.txt`、`blk_features.txt`
- 修正腳本：`fix_vmt.ps1`（首次，guard 條件寫錯）、`fix_vmt2.ps1`（成功）
- `src/hvh/hvh_internal.hpp`：`inline_hook` → `vtable_hook` + `find_pointer_entry` 宣告
- `src/hvh/dllmain.cpp`：實作 `find_pointer_entry` 與 `vtable_hook::install` / `remove`
- `src/hvh/hvh_features.cpp`：`g_hook` 型別改為 `vtable_hook`；`initialize()` 改以 vtable slot 安裝 hook
- `agent/deep_todos.md`、`agent/memory.md`：本紀錄

### 刪除 (Delete)
- 移除 `src/hvh/dllmain.cpp` 的 `inline_hook::install` / `remove` 實作，及 `hvh_internal.hpp` 的 `class inline_hook`（以區塊取代方式）

### 驗證 (Verify)
- `fix_vmt2.ps1` 輸出：`spliced dllmain.cpp 274..335 -> 70 lines; now 378 lines`、`spliced hvh_features.cpp 157..172 -> 29 lines; now 206 lines`、`g_hook decl replaced: 1`
- 逐檔計數：hvh_internal.hpp `vtable_hook=2 inline_hook=0 find_pointer_entry=1`；dllmain.cpp `vtable_hook=2 inline_hook=0 find_pointer_entry=1`；hvh_features.cpp `vtable_hook=1 inline_hook=0 find_pointer_entry=1`
- 尚未編譯；待 CI Build 驗證
