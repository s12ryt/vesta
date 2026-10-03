#pragma once

#include <string_view>

namespace render::localization
{

enum class id : int
{
    en = 0,
    ru = 1,
    zh = 2,
    zh_hant = 3,
    count
};

void set(id value);
[[nodiscard]] id current();

// Short label for the language switch itself ("EN" / "RU" / "ZH" / "TW").
[[nodiscard]] const char *code(id value);

[[nodiscard]] const char *tr(const char *english);

// Same, for text built at runtime. Only the fixed fragments are looked up.
[[nodiscard]] std::string_view tr(std::string_view english);

// Every Chinese string the UI can emit, concatenated. Used to build the exact
// glyph range merged into the menu/HUD fonts so both Simplified and Traditional
// text render without loading the whole CJK atlas.
[[nodiscard]] const char *cjk_glyph_text();

} // namespace render::localization
