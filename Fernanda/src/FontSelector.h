#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QObject>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "Coco/Debug.h"
#include "Coco/Layout.h"

#include "DisplaySlider.h"
#include "Ini.h"
#include "Tr.h"

namespace Fernanda {

class FontSelector : public QWidget
{
    Q_OBJECT

public:
    FontSelector(const QFont& initialFont, QWidget* parent = nullptr)
        : QWidget(parent)
        , currentFont_(initialFont)
    {
        initialize_();
    }

    virtual ~FontSelector() override { COCO_TRACER; }

signals:
    void currentChanged(const QFont& font);

private:
    QFont currentFont_;

    QComboBox* fontsBox_ = new QComboBox(this);
    QCheckBox* boldCheckBox_ = new QCheckBox(
        Tr::FontSelector::bold(),
        this); // Eventually, use custom toggle switch widget (maybe)
    QCheckBox* italicCheckBox_ =
        new QCheckBox(Tr::FontSelector::italic(), this); // ^
    DisplaySlider* sizeSlider_ = new DisplaySlider(this);

private:
    void initialize_()
    {
        // Setup
        QFontDatabase db{};
        fontsBox_->addItems(db.families());
        boldCheckBox_->setTristate(false);
        italicCheckBox_->setTristate(false);
        sizeSlider_->setRange(
            Ini::Editor::FONT_PT_SIZE_MIN,
            Ini::Editor::FONT_PT_SIZE_MAX);

        // Populate
        fontsBox_->setCurrentText(currentFont_.family());
        boldCheckBox_->setChecked(currentFont_.bold());
        italicCheckBox_->setChecked(currentFont_.italic());
        sizeSlider_->setValue(currentFont_.pointSize());

        // Layout
        auto main_layout = Coco::Layout::make<QVBoxLayout*>(this);
        auto top_layout = Coco::Layout::make<QHBoxLayout*>();

        top_layout->addWidget(fontsBox_, 1);
        top_layout->addWidget(boldCheckBox_, 0);
        top_layout->addWidget(italicCheckBox_, 0);

        main_layout->addLayout(top_layout);
        main_layout->addWidget(sizeSlider_);

        // Connect
        connect(
            fontsBox_,
            &QComboBox::currentTextChanged,
            this,
            [&](const QString& text) {
                currentFont_.setFamily(text);
                emit currentChanged(currentFont_);
            });

        connect(boldCheckBox_, &QCheckBox::toggled, this, [&](bool checked) {
            currentFont_.setBold(checked);
            emit currentChanged(currentFont_);
        });

        connect(italicCheckBox_, &QCheckBox::toggled, this, [&](bool checked) {
            currentFont_.setItalic(checked);
            emit currentChanged(currentFont_);
        });

        connect(
            sizeSlider_,
            &DisplaySlider::valueChanged,
            this,
            [&](int value) {
                currentFont_.setPointSize(value);
                emit currentChanged(currentFont_);
            });
    }
};

} // namespace Fernanda
