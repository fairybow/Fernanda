/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

 // preview.h, Fernanda

#pragma once

#include "layout.h"

#include <QDesktopServices>
#include <QEvent>
#include <QObject>
#include <Qt>
#include <QWebChannel>
#include <QWebEnginePage>
//#include <QWebEngineProfile>
//#include <QWebEngineScriptCollection>
#include <QWebEngineView>
#include <QWidget>

#include <memory>

class WebDocument : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text MEMBER held_text NOTIFY textChanged FINAL)

public:
    explicit WebDocument(QObject* parent = nullptr) : QObject(parent) {}

    void setText(const QString& text)
    {
        if (text == held_text) return;
        held_text = text;
        textChanged(held_text);
    }

private:
    QString held_text;

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
        if (url.scheme() == QString("qrc")) return true;
        QDesktopServices::openUrl(url);
        return false;
    }
};

class Preview : public QWidget
{
    Q_OBJECT

public:
    Preview(QWidget* parent = nullptr);

    void setText(const QString& text);

private:
    std::unique_ptr<QWebEngineView> view;
    WebDocument content;

    bool eventFilter(QObject* watched, QEvent* event);
    void check(bool isVisible);
    void open();
};

// preview.h, Fernanda
