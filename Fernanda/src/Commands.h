#pragma once

namespace Fernanda::Commands {

/// * = registered

constexpr auto QUIT = "application:quit";
constexpr auto ABOUT_DIALOG = "application:about_dialog"; /// *

constexpr auto NEW_NOTEBOOK = "workspace:new_notebook";
constexpr auto OPEN_NOTEBOOK = "workspace:open_notebook";
constexpr auto CLOSE_WINDOW = "workspace:close_window";

constexpr auto NEW_WINDOW = "windows:new"; /// *
constexpr auto ACTIVE_WINDOW = "windows:active"; /// *, non-menu
constexpr auto WINDOWS_SET = "windows:set"; /// *, non-menu

} // namespace Fernanda::Commands
