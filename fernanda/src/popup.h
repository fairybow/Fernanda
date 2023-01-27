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

    static void about(QWidget* parent);
    static OnClose confirm(bool isQuit);
    static void shortcuts();
    static Action sample();
    static void update(Text::VersionCheck result, QString latestVersion);
    static void timeUp();
    static void totalCounts(int lines, int words, int characters);

private:
    static void box(QMessageBox& box, QString text, bool hasOk = true, bool hasIcon = false, QString title = nullptr, QWidget* parent = nullptr);
};

// popup.h, Fernanda
