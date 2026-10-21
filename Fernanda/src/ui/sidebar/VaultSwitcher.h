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

#include <functional>
#include <utility>

#include <QAction>
#include <QFontMetrics>
#include <QIcon>
#include <QList>
#include <QMenu>
#include <QPaintEvent>
#include <QPalette>
#include <QRect>
#include <QSizePolicy>
#include <QString>
#include <QStyle>
#include <QStyleOptionToolButton>
#include <QStylePainter>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QtMinMax>

#include <Coco/Debug.h>
#include <Coco/Path.h>

#include "core/PathKey.h"
#include "core/VaultEntry.h"
#include "ui/UiConstants.h"
#include "ui/widgets/Glyph.h"

namespace Fernanda::Ui {

namespace Internal {

// The switcher's button: a QToolButton that paints Lucide ChevronsUpDown, then
// the vault name left-aligned after it (Obsidian's Vault profile layout), in
// place of the style's label and menu indicator.
//
// The style draws only the frame. paintEvent builds the option exactly as
// QToolButton's own does, clears HasMenu (the style draws its bottom-right
// indicator whenever the option carries it, and nothing short of QSS turns it
// off), and empties the label, so CC_ToolButton paints the panel's hover,
// press, and sunken-while-open states and nothing else. The glyph and name
// are painted here, from the VAULT_SWITCHER_* knobs in UiConstants.h. The
// style's label couldn't be kept: Qt 6.11's Windows 11 style centers the text
// beside an icon where 6.10 left-aligned it. Owning the layout also elides a
// long name on the right, and shows a name's '&' as itself rather than as a
// mnemonic.
//
// Everything else stays QToolButton's: setMenu / InstantPopup still position
// the popup (flipping it upward at the screen's bottom edge) and keep the
// button sunken while the menu is open. Only the painted option changes; the
// widget's own state never does. The glyph goes through a Glyph::Cache keyed on
// its tint, so it follows the current color group with no refresh bookkeeping
// and re-renders only when the resolved color (or the DPR) actually moves
class VaultSwitcherButton_ : public QToolButton
{
    Q_OBJECT

public:
    explicit VaultSwitcherButton_(QWidget* parentVaultSwitcher)
        : QToolButton(parentVaultSwitcher)
    {
    }

    ~VaultSwitcherButton_() override { TRACER; }

protected:
    void paintEvent([[maybe_unused]] QPaintEvent* event) override
    {
        QStylePainter painter(this);

        // Frame only: no indicator, no label
        QStyleOptionToolButton option;
        initStyleOption(&option);
        option.features &= ~QStyleOptionToolButton::HasMenu;
        option.text.clear();
        option.icon = QIcon();
        option.toolButtonStyle = Qt::ToolButtonTextOnly;
        painter.drawComplexControl(QStyle::CC_ToolButton, option);

        // Sink the content with the panel on press. The metrics are 0 on
        // styles that don't shift, so this is a no-op there
        auto content = rect();
        if (option.state & QStyle::State_Sunken) {
            content.translate(
                style()->pixelMetric(
                    QStyle::PM_ButtonShiftHorizontal,
                    &option,
                    this),
                style()->pixelMetric(
                    QStyle::PM_ButtonShiftVertical,
                    &option,
                    this));
        }

        auto glyph_x = content.x() + VAULT_SWITCHER_LEFT_PADDING;

        // Clamped to the button, so an over-large extent fills it rather than
        // overflowing
        auto extent = qMin(VAULT_SWITCHER_ICON_EXTENT, content.height());
        if (extent > 0) {
            auto glyph = glyph_.pixmap(
                QString::fromLatin1(ICON_PATH_),
                extent,
                palette().color(VAULT_SWITCHER_ICON_ROLE),
                devicePixelRatioF());
            painter.drawPixmap(
                glyph_x,
                content.y() + (content.height() - extent) / 2,
                glyph);
        }

        // Measured from the unclamped slot, so the name never shifts with a
        // clamp
        auto text_x =
            glyph_x + VAULT_SWITCHER_ICON_EXTENT + VAULT_SWITCHER_ICON_SPACING;
        auto text_rect = QRect(
            text_x,
            content.y(),
            content.right() + 1 - RIGHT_PADDING_ - text_x,
            content.height());
        if (text_rect.width() <= 0) {
            return;
        }

        painter.setPen(palette().color(QPalette::ButtonText));
        painter.drawText(
            text_rect,
            Qt::AlignLeft | Qt::AlignVCenter,
            fontMetrics()
                .elidedText(text(), Qt::ElideRight, text_rect.width()));
    }

private:
    static constexpr auto ICON_PATH_ = ":/lucide/ChevronsUpDown.svg";

    // Room kept right of the name when eliding
    static constexpr auto RIGHT_PADDING_ = 8;

    Glyph::Cache glyph_{};
};

} // namespace Internal

// Obsidian-style vault switcher: shows the current vault's name, and pops a
// menu of the OTHER openable vaults plus a permanent "Manage Vaults" item, and
// "Open Common Vault" while there is one to open. The vault list is read live
// from a provider App threads down and rebuilt on every aboutToShow, so it
// always reflects the current recents without this widget observing a signal.
// Two things are handed in rather than owned:
//
//   - manageVaultsAction — a QAction owned and wired upstream (VaultWindow,
//     App); the switcher only displays it and forwards no signal of its own
//     for it.
//   - vaultsProvider — App's vaultEntries(), the same source the ManageVaults
//     recents column reads.
//
// The two roots are this window's vault's and the Common Vault's, which is
// empty when there's none.
//
// Choosing a vault emits vaultActivated with its root; the Sidebar relays it up
// to App's one convergence point, so switching and opening share a path
class VaultSwitcher : public QWidget
{
    Q_OBJECT

public:
    VaultSwitcher(
        const Coco::Path& currentVaultRoot,
        const Coco::Path& commonVaultRoot,
        QAction* manageVaultsAction,
        std::function<QList<VaultEntry>()> vaultsProvider,
        QWidget* parentSidebar)
        : QWidget(parentSidebar)
        , currentVaultRoot_(currentVaultRoot)
        , commonVaultRoot_(commonVaultRoot)
        , manageVaultsAction_(manageVaultsAction)
        , vaultsProvider_(std::move(vaultsProvider))
    {
        setup_();
    }

    ~VaultSwitcher() override { TRACER; }

    // Either root spelled anew (Vault::respellRoot), or the Common Vault's
    // changed. The button shows the new name at once; the menu is rebuilt on
    // each show anyway
    void setVaultRoots(
        const Coco::Path& currentVaultRoot,
        const Coco::Path& commonVaultRoot)
    {
        currentVaultRoot_ = currentVaultRoot;
        commonVaultRoot_ = commonVaultRoot;
        showName_();
    }

signals:
    // The user picked another known vault. Carries its root — the Sidebar
    // relays this to App::onOpenOrMakeVaultRequested_
    void vaultActivated(const Coco::Path& vaultRoot);

private:
    Coco::Path currentVaultRoot_;
    Coco::Path commonVaultRoot_;
    QAction* manageVaultsAction_ = nullptr;
    std::function<QList<VaultEntry>()> vaultsProvider_;

    Internal::VaultSwitcherButton_* button_ =
        new Internal::VaultSwitcherButton_(this);
    QMenu* menu_ = new QMenu(this);

    void setup_()
    {
        // Rebuilt on demand so the list never goes stale — no observing of
        // vaultsChanged needed. aboutToShow fires before each popup
        connect(menu_, &QMenu::aboutToShow, this, &VaultSwitcher::rebuildMenu_);

        // Openable entries carry their path as a tooltip; QMenu hides action
        // tooltips unless asked
        menu_->setToolTipsVisible(true);

        showName_();
        button_->setMenu(menu_);
        button_->setPopupMode(QToolButton::InstantPopup);
        // The button paints its own glyph and name (VaultSwitcherButton_), so
        // no tool-button style or icon size is declared; only the height is
        // ours
        button_->setFixedHeight(VAULT_SWITCHER_HEIGHT);
        button_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(button_);
    }

    void showName_()
    {
        auto name = currentVaultRoot_.nameQString();
        button_->setText(name.isEmpty() ? tr("Vault") : name);
    }

    // Wipe and repopulate from the live provider. menu_->clear() deletes the
    // per-vault actions (menu-owned) but not manageVaultsAction_, which is
    // parented to the VaultWindow — so it's safe to re-add each time
    void rebuildMenu_()
    {
        menu_->clear();

        auto any_other = false;

        for (const auto& entry : vaultsProvider_()) {
            // Switcher = OTHER, openable vaults. Skip this window's own vault,
            // the common vault (it has its own permanent item below, so it
            // never rides the recents list here), and missing/moved ones — you
            // can't switch to a folder that isn't there. Those surface (with a
            // warning and a remove action) in ManageVaults, which is what the
            // permanent "Manage Vaults" item below is for.
            //
            // Compared as keys, not as written: a record can spell a folder
            // otherwise than the live Vault does. The Common Vault's root is
            // spelled from its fixed place at each launch, though its record
            // may have been respelled in an earlier one
            if (entry.isCommon || entry.missing ||
                PathKey::of(entry.root) == PathKey::of(currentVaultRoot_)) {
                continue;
            }

            any_other = true;

            // Capture the root by value — the entry list is a temporary
            auto root = entry.root;
            auto action = menu_->addAction(entry.name);
            action->setToolTip(entry.root.prettyQString());

            connect(action, &QAction::triggered, this, [this, root] {
                emit vaultActivated(root);
            });
        }

        if (!any_other) {
            auto placeholder = menu_->addAction(tr("No other vaults"));
            placeholder->setEnabled(false);
        }

        menu_->addSeparator();
        menu_->addAction(manageVaultsAction_);

        // The common vault is always openable as its own window, independent of
        // recents — so it gets a permanent item rather than riding the provider
        // list (from which it's filtered above). Same convergence as any
        // switch: emit its root and App opens it or raises the existing window.
        // Omitted when there is no Common Vault, or this window already IS it
        // — there'd be nothing to open
        auto is_common_window =
            PathKey::of(currentVaultRoot_) == PathKey::of(commonVaultRoot_);

        if (!commonVaultRoot_.isEmpty() && !is_common_window) {
            auto open_common = menu_->addAction(tr("Open Common Vault"));
            connect(open_common, &QAction::triggered, this, [this] {
                emit vaultActivated(commonVaultRoot_);
            });
        }
    }
};

} // namespace Fernanda::Ui
