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

    enum class Has {
        RunOnStartUp,
        Self
    };
    enum class Run {
        None = 0,
        Green,
        Red,
        Pastels
    };

    void toggle(bool checked, Has has);
    void run(Run theme = Run::None);
    void delayedStartUp();

public slots:
    void setAlignment(QString alignment);
    bool hasStartUp();

private:
    QProgressBar* bar = new QProgressBar(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    QTimer* barTimer = new QTimer(this);

    bool hasSelf = true;
    bool hasRunOnStartUp = true;

    void style(Run theme);
};

// colorbar.h, Fernanda
