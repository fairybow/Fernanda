#pragma once

#include <functional>

#include <QDialog>
#include <QFont>
#include <QObject>
#include <QVBoxLayout>
#include <QWidget>

#include "Coco/Debug.h"
#include "Coco/Layout.h"

#include "Debouncer.h"
#include "FontSelector.h"

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
        initialize_(initialFont);
    }

    virtual ~SettingsDialog() override { COCO_TRACER; }

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
    void fontPersistenceRequested(const QFont& font);

private:
    static constexpr auto DEBOUCE_MS_ = 500;

    FontSelector* fontSelector_ = nullptr;
    FontChangeHandler fontChangeHandler_ = nullptr;
    QFont pendingFont_{};
    bool hasPendingFont_ = false;
    Debouncer* fontDebouncer_ =
        new Debouncer(DEBOUCE_MS_, this, &SettingsDialog::onFontDebounce_);

    void initialize_(const QFont& initialFont)
    {
        fontSelector_ = new FontSelector(initialFont, this);
        setModal(false);

        auto layout = Coco::Layout::makeDense<QVBoxLayout*>(this);
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

        emit fontPersistenceRequested(pendingFont_);
        hasPendingFont_ = false;
    }
};

} // namespace Fernanda
