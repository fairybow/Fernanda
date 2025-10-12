#pragma once

namespace Fernanda::Commands {

/// * = registered

constexpr auto QUIT = "application:quit";

// TODO: Should be WindowService registers
constexpr auto NEW_WINDOW = "workspace:new_window";
constexpr auto NEW_NOTEBOOK = "workspace:new_notebook";
constexpr auto OPEN_NOTEBOOK = "workspace:open_notebook";
constexpr auto CLOSE_WINDOW = "workspace:close_window";

// TODO: Should be application scope (change here and docs) but registered in WS
constexpr auto ABOUT_DIALOG = "workspace:about_dialog"; /// *

} // namespace Fernanda::Commands
