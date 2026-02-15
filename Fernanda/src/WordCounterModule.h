/*
 * Fernanda  Copyright (C) 2025-2026  fairybow
 *
 * Licensed under GPL 3 with additional terms under Section 7. See LICENSE and
 * ADDITIONAL_TERMS files, or visit: <https://www.gnu.org/licenses/>
 *
 * Uses Qt 6 - <https://www.qt.io/>
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QPlainTextEdit>
#include <QStatusBar>

#include "AbstractService.h"
#include "Bus.h"
#include "Debug.h"
#include "Ini.h"
#include "TextFileView.h"
#include "Window.h"
#include "WordCounter.h"

namespace Fernanda {

// Coordinator for Window WordCounters
// TODO: Where to get word counter settings and also implement on startup
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
        auto get = [&](const char* key, auto defaultVal) {
            return bus->call<bool>(
                Bus::GET_SETTING,
                { { "key", key }, { "defaultValue", defaultVal } });
        };

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
        if (key == Ini::Keys::WORD_COUNTER_LINE_COUNT)
            forEachWordCounter_(
                [&](WordCounter* wc) { wc->setHasLineCount(value.toBool()); });

        if (key == Ini::Keys::WORD_COUNTER_WORD_COUNT)
            forEachWordCounter_(
                [&](WordCounter* wc) { wc->setHasWordCount(value.toBool()); });

        if (key == Ini::Keys::WORD_COUNTER_CHAR_COUNT)
            forEachWordCounter_(
                [&](WordCounter* wc) { wc->setHasCharCount(value.toBool()); });

        if (key == Ini::Keys::WORD_COUNTER_SELECTION)
            forEachWordCounter_([&](WordCounter* wc) {
                wc->setHasSelectionCounts(value.toBool());
            });

        if (key == Ini::Keys::WORD_COUNTER_SEL_REPLACE)
            forEachWordCounter_([&](WordCounter* wc) {
                wc->setHasSelectionReplacement(value.toBool());
            });

        if (key == Ini::Keys::WORD_COUNTER_LINE_POS)
            forEachWordCounter_([&](WordCounter* wc) {
                wc->setHasLinePosition(value.toBool());
            });

        if (key == Ini::Keys::WORD_COUNTER_COL_POS)
            forEachWordCounter_([&](WordCounter* wc) {
                wc->setHasColumnPosition(value.toBool());
            });
    }
};

} // namespace Fernanda
