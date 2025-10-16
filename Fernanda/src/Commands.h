#pragma once

// TODO: Compare with Bus.md and find what's missing
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

constexpr auto RUN_COLOR_BAR = "color_bar:run"; /// *, non-menu
constexpr auto BE_CUTE = "color_bar:be_cute"; /// *, non-menu

} // namespace Fernanda::Commands
