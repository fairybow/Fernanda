// userdata.cpp, Fernanda

#include "userdata.h"

void Ud::setName(QString name)
{
    dataVars.appName = name.toLower();
}

const Ud::Fs::path Ud::userData(Op operation)
{
    auto user_data = Path::toFs(QDir::homePath()) / Path::toFs("." + dataVars.appName);
    auto active_temp = user_data / ".active_temp";
    auto backup = user_data / "backup";
    auto dll = user_data / "dll";
    auto rollback = backup / ".rollback";
    auto config = user_data / Path::toFs(dataVars.appName + ".ini");
    auto docs = Path::toFs(QStandardPaths::locate(QStandardPaths::DocumentsLocation, nullptr, QStandardPaths::LocateDirectory));
    auto user_docs = docs / "Fernanda";
    Fs::path result;
    switch (operation) {
    case Op::Config:
        result = config;
        break;
    case Op::Create:
        for (const auto& data_folder : { user_data, active_temp, backup, dll, rollback, user_docs })
            Path::makeDirs(data_folder);
        result = Fs::path();
        break;
    case Op::GetBackup:
        result = backup;
        break;
    case Op::GetDLL:
        result = dll;
        break;
    case Op::GetDocs:
        result = user_docs;
        break;
    case Op::GetRollback:
        result = rollback;
        break;
    case Op::GetTemp:
        result = active_temp;
        break;
    case Op::GetUserData:
        result = user_data;
        break;
    }
    return result;
}

void Ud::saveConfig(ConfigGroup group, ConfigVal valueType, QVariant value)
{
    auto config = Path::toQString(userData(Op::Config));
    QSettings ini(config, QSettings::IniFormat);
    ini.beginGroup(groupName(group));
    ini.setValue(valueName(valueType), value);
    ini.endGroup();
}

QVariant Ud::loadConfig(ConfigGroup group, ConfigVal valueType, QVariant fallback, Ud::Type type)
{
    auto config = userData(Op::Config);
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
    case Ud::Type::Bool:
        if (result != "true" && result != "false")
            bad_result = true;
        break;
    case Ud::Type::Int:
        if (result.toInt() < 1)
            bad_result = true;
        break;
    case Ud::Type::QRect:
        if (!result.canConvert<QRect>())
            bad_result = true;
        break;
    default: break;
    }
    ini.endGroup();
    if (bad_result) return fallback;
    return result;
}

const QString Ud::groupName(ConfigGroup group)
{
    QString result;
    switch (group) {
    case ConfigGroup::Data:
        result = "data";
        break;
    case ConfigGroup::Editor:
        result = "editor";
        break;
    case ConfigGroup::Window:
        result = "window";
        break;
    }
    return result;
}

const QString Ud::valueName(ConfigVal valueType)
{
    QString result;
    switch (valueType) {
    case ConfigVal::Aot:
        result = "always_on_top";
        break;
    case ConfigVal::BarAlign:
        result = "bar_alignment";
        break;
    case ConfigVal::CountChar:
        result = "count_characters";
        break;
    case ConfigVal::CountLine:
        result = "count_lines";
        break;
    case ConfigVal::CountWord:
        result = "count_words";
        break;
    case ConfigVal::EditorTheme:
        result = "theme";
        break;
    case ConfigVal::Font:
        result = "font";
        break;
    case ConfigVal::FontSlider:
        result = "font_size";
        break;
    case ConfigVal::PosCol:
        result = "position_column";
        break;
    case ConfigVal::PosLine:
        result = "position_line";
        break;
    case ConfigVal::Position:
        result = "position";
        break;
    case ConfigVal::Project:
        result = "project";
        break;
    case ConfigVal::Splitter:
        result = "splitter";
        break;
    case ConfigVal::State:
        result = "state";
        break;
    case ConfigVal::T_AotBtn:
        result = "aot_button";
        break;
    case ConfigVal::T_ColorBar:
        result = "color_bar";
        break;
    case ConfigVal::T_CursorBlink:
        result = "cursor_blink";
        break;
    case ConfigVal::T_Cursor:
        result = "block_cursor";
        break;
    case ConfigVal::T_EditorTheme:
        result = "theme_on";
        break;
    case ConfigVal::T_Indicator:
        result = "indicator";
        break;
    case ConfigVal::T_Keyfilter:
        result = "key_filter";
        break;
    case ConfigVal::T_Lmr:
        result = "load_most_recent";
        break;
    case ConfigVal::T_Lna:
        result = "line_number_area";
        break;
    case ConfigVal::T_LineHighlight:
        result = "line_highlight";
        break;
    case ConfigVal::T_Nav:
        result = "nav_scrolls";
        break;
    case ConfigVal::T_Pane:
        result = "pane";
        break;
    case ConfigVal::T_Shadow:
        result = "shadow";
        break;
    case ConfigVal::T_StatusBar:
        result = "statusbar";
        break;
    case ConfigVal::T_WinTheme:
        result = "wintheme_on";
        break;
    case ConfigVal::TabStop:
        result = "tab";
        break;
    case ConfigVal::WinTheme:
        result = "wintheme";
        break;
    case ConfigVal::Wrap:
        result = "wrap";
        break;
    }
    return result;
}

void Ud::clear(Fs::path dirPath, bool clearSelf)
{
    for (const auto& item : Fs::directory_iterator(dirPath))
        Fs::remove_all(item);
    if (!clearSelf) return;
    Fs::remove(dirPath);
}

QString Ud::timestamp()
{
    const time_t now = std::time(0);
    return QString::fromLocal8Bit(std::ctime(&now));
}

#ifdef Q_OS_LINUX

std::string Ud::dll()
{
    const auto search_paths = QStringList{ "/lib", "/usr/lib", "/usr/local/lib" } << qEnvironmentVariable("LD_LIBRARY_PATH").split(',');
    for (const auto& search_path : search_paths)
    {
        for (const auto& library_name : { "7z.so", "p7zip/7z.so" })
        {
            const auto candidate = std::filesystem::path{ search_path.toStdString() } / library_name;
            if (std::filesystem::exists(candidate))
                return candidate;
        }
    }
    throw std::runtime_error("Unable to locate shared 7z library. Have you installed all dependencies?");
}

#else

const std::string Ud::dll()
{
    auto dll_path = userData(Op::GetDLL) / "7z.dll";
    if (!QFile(dll_path).exists())
        Fs::copy_file(Fs::path(":/lib/7zip/7z64.dll"), dll_path);
    return dll_path.string();
}

#endif

// userdata.cpp, Fernanda
