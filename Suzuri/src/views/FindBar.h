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

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QObject>
#include <QPalette>
#include <QString>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "ui/widgets/GlyphButton.h"
#include "views/TextSearch.h"
#include "views/ViewConstants.h"

namespace Fernanda {

using namespace Qt::StringLiterals;

namespace Internal {

// One of the find bar's glyph buttons: the toggle for the row to replace
// with, previous, next, or close. A GlyphButton whose glyph its owner sets;
// only the toggle's ever changes. It takes no focus, so a click leaves the
// caret in the field it was in
class FindBarButton_ : public Ui::GlyphButton
{
    Q_OBJECT

public:
    FindBarButton_(const Coco::Path& glyphPath, QWidget* parentFindBar)
        : GlyphButton(
              FIND_BAR_BUTTON_EXTENT,
              FIND_BAR_ICON_EXTENT,
              parentFindBar)
        , glyphPath_(glyphPath)
    {
        setFocusPolicy(Qt::NoFocus);
    }

    void setGlyphPath(const Coco::Path& glyphPath)
    {
        glyphPath_ = glyphPath;
        update();
    }

protected:
    [[nodiscard]] Coco::Path glyphPath() const override { return glyphPath_; }

    [[nodiscard]] QPalette::ColorRole glyphRole() const override
    {
        return FIND_BAR_ICON_ROLE;
    }

private:
    Coco::Path glyphPath_;
};

} // namespace Internal

// The controls a text view shows across its top while searching. One row to
// find: a toggle for the second row, the search field, previous and next, the
// two options, how many matches there are and which one the search is on, and
// close. Under it, when asked for or toggled open, a row to replace: the
// replacement field, Replace, and Replace all.
//
// It holds what the user typed and chose, and says when that changes or when a
// button is pressed. It searches and replaces nothing and knows nothing of the
// document: TextFileView owns the search and connects to these signals
// directly.
//
// In the search field, Enter is next and Shift+Enter previous. In the
// replacement field, Enter is Replace. Esc closes from either
class FindBar : public QWidget
{
    Q_OBJECT

public:
    explicit FindBar(QWidget* parentTextFileView)
        : QWidget(parentTextFileView)
    {
        setup_();
    }

    ~FindBar() override { TRACER; }

    [[nodiscard]] QString term() const { return term_->text(); }

    [[nodiscard]] TextSearch::Options options() const
    {
        return { matchCase_->isChecked(), wholeWord_->isChecked() };
    }

    // Announces searchChanged if the text differs from what is there
    void setTerm(const QString& term) { term_->setText(term); }

    [[nodiscard]] QString replacement() const { return replacement_->text(); }

    // Show or hide the row to replace with. The toggle points down while the
    // row is open and right while it is closed, and its tooltip says what a
    // click will do
    void setReplaceShown(bool shown)
    {
        replaceRow_->setVisible(shown);

        // Showing a widget redoes the layouts above it at once. Hiding one
        // leaves them for later passes of the event loop, one layout per
        // pass, and the bar is painted at its old height in between. Redo
        // this layout and the hosting view's now. (There is no layout yet
        // when setup_ first hides the row)
        if (!shown && layout()) {
            layout()->activate();

            if (auto* host = parentWidget(); host && host->layout()) {
                host->layout()->activate();
            }
        }

        toggleReplace_->setGlyphPath(
            shown ? u":/lucide/ChevronDown.svg"_s
                  : u":/lucide/ChevronRight.svg"_s);
        toggleReplace_->setToolTip(
            shown ? tr("Hide replace") : tr("Show replace"));
    }

    // Put the caret in the search field with its text selected, so typing
    // replaces it
    void focusTerm()
    {
        term_->setFocus();
        term_->selectAll();
    }

    // Show where the search stands: "3 of 17" when it is on a match (current
    // counts from 0), "17 matches" when it is on none (current is -1), "No
    // matches" for a term that matched nothing, and nothing for an empty term
    void setCount(int current, int total)
    {
        QLocale locale{};

        if (term_->text().isEmpty()) {
            count_->clear();
        } else if (total == 0) {
            count_->setText(tr("No matches"));
        } else if (current < 0) {
            count_->setText(tr("%Ln match(es)", nullptr, total));
        } else {
            count_->setText(
                tr("%1 of %2")
                    .arg(locale.toString(current + 1), locale.toString(total)));
        }
    }

    // Show a message where the count goes, until the count is next set
    void setMessage(const QString& message) { count_->setText(message); }

signals:
    // The term or an option changed
    void searchChanged();

    void nextRequested();
    void previousRequested();
    void replaceRequested();
    void replaceAllRequested();
    void closeRequested();

protected:
    // The fields' keys. Taken here because QLineEdit reports Enter with no
    // word of Shift, and does nothing with Esc
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        auto is_field = watched == term_ || watched == replacement_;

        if (is_field && event->type() == QEvent::KeyPress) {
            auto* key_event = static_cast<QKeyEvent*>(event);

            switch (key_event->key()) {
            case Qt::Key_Return:
            case Qt::Key_Enter:
                if (watched == replacement_) {
                    emit replaceRequested();
                } else if (key_event->modifiers() & Qt::ShiftModifier) {
                    emit previousRequested();
                } else {
                    emit nextRequested();
                }
                return true;

            case Qt::Key_Escape:
                emit closeRequested();
                return true;

            default:
                break;
            }
        }

        return QWidget::eventFilter(watched, event);
    }

private:
    Internal::FindBarButton_* toggleReplace_ =
        new Internal::FindBarButton_(u":/lucide/ChevronRight.svg"_s, this);
    QLineEdit* term_ = new QLineEdit(this);
    QLabel* count_ = new QLabel(this);
    Internal::FindBarButton_* previous_ =
        new Internal::FindBarButton_(u":/lucide/ChevronUp.svg"_s, this);
    Internal::FindBarButton_* next_ =
        new Internal::FindBarButton_(u":/lucide/ChevronDown.svg"_s, this);
    QToolButton* matchCase_ = new QToolButton(this);
    QToolButton* wholeWord_ = new QToolButton(this);
    Internal::FindBarButton_* close_ =
        new Internal::FindBarButton_(u":/lucide/X.svg"_s, this);

    QWidget* replaceRow_ = new QWidget(this);
    QLineEdit* replacement_ = new QLineEdit(replaceRow_);
    QToolButton* replace_ = new QToolButton(replaceRow_);
    QToolButton* replaceAll_ = new QToolButton(replaceRow_);

    void setup_()
    {
        // The bar sits on the page, in the page's own color, so the current
        // tab, the bar, and the text read as one surface
        setBackgroundRole(QPalette::Base);
        setAutoFillBackground(true);

        setupField_(term_, tr("Find"));
        setupField_(replacement_, tr("Replace with"));

        count_->setForegroundRole(FIND_BAR_COUNT_ROLE);

        previous_->setToolTip(tr("Previous match"));
        next_->setToolTip(tr("Next match"));
        close_->setToolTip(tr("Close"));

        setupOption_(matchCase_, tr("Match case"));
        setupOption_(wholeWord_, tr("Whole word"));

        setupButton_(replace_, tr("Replace"));
        setupButton_(replaceAll_, tr("Replace all"));

        // In each row the field alone has a stretch factor, so it takes spare
        // width up to its maximum before the gap after the controls takes any,
        // and the two fields come out the same width
        auto* find_row = new QHBoxLayout{};
        find_row->setSpacing(FIND_BAR_SPACING);
        find_row->addWidget(toggleReplace_);
        find_row->addWidget(term_, 1);
        find_row->addWidget(previous_);
        find_row->addWidget(next_);
        find_row->addWidget(matchCase_);
        find_row->addWidget(wholeWord_);
        find_row->addWidget(count_);
        find_row->addStretch(0);
        find_row->addWidget(close_);

        auto* replace_row = new QHBoxLayout(replaceRow_);
        replace_row->setContentsMargins(0, 0, 0, 0);
        replace_row->setSpacing(FIND_BAR_SPACING);

        // The width of the toggle and the gap after it, so the replacement
        // field sits under the search field. A layout puts no gap of its own
        // beside a spacer
        replace_row->addSpacing(FIND_BAR_BUTTON_EXTENT + FIND_BAR_SPACING);
        replace_row->addWidget(replacement_, 1);
        replace_row->addWidget(replace_);
        replace_row->addWidget(replaceAll_);
        replace_row->addStretch(0);

        setReplaceShown(false);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(
            FIND_BAR_MARGIN,
            FIND_BAR_MARGIN,
            FIND_BAR_MARGIN,
            FIND_BAR_MARGIN);
        layout->setSpacing(FIND_BAR_SPACING);
        layout->addLayout(find_row);
        layout->addWidget(replaceRow_);

        connect(toggleReplace_, &QAbstractButton::clicked, this, [this] {
            setReplaceShown(replaceRow_->isHidden());
        });

        connect(term_, &QLineEdit::textChanged, this, [this] {
            emit searchChanged();
        });

        connect(previous_, &QAbstractButton::clicked, this, [this] {
            emit previousRequested();
        });

        connect(next_, &QAbstractButton::clicked, this, [this] {
            emit nextRequested();
        });

        connect(close_, &QAbstractButton::clicked, this, [this] {
            emit closeRequested();
        });

        connect(replace_, &QAbstractButton::clicked, this, [this] {
            emit replaceRequested();
        });

        connect(replaceAll_, &QAbstractButton::clicked, this, [this] {
            emit replaceAllRequested();
        });
    }

    void setupField_(QLineEdit* field, const QString& placeholder)
    {
        field->setPlaceholderText(placeholder);
        field->setMinimumWidth(FIND_BAR_TERM_MIN_WIDTH);
        field->setMaximumWidth(FIND_BAR_TERM_WIDTH);
        field->setClearButtonEnabled(true);
        field->installEventFilter(this);
    }

    // A text button on the bar. It takes no focus, so a click leaves the
    // caret in the field it was in
    void setupButton_(QToolButton* button, const QString& text)
    {
        button->setText(text);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
    }

    // A text button that stays down while its option is on
    void setupOption_(QToolButton* button, const QString& text)
    {
        setupButton_(button, text);
        button->setCheckable(true);

        connect(button, &QToolButton::toggled, this, [this] {
            emit searchChanged();
        });
    }
};

} // namespace Fernanda
