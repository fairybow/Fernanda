/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

 // preview.cpp, Fernanda

#include "preview.h"

Preview::Preview(QWidget* parent)
    : QWidget(parent)
{
    installEventFilter(this);
    setContentsMargins(0, 0, 0, 0);
}

void Preview::setText(const QString& text)
{
    content.setText(text);
}

bool Preview::eventFilter(QObject* watched, QEvent* event)
{
    Preview* widget = qobject_cast<Preview*>(watched);
    if (!widget) return false;
    if (event->type() == QEvent::Hide || event->type() == QEvent::Show)
    {
        check(isVisible());
        return true;
    }
    return false;
}

void Preview::check(bool isVisible)
{
    if (isVisible && view.get() == nullptr)
    {
        view = std::unique_ptr<QWebEngineView>(new QWebEngineView(this));
        open();
    }
    else if (!isVisible && view.get() != nullptr)
    {
        //QWebEngineProfile::defaultProfile()->clearHttpCache();
        //QWebEngineProfile::defaultProfile()->scripts()->clear();
        delete layout();
        view.reset();
    }
}

void Preview::open()
{
    auto view_value = view.get();
    setLayout(Layout::stackLayout(view_value, this));
    view_value->setContextMenuPolicy(Qt::NoContextMenu);
    WebEnginePage* page = new WebEnginePage(view_value);
    view_value->setPage(page);
    QWebChannel* channel = new QWebChannel(view_value);
    channel->registerObject(QStringLiteral("content"), &content);
    page->setWebChannel(channel);
    view_value->setUrl(QUrl("qrc:/preview/index.html"));
}

// preview.cpp, Fernanda
