/*
*   Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
*   Copyright(C) 2022 - 2023  @fairybow (https://github.com/fairybow)
*
*   https://github.com/fairybow/fernanda
*
*   You should have received a copy of the GNU General Public License
*   along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

// userdata.h, Fernanda

#pragma once

#include "path.h"

#include <ctime>

#include <QCoreApplication>
#include <QFile>
#include <QRect>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QVariant>

namespace UserData
{
    namespace StdFs = std::filesystem;

    enum class IniGroup {
        Data,
        Editor,
        Window
    };

    enum class IniValue {
        AlwaysOnTop,
        CharCount,
        ColorBarAlignment,
        ColumnPosition,
        EditorFont,
        EditorFontSize,
        EditorTheme,
        LineCount,
        LinePosition,
        MostRecent,
        SplitterPosition,
        StayAwake,
        TabStop,
        ToggleColorBar,
        ToggleCursorBlink,
        ToggleCursorBlock,
        ToggleEditorShadow,
        ToggleEditorTheme,
        ToggleIndicator,
        ToggleKeyFilters,
        ToggleLineNumberArea,
        ToggleLineHighlight,
        ToggleLoadMostRecent,
        TogglePane,
        ToggleScrollsPrevNext,
        ToggleStatusBar,
        ToggleToolAOT,
        ToggleToolSA,
        ToggleToolTimer,
        ToggleWindowTheme,
        ToolTimer,
        WordCount,
        WrapMode,
        WindowPosition,
        WindowState,
        WindowTheme,
    };

    enum class Operation {
        Config,
        Create,
        GetActiveTemp,
        GetBackup,
        GetDLL,
        GetDocuments,
        GetRollback,
        GetUserData
    };

    enum class Type {
        Bool,
        Int,
        QRect,
        QVariant
    };

    inline struct DataVars {
        QString appName;
    } dataVars;

    const StdFs::path doThis(Operation operation = Operation::Create);
    void saveConfig(IniGroup group, IniValue valueType, QVariant value);
    QVariant loadConfig(IniGroup group, IniValue valueType, QVariant fallback = QVariant(), UserData::Type type = UserData::Type::QVariant);
    const QString groupName(IniGroup group);
    const QString valueName(IniValue valueType);
    void clear(StdFs::path dirPath, bool clearSelf = false);
    QString timestamp();
    std::string dll();

    inline void setName(QString name) { dataVars.appName = name.toLower(); }
}

// userdata.h, Fernanda
