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

#include <QAbstractButton>
#include <QEnterEvent>
#include <QEvent>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QMessageBox>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QToolTip>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Path.h>
#include <Coco/Time.h>

#include "core/git/GitRepository.h"
#include "ui/UiConstants.h"
#include "ui/widgets/ButtonWash.h"
#include "ui/widgets/Glyph.h"
#include "ui/widgets/HoverSync.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

// The Git page's sync button: one button that does what the branch needs, as
// GitHub Desktop's does, in this order: publish a branch that has no upstream,
// or whose upstream is gone from the remote; pull when the remote has commits
// this branch doesn't; push when this branch has commits the remote doesn't;
// otherwise fetch (GitRepository::sync). It shows the action's glyph, and the
// counts beside it: "↓2" to pull, "↑1" to push, both when the two have moved
// apart. Its tooltip names the action and the remote and says when the
// repository last fetched, worked out as it shows, so it's never stale; a
// fetch in a terminal counts too.
//
// It does nothing until clicked: Fernanda never fetches on its own. While a
// sync runs, the loader glyph turns; a failure shows git's words. Disabled
// while the repository is busy with any operation, and when it has no remote
class GitSyncButton : public QAbstractButton
{
    Q_OBJECT

public:
    GitSyncButton(GitRepository* repository, QWidget* parentGitPage)
        : QAbstractButton(parentGitPage)
        , repository_(repository)
    {
        setup_();
    }

    ~GitSyncButton() override { TRACER; }

    [[nodiscard]] QSize sizeHint() const override
    {
        auto width = 2 * GIT_SYNC_BUTTON_PADDING + GIT_SYNC_BUTTON_ICON_EXTENT;

        if (!text().isEmpty()) {
            width += GIT_SYNC_BUTTON_SPACING +
                     fontMetrics().horizontalAdvance(text());
        }

        return { width, VAULT_SWITCHER_HEIGHT };
    }

    [[nodiscard]] QSize minimumSizeHint() const override { return sizeHint(); }

protected:
    bool event(QEvent* event) override
    {
        if (event->type() == QEvent::ToolTip) {
            QToolTip::showText(
                static_cast<QHelpEvent*>(event)->globalPos(),
                toolTipText_(),
                this);
            return true;
        }

        return QAbstractButton::event(event);
    }

    void enterEvent(QEnterEvent* event) override
    {
        QAbstractButton::enterEvent(event);
        update(); // show the wash on hover
    }

    void leaveEvent(QEvent* event) override
    {
        QAbstractButton::leaveEvent(event);
        update(); // drop the wash when the pointer leaves
    }

    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        ButtonWash::paint(painter, this, rect());

        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        auto extent = qMin(GIT_SYNC_BUTTON_ICON_EXTENT, height());
        auto pixmap = glyph_.pixmap(
            glyphPath_,
            extent,
            palette().color(GIT_SYNC_BUTTON_ICON_ROLE),
            devicePixelRatioF());

        // Turned about its own center while a sync runs
        painter.save();
        painter.translate(
            GIT_SYNC_BUTTON_PADDING + extent / 2.0,
            height() / 2.0);
        painter.rotate(angle_);
        painter.drawPixmap(QPointF(-extent / 2.0, -extent / 2.0), pixmap);
        painter.restore();

        if (text().isEmpty()) {
            return;
        }

        auto text_x = GIT_SYNC_BUTTON_PADDING + GIT_SYNC_BUTTON_ICON_EXTENT +
                      GIT_SYNC_BUTTON_SPACING;
        painter.setPen(palette().color(QPalette::ButtonText));
        painter.drawText(
            QRect(text_x, 0, width() - text_x, height()),
            Qt::AlignLeft | Qt::AlignVCenter,
            text());
    }

private:
    using SyncAction_ = GitRepository::SyncAction;

    static constexpr auto FETCH_ICON_PATH_ = ":/lucide/RefreshCw.svg";
    static constexpr auto PULL_ICON_PATH_ = ":/lucide/ArrowDown.svg";
    static constexpr auto PUSH_ICON_PATH_ = ":/lucide/ArrowUp.svg";
    static constexpr auto PUBLISH_ICON_PATH_ = ":/lucide/CloudUpload.svg";
    static constexpr auto RUNNING_ICON_PATH_ = ":/lucide/LoaderCircle.svg";

    GitRepository* repository_;

    // What a click does, as last shown
    SyncAction_ action_ = SyncAction_::Fetch;

    Coco::Path glyphPath_ = QString::fromLatin1(FETCH_ICON_PATH_);
    Glyph::Cache glyph_{};

    // Turns the glyph while a sync runs
    QTimer* spinTimer_ = new QTimer(this);
    qreal angle_ = 0.0;

    void setup_()
    {
        setAttribute(Qt::WA_Hover, true); // hover state for the wash
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);

        // Its counts come and go, so it narrows and widens under a still
        // pointer, and it's disabled while the repository is busy
        HoverSync::follow(this);

        spinTimer_->setInterval(GIT_SYNC_SPIN_INTERVAL_MS);
        connect(spinTimer_, &QTimer::timeout, this, [this] {
            angle_ += GIT_SYNC_SPIN_DEGREES;

            if (angle_ >= 360.0) {
                angle_ -= 360.0;
            }

            update();
        });

        connect(this, &QAbstractButton::clicked, this, [this] {
            if (!repository_->isBusy()) {
                repository_->sync(action_);
            }
        });

        connect(
            repository_,
            &GitRepository::statusChanged,
            this,
            &GitSyncButton::update_);

        connect(
            repository_,
            &GitRepository::remotesChanged,
            this,
            &GitSyncButton::update_);

        connect(
            repository_,
            &GitRepository::foundChanged,
            this,
            &GitSyncButton::update_);

        connect(
            repository_,
            &GitRepository::busyChanged,
            this,
            &GitSyncButton::update_);

        connect(
            repository_,
            &GitRepository::syncFailed,
            this,
            &GitSyncButton::onSyncFailed_);

        update_();
    }

    // The action, its glyph and counts, whether it can be clicked, and the
    // turning while a sync runs
    void update_()
    {
        auto remote = repository_->syncRemote();
        const auto& branch = repository_->status().branch;
        QString counts{};

        if (repository_->syncing()) {
            glyphPath_ = QString::fromLatin1(RUNNING_ICON_PATH_);
        } else if (remote.isEmpty()) {
            glyphPath_ = QString::fromLatin1(FETCH_ICON_PATH_);
        } else if (
            !branch.upstreamFound && !branch.detached && !branch.unborn) {
            action_ = SyncAction_::Publish;
            glyphPath_ = QString::fromLatin1(PUBLISH_ICON_PATH_);
        } else if (branch.behind > 0) {
            action_ = SyncAction_::Pull;
            glyphPath_ = QString::fromLatin1(PULL_ICON_PATH_);
        } else if (branch.ahead > 0) {
            action_ = SyncAction_::Push;
            glyphPath_ = QString::fromLatin1(PUSH_ICON_PATH_);
        } else {
            action_ = SyncAction_::Fetch;
            glyphPath_ = QString::fromLatin1(FETCH_ICON_PATH_);
        }

        if (!remote.isEmpty() && branch.upstreamFound) {
            if (branch.behind > 0) {
                counts = u"↓"_s + QString::number(branch.behind);
            }

            if (branch.ahead > 0) {
                counts += (counts.isEmpty() ? u""_s : u" "_s) + u"↑"_s +
                          QString::number(branch.ahead);
            }
        }

        if (repository_->syncing()) {
            spinTimer_->start();
        } else {
            spinTimer_->stop();
            angle_ = 0.0;
        }

        setText(counts);
        setEnabled(!repository_->isBusy() && !remote.isEmpty());
        updateGeometry();
        update();
    }

    [[nodiscard]] QString toolTipText_() const
    {
        auto remote = repository_->syncRemote();

        if (auto running = repository_->syncing()) {
            switch (*running) {
            case SyncAction_::Fetch:
                return tr("Fetching %1…").arg(remote);
            case SyncAction_::Pull:
                return tr("Pulling %1…").arg(remote);
            case SyncAction_::Push:
                return tr("Pushing %1…").arg(remote);
            case SyncAction_::Publish:
                return tr("Publishing to %1…").arg(remote);
            }
        }

        if (remote.isEmpty()) {
            return tr("This repository has no remote");
        }

        QString action{};

        switch (action_) {
        case SyncAction_::Fetch:
            action = tr("Fetch %1").arg(remote);
            break;
        case SyncAction_::Pull:
            action = tr("Pull %1").arg(remote);
            break;
        case SyncAction_::Push:
            action = tr("Push %1").arg(remote);
            break;
        case SyncAction_::Publish:
            action = tr("Publish this branch to %1").arg(remote);
            break;
        }

        return action + u'\n' + lastFetchedText_();
    }

    // How long ago the repository last fetched, roughly, as GitHub Desktop
    // says it
    [[nodiscard]] QString lastFetchedText_() const
    {
        auto fetched = repository_->lastFetchedEpochMs();

        if (fetched <= 0) {
            return tr("Never fetched");
        }

        auto minutes = int((Coco::Time::epoch() - fetched) / (60 * 1000));

        if (minutes < 1) {
            return tr("Last fetched just now");
        }

        if (minutes < 60) {
            return tr("Last fetched %Ln minute(s) ago", nullptr, minutes);
        }

        if (minutes < 24 * 60) {
            return tr("Last fetched %Ln hour(s) ago", nullptr, minutes / 60);
        }

        return tr("Last fetched %Ln day(s) ago", nullptr, minutes / (24 * 60));
    }

    // Only this button asks for a sync, so every failure is its own to show.
    // Shown without exec: the window may close while it's up
    void onSyncFailed_(const QString& error)
    {
        auto* box = new QMessageBox(
            QMessageBox::Warning,
            tr("Sync failed"),
            failureText_(),
            QMessageBox::Ok,
            this);
        box->setInformativeText(error);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->open();
    }

    // Both sides have commits the other lacks, which a fast-forward pull
    // refuses. Only then does a failed pull name merging, so a pull refused
    // for another reason (uncommitted changes, a remote that can't be
    // reached) isn't blamed on it
    [[nodiscard]] bool diverged_() const
    {
        const auto& branch = repository_->status().branch;
        return branch.upstreamFound && branch.ahead > 0 && branch.behind > 0;
    }

    // The main line of a failure's message. The informative text under it
    // gives the reason, most often in git's words
    [[nodiscard]] QString failureText_() const
    {
        switch (action_) {
        case SyncAction_::Fetch:
            return tr("Nothing was fetched.");
        case SyncAction_::Pull:
            return diverged_()
                       ? tr("Nothing was pulled. This branch and its remote "
                            "both have new commits, which need merging "
                            "outside Fernanda.")
                       : tr("Nothing was pulled.");
        case SyncAction_::Push:
            return tr("Nothing was pushed.");
        case SyncAction_::Publish:
            return tr("The branch wasn't published.");
        }

        return {};
    }
};

} // namespace Fernanda::Ui
