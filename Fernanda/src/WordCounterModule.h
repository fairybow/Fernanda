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
#include "TextFileView.h"
#include "Window.h"
#include "WordCounter.h"

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
    }

private:
    QHash<Window*, WordCounter*> wordCounters_{};

    void setup_()
    {
        //...
    }

private slots:
    void onBusWindowCreated_(Window* window)
    {
        if (!window) return;

        auto word_counter = new WordCounter(window);
        wordCounters_[window] = word_counter;
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
};

} // namespace Fernanda
