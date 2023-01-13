// colorbar.h, Fernanda

#pragma once

#include "io.h"
#include "userdata.h"

#include <QProgressBar>
#include <QTimeLine>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class ColorBar : public QWidget
{
    Q_OBJECT

public:
    ColorBar(QWidget* parent = nullptr);

    enum class Run {
        None = 0,
        Green,
        Red,
        Pastels
    };

    void run(Run theme = Run::None);
    void delayedStartUp();

public slots:
    void toggleSelf(bool checked);
    void setAlignment(QString alignment);
    bool hasStartUp();
    void toggleStartUp(bool checked);

private:
    QProgressBar* bar = new QProgressBar(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    QTimer* barTimer = new QTimer(this);

    bool hasSelf = true;
    bool runOnStartUp = true;

    void style(Run theme);
};

// colorbar.h, Fernanda
