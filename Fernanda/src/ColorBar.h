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

// ColorBar.h, Fernanda

#pragma once

#include "Io.h"
#include "UserData.h"

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

    void delayedStartUp() { QTimer::singleShot(1500, this, [&]() { run(Run::Pastels); }); }

public slots:
    void setAlignment(QString alignment);

    bool hasStartUp() { return hasRunOnStartUp; }

private:
    QProgressBar* bar = new QProgressBar(this);
    QVBoxLayout* layout = new QVBoxLayout(this);
    QTimer* barTimer = new QTimer(this);

    bool hasSelf = true;
    bool hasRunOnStartUp = true;

    void style(Run theme);
};

// ColorBar.h, Fernanda
