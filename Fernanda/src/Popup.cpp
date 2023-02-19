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

// Popup.cpp, Fernanda

#include "Popup.h"

bool Popup::about(QWidget* parent)
{
    auto result = false;
    QMessageBox about;
    box(about, Text::about(), true, true);
    auto update = about.addButton(tr(Text::checkForUpdates().toLocal8Bit()), QMessageBox::AcceptRole);
    auto qt = about.addButton(tr("About Qt"), QMessageBox::AcceptRole);
    connect(qt, &QPushButton::clicked, parent, QApplication::aboutQt);
    about.exec();
    if (about.clickedButton() == update) result = true;
    return result;
}

Popup::OnClose Popup::confirm(bool isQuit)
{
    auto result = OnClose::Close;
    QMessageBox alert;
    box(alert, Text::change(isQuit), false, false, "Hey!");
    alert.addButton(QMessageBox::Yes);
    auto no = alert.addButton(QMessageBox::No);
    auto save_and = alert.addButton(tr(Text::saveAndButtons(isQuit).toLocal8Bit()), QMessageBox::ActionRole);
    alert.setDefaultButton(no);
    alert.exec();
    if (alert.clickedButton() == no) result = OnClose::Return;
    else if (alert.clickedButton() == save_and) result = OnClose::SaveAndClose;
    return result;
}

void Popup::shortcuts()
{
    QMessageBox shortcuts;
    box(shortcuts, Text::shortcuts());
    shortcuts.exec();
}

Popup::Action Popup::sample()
{
    QMessageBox alert;
    box(alert, Text::samples(), true, false, "Hey!");
    auto open = alert.addButton(tr(Text::openUdButton().toLocal8Bit()), QMessageBox::AcceptRole);
    alert.exec();
    if (alert.clickedButton() == open) return Action::Open;
    return Action::Accept;
}

void Popup::update(Text::VersionCheck result, QString latestVersion)
{
    QMessageBox version_check;
    box(version_check, Text::version(result, latestVersion), true, true);
    version_check.exec();
}

void Popup::timeUp()
{
    QMessageBox time_up;
    box(time_up, Text::timeUp());
    time_up.exec();
}

void Popup::totalCounts(int lines, int words, int characters)
{
    QMessageBox total_counts;
    box(total_counts, Text::totalCounts(lines, words, characters));
    total_counts.exec();
}

void Popup::box(QMessageBox& box, QString text, bool hasOk, bool hasIcon, QString title, QWidget* parent)
{
    (title == nullptr)
        ? box.setWindowTitle("Fernanda")
        : box.setWindowTitle(title);
    box.setText(text);
    if (hasIcon)
        box.setIconPixmap(QPixmap(QStringLiteral(":/icons/Fernanda_64.png")));
    if (hasOk)
    {
        auto ok = box.addButton(QMessageBox::Ok);
        box.setDefaultButton(ok);
    }
}

// Popup.cpp, Fernanda
