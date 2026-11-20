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

#include <algorithm>

#include <QApplication>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QPalette>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QVBoxLayout>
#include <QtTypes>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/files/FileTypes.h"
#include "core/files/PathKey.h"
#include "core/vault/Vault.h"
#include "ui/OpenMode.h"
#include "ui/UiConstants.h"
#include "ui/widgets/HoverSync.h"
#include "ui/widgets/RowBadges.h"
#include "ui/widgets/UserTextEntry.h"

namespace Fernanda::Ui {

using namespace Qt::StringLiterals;

namespace Internal {

// Paints a Go to File row with its labels ("RECENT", "COMMON") at the right
// end, each with a border so two side by side read as two. The labels ride on
// the item as a QStringList under BADGES_ROLE, in the order they're drawn left
// to right; a row with none is the style's alone
class FileSwitcherDelegate_ : public QStyledItemDelegate
{
public:
    static constexpr auto BADGES_ROLE = Qt::UserRole;

    explicit FileSwitcherDelegate_(QObject* parentList)
        : QStyledItemDelegate(parentList)
    {
    }

    void paint(
        QPainter* painter,
        const QStyleOptionViewItem& option,
        const QModelIndex& index) const override
    {
        auto badges = index.data(BADGES_ROLE).toStringList();

        if (badges.isEmpty()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);

        auto style = opt.widget ? opt.widget->style() : QApplication::style();
        auto badge_font = RowBadges::font(opt.font);
        auto badge_rects = RowBadges::rects(opt.rect, badges, badge_font);

        opt.text =
            RowBadges::nameElidedBefore(opt, style, badge_rects[0].left());
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

        for (qsizetype i = 0; i < badges.size(); ++i) {
            RowBadges::paint(
                painter,
                opt,
                badges[i],
                badge_font,
                badge_rects[i],
                true);
        }
    }
};

} // namespace Internal

// Go to File: an Obsidian-style quick switcher, a filter field over a flat list
// of every file the window's two vaults show. The list is Vault::visibleFiles,
// so it matches the sidebar trees, and every row carries its file's key, so
// nothing outside the vaults can be picked. A file both vaults list, where
// they nest, is listed once, as the window's own. The list is a snapshot; a
// file deleted since is refused by BufferStore::openModel.
//
// An empty query lists the window's recent files (RecentFiles), newest first,
// then the rest. If the newest is the active tab's file, the second row starts
// selected, so Enter goes back to the file before.
//
// Matching is case-insensitive, and every space-separated term must appear in
// the row's path. Rows whose FILE NAME holds every term rank first; then
// project files before Common Vault files, then by path. No fuzzy scoring,
// unlike Obsidian (docs/Future.md, "Go to File").
//
// The typed text, trimmed, also names a file in the window's own vault, from
// its root: "/" separates folders, each part must pass UserTextEntry, and a
// name without a text extension gets ".txt". Enter with no match, or
// Shift+Enter always, picks it; if the file exists, the pick is its row. A
// line under the field says what would be created, or why it can't be. The
// switcher never touches the disk: Pick::isNew tells the caller to create it
// (Vault::createFileAt).
//
// Rows are marked RECENT and COMMON at the right
// (Internal::FileSwitcherDelegate_). Focus stays in the field, which forwards
// Up/Down/PageUp/PageDown to the list. Enter or a click opens as ReplaceActive,
// with Ctrl (Cmd on macOS) as NewTab, a create included; Esc cancels
class FileSwitcher : public QDialog
{
    Q_OBJECT

public:
    struct Pick
    {
        PathKey key{};
        OpenMode mode{};

        // The file isn't on disk yet. The caller creates it before opening
        bool isNew = false;
    };

    // parentWindow is the window the switcher opens over — the one hosting the
    // request, which may be a pop-out. commonVault may be null, and is skipped
    // if it's the same vault. recentFiles are the window's, newest first, and
    // current the file in its active tab (none for a new tab)
    FileSwitcher(
        QWidget* parentWindow,
        Vault* vault,
        Vault* commonVault,
        const QList<PathKey>& recentFiles,
        const PathKey& current)
        : QDialog(parentWindow)
        , vault_(vault)
    {
        setup_(vault, commonVault, recentFiles, current);
    }

    ~FileSwitcher() override { TRACER; }

    // Only meaningful after exec() returns Accepted
    [[nodiscard]] Pick pick() const { return pick_; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched != queryEdit_ || event->type() != QEvent::KeyPress) {
            return QDialog::eventFilter(watched, event);
        }

        auto key_event = static_cast<QKeyEvent*>(event);

        switch (key_event->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            // The list moves its current row on these whether or not it has
            // focus — it only needs the event
            QCoreApplication::sendEvent(list_, event);
            return true;

        case Qt::Key_Return:
        case Qt::Key_Enter: {
            auto modifiers = key_event->modifiers();
            auto row = list_->currentRow();

            if ((modifiers & Qt::ShiftModifier) || row < 0) {
                acceptTarget_(modeFor_(modifiers));
            } else {
                acceptRow_(row, modeFor_(modifiers));
            }

            return true;
        }

        default:
            return QDialog::eventFilter(watched, event);
        }
    }

private:
    // One listable file. path and name are precomputed display/match strings,
    // so a keystroke's refilter does no path work
    struct Entry_
    {
        PathKey key{};
        QString path{}; // relative to its vault's root, forward slashes
        QString name{};
        bool isCommon = false;
    };

    Vault* vault_;

    QList<Entry_> entries_{};  // presorted: project first, then by path
    QList<qsizetype> shown_{}; // entries_ indices, in list-row order

    // The recent files that are listed, as entries_ indices, newest first;
    // the same as a set, for the RECENT label; and the row an empty query
    // starts on (see class note)
    QList<qsizetype> recent_{};
    QSet<qsizetype> recentSet_{};
    int emptyQueryRow_ = 0;

    // The file the typed text names (see class note): empty when it names
    // none; and, when that file is already there, its entries_ index
    PathKey target_{};
    qsizetype targetEntry_ = -1;

    QLineEdit* queryEdit_ = nullptr;
    QLabel* hint_ = nullptr;
    QListWidget* list_ = nullptr;
    Pick pick_{};

    void setup_(
        Vault* vault,
        Vault* commonVault,
        const QList<PathKey>& recentFiles,
        const PathKey& current)
    {
        setWindowTitle(tr("Go to File"));
        setWindowModality(Qt::ApplicationModal);

        collect_(vault, false);
        if (commonVault && commonVault != vault) {
            collect_(commonVault, true);
        }

        presort_();
        findRecent_(recentFiles, current);
        setupUi_();
        placeOverParent_();
        refilter_();
    }

    // A Common Vault file inside the window's own vault is already listed as
    // the window's own, so it's skipped (see class note)
    void collect_(Vault* vault, bool isCommon)
    {
        auto root_key = vault->rootKey();
        auto own_root_key = vault_->rootKey();

        for (const auto& key : vault->visibleFiles()) {
            if (isCommon && key.isUnder(own_root_key)) {
                continue;
            }

            entries_ << Entry_{
                .key = key,
                .path = key.relativeTo(root_key).prettyQString(),
                .name = key.path().nameQString(),
                .isCommon = isCommon,
            };
        }
    }

    // Once, at construction. refilter_ then only partitions by tier, and a
    // stable partition keeps this order inside each tier
    void presort_()
    {
        std::sort(
            entries_.begin(),
            entries_.end(),
            [](const Entry_& a, const Entry_& b) {
                if (a.isCommon != b.isCommon) {
                    return !a.isCommon;
                }

                return QString::compare(a.path, b.path, Qt::CaseInsensitive) <
                       0;
            });
    }

    // Each recent file that is in the listing, in the recent order. One that
    // isn't (deleted or moved since) is skipped
    void findRecent_(const QList<PathKey>& recentFiles, const PathKey& current)
    {
        for (const auto& key : recentFiles) {
            for (qsizetype i = 0; i < entries_.size(); ++i) {
                if (entries_[i].key == key) {
                    recent_ << i;
                    recentSet_ << i;
                    break;
                }
            }
        }

        if (recent_.size() > 1 && entries_[recent_.first()].key == current) {
            emptyQueryRow_ = 1;
        }
    }

    void setupUi_()
    {
        queryEdit_ = new QLineEdit(this);
        queryEdit_->setPlaceholderText(tr("Type to find a file..."));
        queryEdit_->installEventFilter(this);

        connect(
            queryEdit_,
            &QLineEdit::textChanged,
            this,
            &FileSwitcher::refilter_);

        hint_ = new QLabel(this);
        hint_->setTextFormat(Qt::PlainText);
        hint_->setForegroundRole(QPalette::PlaceholderText);
        hint_->hide();

        list_ = new QListWidget(this);
        list_->setFocusPolicy(Qt::NoFocus); // typing always lands in the field
        list_->setUniformItemSizes(true);   // cheap layout for long listings
        list_->setItemDelegate(new Internal::FileSwitcherDelegate_(list_));

        // The hovered row follows the pointer while the scroll bar is dragged
        HoverSync::followScrollBars(list_);

        connect(
            list_,
            &QListWidget::itemClicked,
            this,
            [this](QListWidgetItem* item) {
                acceptRow_(
                    list_->row(item),
                    modeFor_(QGuiApplication::keyboardModifiers()));
            });

        auto root_layout = new QVBoxLayout(this);
        root_layout->setContentsMargins(
            FILE_SWITCHER_MARGIN,
            FILE_SWITCHER_MARGIN,
            FILE_SWITCHER_MARGIN,
            FILE_SWITCHER_MARGIN);
        root_layout->setSpacing(FILE_SWITCHER_SPACING);
        root_layout->addWidget(queryEdit_);
        root_layout->addWidget(hint_);
        root_layout->addWidget(list_);

        queryEdit_->setFocus();
    }

    // Top-center over the host window, Obsidian-style, instead of QDialog's
    // default centering. Moving before show sets WA_Moved, which is what stops
    // QDialog::setVisible from re-centering it
    void placeOverParent_()
    {
        auto* host = parentWidget();
        if (!host) {
            return;
        }

        auto geometry = host->window()->geometry();
        auto width = qMin(FILE_SWITCHER_WIDTH, geometry.width());

        resize(width, FILE_SWITCHER_HEIGHT);
        move(
            geometry.center().x() - width / 2,
            geometry.top() + FILE_SWITCHER_TOP_OFFSET);
    }

    void refilter_()
    {
        auto terms = queryEdit_->text().split(u' ', Qt::SkipEmptyParts);

        shown_.clear();

        if (terms.isEmpty()) {
            shown_ = recent_;

            for (qsizetype i = 0; i < entries_.size(); ++i) {
                if (!recentSet_.contains(i)) {
                    shown_ << i;
                }
            }

            rebuildList_(emptyQueryRow_);
            findTarget_();
            return;
        }

        for (qsizetype i = 0; i < entries_.size(); ++i) {
            if (containsAll_(entries_[i].path, terms)) {
                shown_ << i;
            }
        }

        // Name matches ahead of path-only matches. Stable, so the presort
        // (project before common, then path) holds inside each tier. With no
        // terms every entry is a name match and the order is the presort
        std::stable_partition(shown_.begin(), shown_.end(), [&](qsizetype i) {
            return containsAll_(entries_[i].name, terms);
        });

        rebuildList_(0);
        findTarget_();
    }

    // Reads the typed text as a file to create (see class note), and says on
    // the hint line what Enter or Shift+Enter would do with it. A part left
    // empty ("drafts/" while typing on) is a neutral state, as an empty name
    // is to UserTextEntry, and shows nothing
    void findTarget_()
    {
        target_ = {};
        targetEntry_ = -1;
        hint_->hide();

        auto text = queryEdit_->text().trimmed();
        if (text.isEmpty()) {
            return;
        }

        auto parts = text.split(u'/');

        for (const auto& part : parts) {
            auto problem = UserTextEntry::problemOf(part);

            if (problem == UserTextEntry::Problem::Empty) {
                return;
            }

            if (problem != UserTextEntry::Problem::None) {
                showHint_(UserTextEntry::describeInPath(problem));
                return;
            }
        }

        if (FileTypes::typeOf(Coco::Path(parts.last())) !=
            FileTypes::Type::Text) {
            parts.last() += u".txt"_s;
        }

        auto relative = Coco::Path(parts.join(u'/'));
        auto path = relative.prettyQString();
        auto key = PathKey::of(vault_->root() / relative);

        // On disk, by the disk's own rule for letter case. The row, if any,
        // gives the file's own spelling, so the pick takes the row's key
        if (key.path().exists()) {
            targetEntry_ = findEntry_(key);

            if (targetEntry_ < 0) {
                showHint_(tr("\"%1\" already exists.").arg(path));
            }

            return;
        }

        target_ = key;
        showHint_(
            shown_.isEmpty() ? tr("Enter to create \"%1\"").arg(path)
                             : tr("Shift+Enter to create \"%1\"").arg(path));
    }

    void showHint_(const QString& text)
    {
        hint_->setText(text);
        hint_->show();
    }

    // The window's own vault's row for a key, or -1. Keys compare letter
    // case as the platform's file systems do, so on Windows and macOS a row
    // differing only in case is found
    [[nodiscard]] qsizetype findEntry_(const PathKey& key) const
    {
        for (qsizetype i = 0; i < entries_.size(); ++i) {
            const auto& entry = entries_[i];

            if (!entry.isCommon && entry.key == key) {
                return i;
            }
        }

        return -1;
    }

    // currentRow is the row to select, when there is one
    void rebuildList_(int currentRow)
    {
        list_->setUpdatesEnabled(false);
        list_->clear();

        for (auto i : shown_) {
            const auto& entry = entries_[i];

            auto* item = new QListWidgetItem(entry.path, list_);

            QStringList badges{};

            if (entry.isCommon) {
                badges << tr("COMMON");
                item->setToolTip(tr("Common Vault"));
            }

            if (recentSet_.contains(i)) {
                badges << tr("RECENT");
            }

            item->setData(Internal::FileSwitcherDelegate_::BADGES_ROLE, badges);
        }

        list_->setCurrentRow(shown_.isEmpty() ? -1 : currentRow);
        list_->setUpdatesEnabled(true);
    }

    // No row does nothing and keeps the switcher open
    void acceptRow_(int row, OpenMode mode)
    {
        if (row < 0 || row >= shown_.size()) {
            return;
        }

        pick_ = { entries_[shown_[row]].key, mode };
        accept();
    }

    // The typed text's file: its row if it's there, or else a new file. Text
    // that names no file does nothing and keeps the switcher open
    void acceptTarget_(OpenMode mode)
    {
        if (targetEntry_ >= 0) {
            pick_ = { entries_[targetEntry_].key, mode };
        } else if (!target_.isEmpty()) {
            pick_ = { target_, mode, true };
        } else {
            return;
        }

        accept();
    }

    [[nodiscard]] static bool
    containsAll_(const QString& haystack, const QStringList& terms)
    {
        return std::all_of(
            terms.begin(),
            terms.end(),
            [&](const QString& term) {
                return haystack.contains(term, Qt::CaseInsensitive);
            });
    }

    [[nodiscard]] static OpenMode modeFor_(Qt::KeyboardModifiers modifiers)
    {
        return (modifiers & Qt::ControlModifier) ? OpenMode::NewTab
                                                 : OpenMode::ReplaceActive;
    }
};

} // namespace Fernanda::Ui
