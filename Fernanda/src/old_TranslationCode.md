# Old Translation Code

```cpp
// In Application.h:

// (in initialize):
initializeSettings_();
setLanguageCode(languageCode());

QStringList languageCodes() const
{
    QStringList codes{};
    codes << DEFAULT_LANGUAGE_CODE_;

    for (auto& path : Coco::PathUtil::fromDir(":/tr/", TR_FILE_EXT_))
        codes << path.stemQString().remove(TR_FILE_PREFIX_);

    return codes;
}

QString languageCode() const
{
    return settings_->value(LANGUAGE_INI_KEY_, DEFAULT_LANGUAGE_CODE_)
        .toString();
}

void setLanguageCode(const QString& languageCode)
{
    if (translator_) {
        removeTranslator(translator_);
        delete translator_;
        translator_ = nullptr;
    }

    if (languageCode != DEFAULT_LANGUAGE_CODE_) {
        translator_ = new QTranslator(this);

        if (translator_->load(
                ":/tr/Translation_" + languageCode + TR_FILE_EXT_)) {
            installTranslator(translator_);
        } else {
            WARN("Failed to load translation: {}", languageCode);
            delete translator_;
            translator_ = nullptr;
        }
    }

    settings_->setValue(LANGUAGE_INI_KEY_, languageCode);
}

QString languageName(const QString& code) const
{
    // never was able to extract ONLY language name (e.g. "English" not
    // "American English") from QLocale
}

QSettings* settings_ = nullptr;
QTranslator* translator_ = nullptr;
constexpr static auto TR_FILE_PREFIX_ = "Translation_";
constexpr static auto TR_FILE_EXT_ = ".qm";
constexpr static auto LANGUAGE_INI_KEY_ = "Locale";
constexpr static auto DEFAULT_LANGUAGE_CODE_ = "en";

void initializeSettings_()
{
    settings_ = new QSettings(
        (AppDirs::userData() / "Application.ini").toQString(),
        QSettings::IniFormat,
        this);
}

// In Workspace.cpp (createWindowMenuBar, help menu)
.action(Tr::nxLanguage())
.onUserTrigger(this, [] { LanguageDialog::exec(); })
```

```cpp
// LanguageDialog

#pragma once

namespace Fernanda::LanguageDialog {

// Application-modal
void exec();

} // namespace Fernanda::LanguageDialog

//==========================================

#include "LanguageDialog.h"

#include <QComboBox>
#include <QDialog>
// #include <QDialogButtonBox>
#include <QLocale>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include "Application.h"
#include "Tr.h"

// TODO: Decide on button box. This is a live settings changer, like
// SettingsDialog which doesn't have a button. Should this match?
namespace Fernanda::LanguageDialog {

void exec()
{
    QDialog dialog{};

    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowTitle(Tr::nxLanguageTitle());
    // dialog.setMinimumSize(400, 200);

    auto selector = new QComboBox(&dialog);
    // auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    // buttons->button(QDialogButtonBox::Ok)->setText(Tr::ok());

    auto fernanda = app();
    // move to strings for this and get name from app too
    for (auto& code : fernanda->languageCodes())
        selector->addItem(fernanda->languageName(code), code);

    selector->setCurrentIndex(selector->findData(fernanda->languageCode()));

    auto main_layout = new QVBoxLayout(&dialog);
    main_layout->addWidget(selector);
    // main_layout->addWidget(buttons);

    // QObject::connect(
    //     buttons,
    //     &QDialogButtonBox::accepted,
    //     &dialog,
    //     &QDialog::accept);

    QObject::connect(
        selector,
        &QComboBox::currentIndexChanged,
        &dialog,
        [selector, fernanda](int index) {
            fernanda->setLanguageCode(selector->itemData(index).toString());
        });

    // TODO: Move to open/show later (not yet)
    dialog.exec();
}

} // namespace Fernanda::LanguageDialog
```
