/*
 * Fernanda is a plain text editor for fiction writing
 * Copyright (C) 2025-2026 fairybow
 *
 * This program is free software, redistributable and/or modifiable under the
 * terms of the GNU GPL v3. It's distributed in the hope that it will be useful
 * but without any warranty (even the implied warranty of merchantability or
 * fitness for a particular purpose)
 *
 * See the LICENSE file or visit <https://www.gnu.org/licenses/>
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QPlainTextEdit>
#include <QStatusBar>

#include "core/Debug.h"
#include "modules/WordCounter.h"
#include "services/AbstractService.h"
#include "settings/Ini.h"
#include "ui/Window.h"
#include "views/TextFileView.h"
#include "workspaces/Bus.h"

namespace Fernanda {

// Coordinator for Window WordCounters
class WordCounterModule : public AbstractService
{
    Q_OBJECT

public:
    WordCounterModule(Bus* bus, QObject* parent = nullptr)
        : AbstractService(bus, parent)
    {
        setup_();
    }

    virtual ~WordCounterModule() override { TRACER; }

protected:
    virtual void registerBusCommands() override
    {
        //...
    }

    virtual void connectBusEvents() override
    {
        connect(
            bus,
            &Bus::windowCreated,
            this,
            &WordCounterModule::onBusWindowCreated_);

        connect(
            bus,
            &Bus::windowDestroyed,
            this,
            &WordCounterModule::onBusWindowDestroyed_);

        connect(
            bus,
            &Bus::activeFileViewChanged,
            this,
            &WordCounterModule::onBusActiveFileViewChanged_);

        connect(
            bus,
            &Bus::settingChanged,
            this,
            &WordCounterModule::onBusSettingChanged_);
    }

private:
    QHash<Window*, WordCounter*> wordCounters_{};

    void setup_()
    {
        //...
    }

    void applyInitialSettings_(WordCounter* wordCounter)
    {
        if (!wordCounter) return;

        // TODO: Use this or comparable refactor in other places
        auto get = [this](const char* key, auto defaultVal) {
            return bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", key }, { "defaultValue", defaultVal } });
        };

        wordCounter->setActive(
            get(Ini::Keys::WORD_COUNTER_ACTIVE,
                Ini::Defaults::wordCounterActive()));
        wordCounter->setHasLineCount(
            get(Ini::Keys::WORD_COUNTER_LINE_COUNT,
                Ini::Defaults::wordCounterLineCount()));
        wordCounter->setHasWordCount(
            get(Ini::Keys::WORD_COUNTER_WORD_COUNT,
                Ini::Defaults::wordCounterWordCount()));
        wordCounter->setHasCharCount(
            get(Ini::Keys::WORD_COUNTER_CHAR_COUNT,
                Ini::Defaults::wordCounterCharCount()));
        wordCounter->setHasSelectionCounts(
            get(Ini::Keys::WORD_COUNTER_SELECTION,
                Ini::Defaults::wordCounterSelection()));
        wordCounter->setHasSelectionReplacement(
            get(Ini::Keys::WORD_COUNTER_SEL_REPLACE,
                Ini::Defaults::wordCounterSelReplace()));
        wordCounter->setHasLinePosition(
            get(Ini::Keys::WORD_COUNTER_LINE_POS,
                Ini::Defaults::wordCounterLinePos()));
        wordCounter->setHasColumnPosition(
            get(Ini::Keys::WORD_COUNTER_COL_POS,
                Ini::Defaults::wordCounterColPos()));
    }

    template <typename CallableT> void forEachWordCounter_(CallableT&& callable)
    {
        for (auto& wc : wordCounters_)
            if (wc) callable(wc);
    }

private slots:
    void onBusWindowCreated_(Window* window)
    {
        if (!window) return;

        auto word_counter = new WordCounter(window);
        wordCounters_[window] = word_counter;

        applyInitialSettings_(word_counter);
        window->statusBar()->addPermanentWidget(word_counter);
    }

    void onBusWindowDestroyed_(Window* window)
    {
        if (!window) return;
        wordCounters_.remove(window);
    }

    // Active view can be nullptr!
    // TODO: Use PlainTextEdit subclass or nah? Would move the WC widget to PTE
    // filter, then
    void onBusActiveFileViewChanged_(
        Window* window,
        AbstractFileView* activeFileView)
    {
        auto word_counter = wordCounters_.value(window);
        if (!word_counter) return;

        QPlainTextEdit* editor = nullptr;

        if (auto text_view = qobject_cast<TextFileView*>(activeFileView))
            editor = text_view->editor();

        word_counter->setTextEdit(editor);
    }

    void onBusSettingChanged_(const QString& key, const QVariant& value)
    {
        if (key == Ini::Keys::WORD_COUNTER_ACTIVE)
            forEachWordCounter_(
                [value](WordCounter* wc) { wc->setActive(value.toBool()); });

        if (key == Ini::Keys::WORD_COUNTER_LINE_COUNT)
            forEachWordCounter_([value](WordCounter* wc) {
                wc->setHasLineCount(value.toBool());
            });

        if (key == Ini::Keys::WORD_COUNTER_WORD_COUNT)
            forEachWordCounter_([value](WordCounter* wc) {
                wc->setHasWordCount(value.toBool());
            });

        if (key == Ini::Keys::WORD_COUNTER_CHAR_COUNT)
            forEachWordCounter_([value](WordCounter* wc) {
                wc->setHasCharCount(value.toBool());
            });

        if (key == Ini::Keys::WORD_COUNTER_SELECTION)
            forEachWordCounter_([value](WordCounter* wc) {
                wc->setHasSelectionCounts(value.toBool());
            });

        if (key == Ini::Keys::WORD_COUNTER_SEL_REPLACE)
            forEachWordCounter_([value](WordCounter* wc) {
                wc->setHasSelectionReplacement(value.toBool());
            });

        if (key == Ini::Keys::WORD_COUNTER_LINE_POS)
            forEachWordCounter_([value](WordCounter* wc) {
                wc->setHasLinePosition(value.toBool());
            });

        if (key == Ini::Keys::WORD_COUNTER_COL_POS)
            forEachWordCounter_([value](WordCounter* wc) {
                wc->setHasColumnPosition(value.toBool());
            });
    }
};

} // namespace Fernanda
