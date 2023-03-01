/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/Fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// Popup.h, Fernanda

#pragma once

#include "Text.h"

#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>

class Popup : public QObject
{
    Q_OBJECT

public:
    enum class Action {
        Accept,
        Open
    };
    enum class OnClose {
        Close,
        Return,
        SaveAndClose
    };

    static bool about(QWidget* parent);
    static OnClose confirm(QWidget* parent, bool isQuit);
    static void shortcuts(QWidget* parent);
    static Action sample(QWidget* parent);
    static void update(QWidget* parent, Text::VersionCheck result, QString latestVersion);
    static void timeUp(QWidget* parent);
    static void totalCounts(QWidget* parent, int lines, int words, int characters);

private:
    static void box(QMessageBox& box, QString text, bool hasOk = true, bool hasIcon = false, QString title = nullptr);
};

// Popup.h, Fernanda
