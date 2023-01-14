// popup.h, Fernanda

#pragma once

#include "text.h"

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

    static void about(QWidget* parent, QString styleSheet = nullptr)
    {
        QMessageBox about;
        box(about, Text::about(), true, true);
        auto qt = about.addButton(tr("About Qt"), QMessageBox::AcceptRole);
        connect(qt, &QPushButton::clicked, parent, QApplication::aboutQt);
        about.exec();
    }

    static OnClose confirm(bool isQuit, QString styleSheet = nullptr)
    {
        QMessageBox alert;
        box(alert, Text::change(isQuit), false, false, "Hey!");
        alert.addButton(QMessageBox::Yes);
        auto no = alert.addButton(QMessageBox::No);
        auto save_and = alert.addButton(tr(Text::saveAndButtons(isQuit).toLocal8Bit()), QMessageBox::ActionRole);
        alert.setDefaultButton(no);
        alert.exec();
        if (alert.clickedButton() == no)
            return OnClose::Return;
        else if (alert.clickedButton() == save_and)
            return OnClose::SaveAndClose;
        else
            return OnClose::Close;
    }

    static void shortcuts(QString styleSheet = nullptr)
    {
        QMessageBox shortcuts;
        box(shortcuts, Text::shortcuts());
        shortcuts.exec();
    }

    static Action sample(QString styleSheet = nullptr)
    {
        QMessageBox alert;
        box(alert, Text::samples(), true, false, "Hey!");
        auto open = alert.addButton(tr(Text::openUdButton().toLocal8Bit()), QMessageBox::AcceptRole);
        alert.exec();
        if (alert.clickedButton() == open) return Action::Open;
        return Action::Accept;
    }

    static void update(Text::VersionCheck result, QString latestVersion, QString styleSheet = nullptr)
    {
        QMessageBox version_check;
        box(version_check, Text::version(result, latestVersion), true, true);
        version_check.exec();
    }

private:
    static void box(QMessageBox& box, QString text, bool hasOk = true, bool hasIcon = false, QString title = nullptr, QWidget* parent = nullptr, QString styleSheet = nullptr)
    {
        box.setStyleSheet(styleSheet);
        (title == nullptr)
            ? box.setWindowTitle("Fernanda")
            : box.setWindowTitle(title);
        box.setText(text);
        if (hasIcon)
            box.setIconPixmap(QPixmap(QStringLiteral(":/icons/fernanda_64.png")));
        if (hasOk)
        {
            auto ok = box.addButton(QMessageBox::Ok);
            box.setDefaultButton(ok);
        }
    }
};

// popup.h, Fernanda
