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

 // statusbarbutton.h, Fernanda

#pragma once

#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QPushButton>

class StatusBarButton : public QPushButton
{
    Q_OBJECT

public:
    StatusBarButton(QWidget* parent = nullptr)
        : QPushButton(parent)
    {
        setObjectName(QStringLiteral("statusBarButton"));
        installEventFilter(this);
        opacity->setOpacity(0.4);
        setGraphicsEffect(opacity);
        connect(this, &StatusBarButton::toggled, this, [&](bool checked) { opacity->setEnabled(!opacity->isEnabled()); });
    }

private:
    QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect(this);

    bool eventFilter(QObject* watched, QEvent* event)
    {
        StatusBarButton* button = qobject_cast<StatusBarButton*>(watched);
        if (!button) return false;
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave)
        {
            opacity->setEnabled(!opacity->isEnabled());
            return true;
        }
        return false;
    }

};

// statusbarbutton.h, Fernanda
