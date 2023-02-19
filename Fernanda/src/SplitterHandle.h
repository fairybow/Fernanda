/*
 *  Fernanda is a plain text editor for drafting long-form fiction. (At least, that's the plan.)
 *  Copyright (C) 2022-2023 @fairybow <https://github.com/fairybow>
 *
 *  <https://github.com/fairybow/Fernanda>
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

// SplitterHandle.h, Fernanda

#pragma once

#include <QEasingCurve>
#include <QEvent>
#include <QPropertyAnimation>
#include <QSplitterHandle>
#include <QTimer>

class SplitterHandle : public QSplitterHandle
{
    Q_OBJECT

public:
    SplitterHandle(Qt::Orientation orientation, QSplitter* parent)
        : QSplitterHandle(orientation, parent)
    {
        installEventFilter(this);
        expanding->setDuration(100);
        expanding->setEasingCurve(QEasingCurve::OutQuad);
        expanding->setStartValue(splitter()->handleWidth());
        expanding->setEndValue(splitter()->handleWidth() * 1.5);
        /*connect(expandTimer, &QTimer::timeout, this, [&]()
            {
                askCollapseOrExpand(this);
                expandTimer->stop();
            });*/
    }

private:
    QPropertyAnimation* expanding = new QPropertyAnimation(this, "minimumWidth");
    //QTimer* expandTimer = new QTimer(this);

    bool eventFilter(QObject* watched, QEvent* event)
    {
        SplitterHandle* object = qobject_cast<SplitterHandle*>(watched);
        auto result = false;
        if (!object) return result;
        switch (event->type()) {
        case QEvent::Enter:
            expanding->start();
            result = true;
            break;
        case QEvent::Leave:
            expanding->stop();
            setFixedWidth(splitter()->handleWidth());
            result = true;
            break;
        case QEvent::MouseButtonDblClick:
            expanding->stop();
            askStoreWidths();
            askToggleExpansion(this);
            result = true;
            break;
        }
        return result;
    }

signals:
    void askStoreWidths();
    void askToggleExpansion(SplitterHandle* handlePointer);
};

// SplitterHandle.h, Fernanda
