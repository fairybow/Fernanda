/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
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

#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>

#include "core/Publication.h"
#include "core/Version.h"
#include "ui/LicensesDialog.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// TODO: Ctor param to show App name or not (for Splash)? Might be something
// that is just handled outside?
class AboutPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AboutPanel(QWidget* parent)
        : QWidget(parent)
    {
        setup_();
    }

    ~AboutPanel() override { TRACER; }

private:
    static constexpr auto ICON_PATH_ = ":/icons/Fernanda-128.png";
    static constexpr auto ICON_SIZE_ = 128;

    void setup_()
    {
        auto root_layout = new QVBoxLayout(this);
        root_layout->setContentsMargins(0, 0, 0, 0);
        root_layout->setSpacing(12);
        root_layout->addWidget(buildIconLabel_());
        root_layout->addWidget(buildVersionLabel_());
        root_layout->addWidget(buildLicensesLink_());
    }

    QLabel* buildIconLabel_()
    {
        auto label = new QLabel(this);
        label->setAlignment(Qt::AlignCenter);
        label->setPixmap(QPixmap(ICON_PATH_)
                             .scaled(
                                 ICON_SIZE_,
                                 ICON_SIZE_,
                                 Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation));

        return label;
    }

    QLabel* buildVersionLabel_()
    {
        auto text = u"%1<br><i>%2</i>"_s.arg(
            VERSION_FULL_QSTRING.toHtmlEscaped(),
            PUB_RELEASE_NAME_QSTRING.toHtmlEscaped());

        auto label = new QLabel(text, this);
        label->setTextFormat(Qt::RichText);
        label->setAlignment(Qt::AlignCenter);
        label->setEnabled(false);

        return label;
    }

    // Opens the licenses dialog over this panel's window
    QLabel* buildLicensesLink_()
    {
        auto label = new QLabel(
            u"<a href=\"licenses\">%1</a>"_s.arg(tr("Licenses")),
            this);
        label->setAlignment(Qt::AlignCenter);
        label->setTextFormat(Qt::RichText);

        connect(label, &QLabel::linkActivated, this, [this] {
            (new LicensesDialog(window()))->open();
        });

        return label;
    }
};

} // namespace Fernanda::Ui
