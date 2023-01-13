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
        about.setStyleSheet(styleSheet);
        about.setWindowTitle("Fernanda");
        about.setText(Text::about());
        about.setIconPixmap(QPixmap(QStringLiteral(":/icons/fernanda_64.png")));
        auto ok = about.addButton(QMessageBox::Ok);
        auto qt = about.addButton(tr("About Qt"), QMessageBox::AcceptRole);
        connect(qt, &QPushButton::clicked, parent, QApplication::aboutQt);
        about.setDefaultButton(ok);
        about.exec();
    }

    static OnClose confirm(bool isQuit, QString styleSheet = nullptr)
    {
        QMessageBox alert;
        alert.setStyleSheet(styleSheet);
        alert.setWindowTitle("Hey!");
        alert.setText(Text::change(isQuit));
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
        shortcuts.setStyleSheet(styleSheet);
        shortcuts.setWindowTitle("Fernanda");
        shortcuts.setText(Text::shortcuts());
        auto ok = shortcuts.addButton(QMessageBox::Ok);
        shortcuts.setDefaultButton(ok);
        shortcuts.exec();
    }

    static Action sample(QString styleSheet = nullptr)
    {
        QMessageBox alert;
        alert.setStyleSheet(styleSheet);
        alert.setWindowTitle("Hey!");
        alert.setText(Text::samples());
        auto ok = alert.addButton(QMessageBox::Ok);
        auto open = alert.addButton(tr(Text::openUdButton().toLocal8Bit()), QMessageBox::AcceptRole);
        alert.setDefaultButton(ok);
        alert.exec();
        if (alert.clickedButton() == open) return Action::Open;
        return Action::Accept;
    }

    static void update(Text::Version result, QString latestVersion, QString styleSheet = nullptr)
    {
        QMessageBox check;
        check.setStyleSheet(styleSheet);
        check.setWindowTitle("Fernanda");
        check.setText(Text::version(result, latestVersion));
        check.setIconPixmap(QPixmap(QStringLiteral(":/icons/fernanda_64.png")));
        auto ok = check.addButton(QMessageBox::Ok);
        check.setDefaultButton(ok);
        check.exec();
    }
};

// popup.h, Fernanda
