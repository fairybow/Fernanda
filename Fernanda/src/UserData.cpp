/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/Fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// UserData.cpp, Fernanda

#include "UserData.h"

const UserData::StdFs::path UserData::doThis(Operation operation)
{
    auto user_data = Path::toStdFs(QDir::homePath()) / Path::toStdFs("." + dataVars.appName.toLower());
    auto active_temp = user_data / ".active_temp";
    auto backup = user_data / "backup";
    auto dll = user_data / "dll";
    auto rollback = backup / ".rollback";
    auto config = user_data / Path::toStdFs(dataVars.appName + ".ini");
    auto documents = Path::toStdFs(QStandardPaths::locate(QStandardPaths::DocumentsLocation, nullptr, QStandardPaths::LocateDirectory));
    auto user_documents = documents / "Fernanda";
    StdFs::path result;
    switch (operation) {
    case Operation::Config:
        result = config;
        break;
    case Operation::Create:
        for (const auto& data_folder : { user_data, active_temp, backup, dll, rollback, user_documents })
            Path::makeDirs(data_folder);
        result = StdFs::path();
        break;
    case Operation::GetActiveTemp:
        result = active_temp;
        break;
    case Operation::GetBackup:
        result = backup;
        break;
    case Operation::GetDLL:
        result = dll;
        break;
    case Operation::GetDocuments:
        result = user_documents;
        break;
    case Operation::GetRollback:
        result = rollback;
        break;
    case Operation::GetUserData:
        result = user_data;
        break;
    }
    return result;
}

void UserData::saveConfig(IniGroup group, IniValue valueType, QVariant value)
{
    auto config = Path::toQString(doThis(Operation::Config));
    QSettings ini(config, QSettings::IniFormat);
    ini.beginGroup(groupName(group));
    ini.setValue(valueName(valueType), value);
    ini.endGroup();
}

QVariant UserData::loadConfig(IniGroup group, IniValue valueType, QVariant fallback, UserData::Type type)
{
    auto config = doThis(Operation::Config);
    if (!QFile(config).exists()) return fallback;
    QSettings ini(Path::toQString(config), QSettings::IniFormat);
    auto group_name = groupName(group);
    auto value_name = valueName(valueType);
    if (!ini.childGroups().contains(group_name)) return fallback;
    ini.beginGroup(group_name);
    if (!ini.childKeys().contains(value_name)) return fallback;
    auto result = ini.value(value_name);
    auto bad_result = false;
    switch (type) {
    case UserData::Type::Bool:
        if (result != "true" && result != "false")
            bad_result = true;
        break;
    case UserData::Type::Int:
        if (result.toInt() < 1)
            bad_result = true;
        break;
    case UserData::Type::QRect:
        if (!result.canConvert<QRect>())
            bad_result = true;
        break;
    default: break;
    }
    ini.endGroup();
    if (bad_result) return fallback;
    return result;
}

const QString UserData::groupName(IniGroup group)
{
    QString result;
    switch (group) {
    case IniGroup::Data:
        result = "data";
        break;
    case IniGroup::Editor:
        result = "editor";
        break;
    case IniGroup::Preview:
        result = "preview";
        break;
    case IniGroup::Window:
        result = "window";
        break;
    }
    return result;
}

const QString UserData::valueName(IniValue valueType)
{
    QString result;
    switch (valueType) {
    case IniValue::AlwaysOnTop:
        result = "window__always_on_top";
        break;
    case IniValue::CharCount:
        result = "set_____character_count";
        break;
    case IniValue::ColorBarAlignment:
        result = "set_____color_bar_alignment";
        break;
    case IniValue::ColumnPosition:
        result = "set_____column_position";
        break;
    case IniValue::EditorFont:
        result = "set_____editor_font";
        break;
    case IniValue::EditorFontSize:
        result = "set_____editor_font_size";
        break;
    case IniValue::EditorTheme:
        result = "set_____editor_theme";
        break;
    case IniValue::LineCount:
        result = "set_____line_count";
        break;
    case IniValue::LinePosition:
        result = "set_____line_position";
        break;
    case IniValue::MostRecent:
        result = "data____most_recent_project";
        break;
    case IniValue::PreviewType:
        result = "set_____preview_type";
        break;
    case IniValue::SplitterPosition:
        result = "window__splitter_position";
        break;
    case IniValue::StayAwake:
        result = "window__stay_awake";
        break;
    case IniValue::TabStop:
        result = "set_____tab_stop_distance";
        break;
    case IniValue::ToggleColorBar:
        result = "toggle__color_bar";
        break;
    case IniValue::ToggleCursorBlink:
        result = "toggle__cursor_blink";
        break;
    case IniValue::ToggleCursorBlock:
        result = "toggle__cursor_block";
        break;
    case IniValue::ToggleEditorShadow:
        result = "toggle__editor_shadow";
        break;
    case IniValue::ToggleEditorTheme:
        result = "toggle__editor_theme";
        break;
    case IniValue::ToggleIndicator:
        result = "toggle__indicator";
        break;
    case IniValue::ToggleKeyFilters:
        result = "toggle__key_filters";
        break;
    case IniValue::ToggleLineNumberArea:
        result = "toggle__line_number_area";
        break;
    case IniValue::ToggleLineHighlight:
        result = "toggle__line_highlight";
        break;
    case IniValue::ToggleLoadMostRecent:
        result = "toggle__load_most_recent_project";
        break;
    case IniValue::TogglePane:
        result = "toggle__pane";
        break;
    case IniValue::TogglePreview:
        result = "toggle__preview";
        break;
    case IniValue::ToggleScrollsPrevNext:
        result = "toggle__scrolls_previous_next";
        break;
    case IniValue::ToggleStatusBar:
        result = "toggle__status_bar";
        break;
    case IniValue::ToggleToolAOT:
        result = "toggle__tool_always_on_top";
        break;
    case IniValue::ToggleToolSA:
        result = "toggle__tool_stay_awake";
        break;
    case IniValue::ToggleToolTimer:
        result = "toggle__tool_timer";
        break;
    case IniValue::ToggleWindowTheme:
        result = "toggle__window_theme";
        break;
    case IniValue::ToolTimer:
        result = "set_____tool_timer";
        break;
    case IniValue::WordCount:
        result = "set_____word_count";
        break;
    case IniValue::WrapMode:
        result = "set_____wrap_mode";
        break;
    case IniValue::WindowPosition:
        result = "window__window_position";
        break;
    case IniValue::WindowState:
        result = "window__window_state";
        break;
    case IniValue::WindowTheme:
        result = "set_____window_theme";
        break;
    }
    return result;
}

void UserData::clear(StdFs::path dirPath, bool clearSelf)
{
    for (const auto& item : StdFs::directory_iterator(dirPath))
        StdFs::remove_all(item);
    if (!clearSelf) return;
    StdFs::remove(dirPath);
}

QString UserData::timestamp()
{
    const time_t now = std::time(0);
    return QString::fromLocal8Bit(std::ctime(&now));
}

#ifdef Q_OS_LINUX

std::string UserData::dll()
{
    const auto search_paths = QStringList{ "/lib", "/usr/lib", "/usr/local/lib" } << qEnvironmentVariable("LD_LIBRARY_PATH").split(',');
    for (const auto& search_path : search_paths)
    {
        for (const auto& library_name : { "7z.so", "p7zip/7z.so" })
        {
            const auto candidate = StdFs::path{ search_path.toStdString() } / library_name;
            if (StdFs::exists(candidate))
                return candidate;
        }
    }
    throw std::runtime_error("Unable to locate shared 7z library. Have you installed all dependencies?");
}

#else

std::string UserData::dll()
{
    auto dll_path = doThis(Operation::GetDLL) / "7z.dll";
    if (!QFile(dll_path).exists())
        Path::copy(StdFs::path(":/lib/7zip/7z64.dll"), dll_path);
    return dll_path.string();
}

#endif

// UserData.cpp, Fernanda
