/*
 * Fernanda  Copyright (C) 2025  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

/*#pragma once

#include <functional>

#include <QDialog>
#include <QFont>
#include <QObject>
#include <QVBoxLayout>
#include <QWidget>

#include "Debug.h"
#include "FontSelector.h"
#include "Timers.h"

namespace Fernanda {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    using FontChangeHandler = std::function<void(const QFont&)>;

    // Initials received via ctor
    explicit SettingsDialog(const QFont& initialFont, QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setup_(initialFont);
    }

    virtual ~SettingsDialog() override { TRACER; }

    FontChangeHandler fontChangeHandler() const noexcept
    {
        return fontChangeHandler_;
    }

    void setFontChangeHandler(const FontChangeHandler& handler)
    {
        fontChangeHandler_ = handler;
    }

    template <typename ClassT>
    void
    setFontChangeHandler(ClassT* object, void (ClassT::*method)(const QFont&))
    {
        fontChangeHandler_ = [object, method](const QFont& font) {
            (object->*method)(font);
        };
    }

signals:
    void fontSaveRequested(const QFont& font);

private:
    FontSelector* fontSelector_ = nullptr;
    FontChangeHandler fontChangeHandler_ = nullptr;
    QFont pendingFont_{};
    bool hasPendingFont_ = false;
    Timers::Debouncer* fontDebouncer_ =
        new Timers::Debouncer(500, this, &SettingsDialog::onFontDebounce_);

    void setup_(const QFont& initialFont)
    {
        fontSelector_ = new FontSelector(initialFont, this);
        setModal(false);

        auto layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(fontSelector_);

        connect(
            fontSelector_,
            &FontSelector::currentChanged,
            this,
            &SettingsDialog::onFontSelectorCurrentChanged_);
    }

private slots:
    void onFontSelectorCurrentChanged_(const QFont& font)
    {
        // Immediate callback for visual feedback
        if (fontChangeHandler_) fontChangeHandler_(font);

        // Debounce the persistence
        pendingFont_ = font;
        hasPendingFont_ = true;
        fontDebouncer_->start();
    }

    void onFontDebounce_()
    {
        if (!hasPendingFont_) return;
        emit fontSaveRequested(pendingFont_);
        hasPendingFont_ = false;
    }
};

} // namespace Fernanda*/
