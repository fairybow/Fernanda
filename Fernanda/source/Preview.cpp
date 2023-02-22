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

// Preview.cpp, Fernanda

#include "Preview.h"

Preview::Preview(QWidget* parent)
    : QWidget(parent)
{
    installEventFilter(this);
    setContentsMargins(0, 0, 0, 0);
}

void Preview::toggle(bool checked, Has has)
{
    switch (has) {
    case Has::ScrollSync:
        hasScrollSync = checked;
        UserData::saveConfig(UserData::IniGroup::Preview, UserData::IniValue::ToggleScrollSync, checked);
        break;
    }
    askEmitTextChanged();
}

void Preview::setType(QString typeName)
{
    (typeName == "Fountain") ? type = Type::Fountain : type = Type::Markdown;
    UserData::saveConfig(UserData::IniGroup::Preview, UserData::IniValue::PreviewType, typeName);
    refresh();
}

void Preview::scrollToBlock(int blockNumber)
{
    auto view_get = view.get();
    if (view_get == nullptr || !hasScrollSync) return;
    view_get->scrollToBlock(blockNumber);
}

bool Preview::eventFilter(QObject* watched, QEvent* event)
{
    Preview* object = qobject_cast<Preview*>(watched);
    if (!object) return false;
    if (event->type() == QEvent::Hide || event->type() == QEvent::Show)
    {
        check(isVisible());
        return true;
    }
    else if (event->type() == QEvent::Resize)
    {
        if (size().width() <= 30)
            refresh();
        return true;
    }
    return false;
}

void Preview::check(bool isVisible)
{
    if ((isVisible || size().width() > 15) && view.get() == nullptr)
    {
        QString url = (type == Type::Fountain) ? "qrc:/preview/fountain.html" : "qrc:/preview/markdown.html";
        view = std::unique_ptr<WebEngineView>(new WebEngineView(url, content, this));
        setLayout(Layout::stackLayout(view.get(), this));
        askEmitTextChanged();
    }
    else if ((!isVisible || size().width() <= 14) && view.get() != nullptr)
    {
        setText(nullptr);
        view.get()->deleteLater(); // unconvinced that this is working as intended
        view.reset();
        delete layout();
    }
}

void Preview::refresh()
{
    if (!isVisible()) return;
    hide();
    show();
}

// Preview.cpp, Fernanda
