/*
 * Fernanda — A plain-text writing app for fiction, poetry, and long projects
 * Copyright (C) 2026 fairybow
 *
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

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QRect>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "ui/UiConstants.h"
#include "ui/tabs/BreadcrumbFit.h"
#include "ui/tabs/PageIcon.h"
#include "ui/tabs/PagePath.h"
#include "ui/tabs/TabHistory.h"
#include "ui/widgets/Glyph.h"
#include "ui/widgets/GlyphButton.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

namespace Internal {

// One of the header's two arrows: a fixed glyph in a fixed tint. Disabled, it
// draws in the palette's Disabled colors, as every glyph button does
class TabHeaderButton_ : public GlyphButton
{
public:
    TabHeaderButton_(
        const Coco::Path& glyphPath,
        QPalette::ColorRole glyphRole,
        QWidget* parentHeader)
        : GlyphButton(
              TAB_HEADER_BUTTON_EXTENT,
              TAB_HEADER_BUTTON_ICON_EXTENT,
              parentHeader)
        , glyphPath_(glyphPath)
        , glyphRole_(glyphRole)
    {
    }

protected:
    [[nodiscard]] Coco::Path glyphPath() const override { return glyphPath_; }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return glyphRole_;
    }

private:
    Coco::Path glyphPath_;
    QPalette::ColorRole glyphRole_;
};

// The header's path: the page's mark, then its folders, muted, each followed
// by a separator, then its name. Where it doesn't all fit, Breadcrumbs::fit
// decides how wide each part is drawn, and each part is cut short with an
// ellipsis to that width. Its tooltip is the whole path. It asks for no width,
// so a long path never widens the pane
class TabHeaderPath_ : public QWidget
{
public:
    explicit TabHeaderPath_(QWidget* parentHeader)
        : QWidget(parentHeader)
    {
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    }

    // The folders are the path's, without its last part, the file. The name
    // is the one the page's tab shows, so a file's has no extension, as in
    // the file tree
    void setPath(
        const QString& path,
        const QString& name,
        const QString& markPath)
    {
        folders_ = path.split(u'/', Qt::SkipEmptyParts);

        if (!folders_.isEmpty()) {
            folders_.removeLast();
        }

        name_ = name;
        markPath_ = markPath;
        setToolTip(path);
        update();
    }

protected:
    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QPainter painter(this);
        auto metrics = fontMetrics();
        auto x = 0;

        if (!markPath_.isEmpty()) {
            auto pixmap = mark_.pixmap(
                markPath_,
                TAB_HEADER_MARK_EXTENT,
                palette().color(TAB_HEADER_MARK_ICON_ROLE),
                devicePixelRatioF());

            painter.drawPixmap(
                x,
                (height() - TAB_HEADER_MARK_EXTENT) / 2,
                pixmap);

            x += TAB_HEADER_MARK_EXTENT + TAB_HEADER_MARK_GAP;
        }

        QList<int> widths{};
        QList<int> minimums{};

        for (const auto& folder : folders_) {
            widths << metrics.horizontalAdvance(folder);
            minimums << metrics.horizontalAdvance(folder.left(1) + ELLIPSIS_);
        }

        auto separator = metrics.horizontalAdvance(SEPARATOR_);
        auto ellipsis = metrics.horizontalAdvance(ELLIPSIS_);

        auto fit = Breadcrumbs::fit(
            width() - x,
            metrics.horizontalAdvance(name_),
            widths,
            minimums,
            separator,
            ellipsis);

        // A part given its whole width is drawn whole. elidedText measures in
        // fractions of a pixel, and horizontalAdvance rounds, so asked to fit
        // a text into its own rounded width, it could cut it short
        auto draw = [&](const QString& text,
                        int width,
                        QPalette::ColorRole role) {
            auto shown = width >= metrics.horizontalAdvance(text)
                             ? text
                             : metrics.elidedText(text, Qt::ElideRight, width);

            painter.setPen(palette().color(role));
            painter.drawText(
                QRect(x, 0, width, height()),
                Qt::AlignLeft | Qt::AlignVCenter,
                shown);

            x += width;
        };

        for (qsizetype i = 0; i < fit.folders.size(); ++i) {
            draw(folders_[i], fit.folders[i], TAB_HEADER_FOLDER_ROLE);
            draw(SEPARATOR_, separator, TAB_HEADER_FOLDER_ROLE);
        }

        if (fit.dropped) {
            draw(ELLIPSIS_, ellipsis, TAB_HEADER_FOLDER_ROLE);
            draw(SEPARATOR_, separator, TAB_HEADER_FOLDER_ROLE);
        }

        draw(name_, fit.name, TAB_HEADER_TITLE_ROLE);
    }

private:
    static inline const QString SEPARATOR_ = u" / "_s;
    static inline const QString ELLIPSIS_ = u"\u2026"_s;

    QStringList folders_{};
    QString name_{};
    QString markPath_{};
    Glyph::Cache mark_{};
};

} // namespace Internal

// The strip over a pane's pages, as Obsidian's view header is: back and
// forward arrows for the tab's history (TabHistory), then the path of the
// page the pane shows. One per pane (TabPaneLeaf), showing whichever page is
// current, so like the leaf it learns nothing of page types: it shows the
// title on the page's tab (a new tab's "New tab" included), and what the page
// widget carries: its history, its path (PagePath), and its tab's mark
// (PageIcon), which marks a Common Vault file in a project's window and a
// diff here as on the tab.
//
// An arrow is enabled while the history has an entry that way, and a click
// asks the leaf to step (stepRequested). The arrows never take the keyboard
// focus, as the tab row's buttons don't.
//
// It sits on the page, in the page's color (Base), so the current tab, the
// header, and the page read as one surface, as the find bar does
class TabHeader : public QWidget
{
    Q_OBJECT

public:
    explicit TabHeader(QWidget* parentLeaf)
        : QWidget(parentLeaf)
    {
        setup_();
    }

    ~TabHeader() override { TRACER; }

    // Show page's history's directions and its path, ending in the title its
    // tab shows, or nothing for no page. Called again whenever any of them may
    // have changed
    void showPage(QWidget* page, const QString& title)
    {
        auto history = pageHistory(page);

        back_->setEnabled(history.canGoBack());
        forward_->setEnabled(history.canGoForward());
        path_->setPath(pagePath(page), title, pageIconPath(page));
    }

signals:
    void stepRequested(TabHistory::Step step);

private:
    Internal::TabHeaderButton_* back_ = new Internal::TabHeaderButton_(
        u":/lucide/ChevronLeft.svg"_s,
        TAB_HEADER_BACK_ICON_ROLE,
        this);
    Internal::TabHeaderButton_* forward_ = new Internal::TabHeaderButton_(
        u":/lucide/ChevronRight.svg"_s,
        TAB_HEADER_FORWARD_ICON_ROLE,
        this);
    Internal::TabHeaderPath_* path_ = new Internal::TabHeaderPath_(this);

    void setup_()
    {
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);
        setFixedHeight(TAB_HEADER_HEIGHT);

        back_->setToolTip(tr("Navigate back"));
        forward_->setToolTip(tr("Navigate forward"));

        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(TAB_HEADER_LEFT_MARGIN, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(back_);
        layout->addSpacing(TAB_HEADER_ARROW_SPACING);
        layout->addWidget(forward_);
        layout->addSpacing(TAB_HEADER_TITLE_GAP);
        layout->addWidget(path_, 1);
        layout->addSpacing(TAB_HEADER_RIGHT_MARGIN);

        connect(back_, &QAbstractButton::clicked, this, [this] {
            emit stepRequested(TabHistory::Step::Back);
        });

        connect(forward_, &QAbstractButton::clicked, this, [this] {
            emit stepRequested(TabHistory::Step::Forward);
        });
    }
};

} // namespace Fernanda::Ui
