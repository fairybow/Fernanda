/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

 // Preview.h, Fernanda

#pragma once

#include "Layout.h"
#include "UserData.h"

#include <QDesktopServices>
#include <QEvent>
#include <QObject>
#include <Qt>
#include <QUrl>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QWidget>

#include <memory>

class WebDocument : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text MEMBER heldText NOTIFY textChanged FINAL)

public:
    explicit WebDocument(QObject* parent = nullptr) : QObject(parent) {}

    void setText(const QString& text)
    {
        if (text == heldText) return;
        heldText = text;
        textChanged(heldText);
    }

private:
    QString heldText;

signals:
    void textChanged(const QString& text);
};

class WebEnginePage : public QWebEnginePage
{
    Q_OBJECT

public:
    using QWebEnginePage::QWebEnginePage;

protected:
    bool acceptNavigationRequest(const QUrl& url, NavigationType, bool) override
    {
        if (url.scheme() == QStringLiteral("qrc")) return true;
        QDesktopServices::openUrl(url);
        return false;
    }
};

class WebEngineView : public QWebEngineView
{
    Q_OBJECT

public:
    WebEngineView(const QUrl& url, WebDocument& content, QWidget* parent = nullptr) : QWebEngineView(parent)
    {
        setContextMenuPolicy(Qt::NoContextMenu);
        channel->registerObject(QStringLiteral("content"), &content);
        page->setWebChannel(channel);
        setPage(page);
        setUrl(url);
    }

private:
    WebEnginePage* page = new WebEnginePage(this);
    QWebChannel* channel = new QWebChannel(this);
};

class Preview : public QWidget
{
    Q_OBJECT

    using StdFsPath = std::filesystem::path;

public:
    Preview(QWidget* parent = nullptr);

    void setText(const QString& text) { content.setText(text); }

public slots:
    void setType(QString typeName);

private:
    enum class Type {
        Fountain,
        Markdown
    };

    std::unique_ptr<WebEngineView> view;
    WebDocument content;
    Type type{};

    bool eventFilter(QObject* watched, QEvent* event);
    void check(bool isVisible);
    void refresh();

signals:
    void askEmitTextChanged();
};

// Preview.h, Fernanda
