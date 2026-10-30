/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 * <https://fernanda.fairybow.com>
 * <https://github.com/fairybow/Fernanda>
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <functional>
#include <utility>

#include <QCheckBox>
#include <QCoreApplication>
#include <QMessageBox>
#include <QWidget>

// The warning a beta build shows at launch: not to trust writing entirely to
// a beta, and to keep backups. Its box mutes it for the running version only,
// so each new beta warns once more. App decides whether it shows
// (App::maybeShowBetaAlert_). A free function, as LossyOpenPrompt is
namespace Fernanda::BetaAlert {

// Window-modal over parent, opened rather than run, so launch carries on
// beneath it. It deletes itself when closed. muted runs when it closes with
// its box checked
inline void open(QWidget* parent, std::function<void()> muted)
{
    auto* box = new QMessageBox(parent);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->setIcon(QMessageBox::Warning);
    box->setWindowModality(parent ? Qt::WindowModal : Qt::ApplicationModal);
    box->setTextFormat(Qt::PlainText);

    box->setWindowTitle(
        QCoreApplication::translate("BetaAlert", "Fernanda is in beta"));

    box->setText(
        QCoreApplication::translate(
            "BetaAlert",
            "Don't trust your writing entirely to beta versions of Fernanda. "
            "Still, whatever version you use, keep regular backups of your "
            "work."));

    box->setCheckBox(new QCheckBox(
        QCoreApplication::translate(
            "BetaAlert",
            "Don't show this again for this version"),
        box));

    box->setStandardButtons(QMessageBox::Ok);

    QObject::connect(
        box,
        &QMessageBox::finished,
        box,
        [box, muted = std::move(muted)] {
            if (box->checkBox()->isChecked()) {
                muted();
            }
        });

    box->open();
}

} // namespace Fernanda::BetaAlert
