/*
 * Suzuri — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QFont>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QString>

#include <Coco/Debug.h>

namespace Suzuri {

class LogView : public QPlainTextEdit
{
    Q_OBJECT

public:
    LogView()
        : QPlainTextEdit(nullptr)
    {
        setReadOnly(true);
        setFont(QFont("Consolas", 9));
        setStyleSheet(R"(
                QPlainTextEdit {
                    background-color: #1a1a1a;
                    color: #e0e0e0;
                    selection-background-color: #444;
                }
        )");
        setLineWrapMode(QPlainTextEdit::WidgetWidth);
        resize(800, 500);

        setAttribute(Qt::WA_DeleteOnClose);
        setWindowFlag(Qt::Window);

        Coco::Debug::setLogSink([this](const QString& msg) {
            QMetaObject::invokeMethod(
                this,
                [this, msg] { appendPlainText(msg); },
                Qt::QueuedConnection);
        });
    }

    ~LogView() override
    {
        TRACER;
        Coco::Debug::setLogSink(nullptr);
    }
};

} // namespace Suzuri
