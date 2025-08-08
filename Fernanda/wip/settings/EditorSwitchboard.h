#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QTextOption>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include "Coco/Debug.h"
#include "Coco/Layout.h"

#include "DisplaySlider.h"
#include "Ini.h"
#include "Tr.h"

class EditorSwitchboard : public QWidget
{
    Q_OBJECT

public:
    EditorSwitchboard(const Ini::Editor::Values& initialValues, QWidget* parent = nullptr)
        : QWidget(parent), currentValues_(initialValues)
    {
        initialize_();
    }

    virtual ~EditorSwitchboard() override { COCO_TRACER; }

signals:
    void currentChanged(const Ini::Editor::Values& values);

private:
    Ini::Editor::Values currentValues_;

    QCheckBox* centerOnScrollCheckBox_ = new QCheckBox(Tr::EditorSb::centerOnScroll(), this);
    QCheckBox* overwriteModeCheckBox_ = new QCheckBox(Tr::EditorSb::overwrite(), this);
    QLabel* tabStopDistanceLabel_ = new QLabel(Tr::EditorSb::tabStopDistance(), this);
    DisplaySlider* tabStopDistanceSlider_ = new DisplaySlider(this);
    QLabel* wordWrapModeLabel_ = new QLabel(Tr::EditorSb::wordWrapMode(), this);
    QComboBox* wordWrapModeComboBox_ = new QComboBox(this);

private:
    void initialize_()
    {
        // Setup
        centerOnScrollCheckBox_->setTristate(false);
        overwriteModeCheckBox_->setTristate(false);
        tabStopDistanceSlider_->setRange(Ini::Editor::TAB_STOP_PX_MIN, Ini::Editor::TAB_STOP_PX_MAX);
        wordWrapModeComboBox_->addItem(Tr::EditorSb::noWrap(), QTextOption::NoWrap);
        wordWrapModeComboBox_->addItem(Tr::EditorSb::wordWrap(), QTextOption::WordWrap);
        wordWrapModeComboBox_->addItem(Tr::EditorSb::wrapAnywhere(), QTextOption::WrapAnywhere);
        wordWrapModeComboBox_->addItem(Tr::EditorSb::wrapAtWordBoundaryOrAnywhere(), QTextOption::WrapAtWordBoundaryOrAnywhere);

        // Populate
        centerOnScrollCheckBox_->setChecked(currentValues_.centerOnScroll);
        overwriteModeCheckBox_->setChecked(currentValues_.overwrite);
        tabStopDistanceSlider_->setValue(currentValues_.tabStopPx);
        wordWrapModeComboBox_->setCurrentIndex(wordWrapModeComboBox_->findData(currentValues_.wordWrapMode));

        // Layout
        auto main_layout = Coco::Layout::make<QVBoxLayout*>(this);
        auto tab_stop_layout = Coco::Layout::make<QHBoxLayout*>();
        auto word_wrap_layout = Coco::Layout::make<QHBoxLayout*>();

        tab_stop_layout->addWidget(tabStopDistanceLabel_, 0);
        tab_stop_layout->addWidget(tabStopDistanceSlider_, 1);

        word_wrap_layout->addWidget(wordWrapModeLabel_, 0);
        word_wrap_layout->addWidget(wordWrapModeComboBox_, 1);

        main_layout->addWidget(centerOnScrollCheckBox_);
        main_layout->addWidget(overwriteModeCheckBox_);
        main_layout->addLayout(tab_stop_layout);
        main_layout->addLayout(word_wrap_layout);

        // Connect
        connect(centerOnScrollCheckBox_, &QCheckBox::toggled, this, [&](bool checked)
            {
                currentValues_.centerOnScroll = checked;
                emit currentChanged(currentValues_);
            });

        connect(overwriteModeCheckBox_, &QCheckBox::toggled, this, [&](bool checked)
            {
                currentValues_.overwrite = checked;
                emit currentChanged(currentValues_);
            });

        connect(tabStopDistanceSlider_, &DisplaySlider::valueChanged, this, [&](int value)
            {
                currentValues_.tabStopPx = value;
                emit currentChanged(currentValues_);
            });

        connect(wordWrapModeComboBox_, &QComboBox::currentIndexChanged, this, [&](int index)
            {
                currentValues_.wordWrapMode = wordWrapModeComboBox_->itemData(index).value<QTextOption::WrapMode>();
                emit currentChanged(currentValues_);
            });
    }
};
