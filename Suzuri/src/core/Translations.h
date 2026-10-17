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

#include <QCoreApplication>
#include <QDir>
#include <QLatin1StringView>
#include <QLocale>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTranslator>

#include <Coco/Debug.h>

namespace Fernanda {

using namespace Qt::StringLiterals;

// The language of Suzuri's interface, and the translators that put it there.
// Owned by App, which installs them at launch, before any text is made, and
// saves the setting (suzuri.json); the General settings page shows and
// changes it. Text is set once, as each widget is built, so a new setting
// takes effect at the next launch.
//
// The catalogs are embedded under :/i18n (CMakeLists.txt, Translations), and
// the available languages are the ones found there: English, whose catalog
// holds only its plural forms ("%n word(s)" as "1 word", "2 words"), and each
// Translation_<lang>.qm. A language other than English also gets Qt's own
// catalog for it (qtbase_<lang>.qm), for the text Qt draws itself, such as
// standard dialog buttons.
//
// Only text follows the language. Numbers and dates keep the system's format
class Translations : public QObject
{
    Q_OBJECT

public:
    Translations() = default;

    // The languages there are catalogs for, as language codes, English first
    [[nodiscard]] static QStringList available()
    {
        QStringList languages{ u"en"_s };

        auto names =
            QDir(u":/i18n"_s)
                .entryList({ u"Translation_*.qm"_s }, QDir::Files, QDir::Name);

        for (const auto& name : names) {
            auto language = name.sliced(PREFIX_.size()).chopped(SUFFIX_.size());
            if (!languages.contains(language)) {
                languages << language;
            }
        }

        return languages;
    }

    // The language to use: setting, when it names an available one; or else
    // the first of the system's preferred languages (uiLanguages, as "es-MX"
    // or "en") that's available; or else English
    [[nodiscard]] static QString pick(
        const QStringList& available,
        const QString& setting,
        const QStringList& uiLanguages)
    {
        if (!setting.isEmpty() && available.contains(setting)) {
            return setting;
        }

        for (const auto& ui_language : uiLanguages) {
            auto language =
                ui_language.section(u'-', 0, 0).section(u'_', 0, 0).toLower();

            if (available.contains(language)) {
                return language;
            }
        }

        return u"en"_s;
    }

    // A language's name in that language, for a list to choose from
    [[nodiscard]] static QString displayName(const QString& language)
    {
        // QLocale gives some names with a region ("American English"), so the
        // shipped languages are named here
        if (language == u"en"_s) {
            return u"English"_s;
        }

        if (language == u"es"_s) {
            return u"Español"_s;
        }

        return QLocale(language).nativeLanguageName();
    }

    // The saved setting: a language code, or empty to follow the system
    [[nodiscard]] const QString& setting() const noexcept { return setting_; }

    // The language in use since launch
    [[nodiscard]] const QString& language() const noexcept { return language_; }

    // Whether the setting picks a language other than the one in use, so
    // Suzuri has to start again to show it
    [[nodiscard]] bool needsRestart() const
    {
        return pick(available(), setting_, QLocale::system().uiLanguages()) !=
               language_;
    }

    // Takes effect at the next launch
    void setSetting(const QString& setting)
    {
        if (setting_ == setting) {
            return;
        }

        setting_ = setting;
        emit settingChanged();
    }

    // Install the translators for the saved setting. Once, by App, before
    // any widget or action is made
    void install(const QString& setting)
    {
        setting_ = setting;
        language_ =
            pick(available(), setting_, QLocale::system().uiLanguages());

        install_(plurals_, PREFIX_ + u"en"_s + SUFFIX_);

        if (language_ != u"en"_s) {
            install_(app_, PREFIX_ + language_ + SUFFIX_);
            install_(qt_, u"qtbase_"_s + language_ + SUFFIX_);
        }

        DEBUG("Language: {}", language_);
    }

signals:
    // The setting changed. App saves it
    void settingChanged();

private:
    static constexpr auto PREFIX_ = "Translation_"_L1;
    static constexpr auto SUFFIX_ = ".qm"_L1;

    QString setting_{};
    QString language_ = u"en"_s;

    // English's plural forms, the chosen language's catalog, and Qt's own.
    // Installed last is searched first, so the chosen language's text is found
    // before English's
    QTranslator plurals_{};
    QTranslator app_{};
    QTranslator qt_{};

    static void install_(QTranslator& translator, const QString& fileName)
    {
        if (!translator.load(fileName, u":/i18n"_s)) {
            WARN("Translation not found: {}", fileName);
            return;
        }

        QCoreApplication::installTranslator(&translator);
    }
};

} // namespace Fernanda
