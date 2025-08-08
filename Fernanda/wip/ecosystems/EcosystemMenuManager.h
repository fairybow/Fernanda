#pragma once

#include <QAction>
#include <QKeyCombination>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QString>

#include "Coco/Debug.h"

#include "window/Window.h"

class Ecosystem;

class EcosystemMenuManager : public QObject
{
    Q_OBJECT

public:
    explicit EcosystemMenuManager(Ecosystem* parentEcosystem);
    virtual ~EcosystemMenuManager() override;

    void initializeGlobalMenuToggles();
    void initializeWindowMenus(Window* window);

    void updateGlobalMenuToggles();
    void updatePerWindowMenuToggles(Window* window);
    void updateEditToggles(Window* window);
    void updateViewToggles(Window* window);
    void cleanup(Window* window);

private:
    Ecosystem* eco_;

    // Note: For this project, a "toggle" is something that itself toggles. A
    //       "toggler" is something that toggles a toggle. So, these are
    //       toggles, but a menu item that, say, turns the color bar on or off
    //       is a toggler.
    struct PerWindowToggles_
    {
        // File menu
        QAction* saveActiveFile = nullptr;
        QAction* saveActiveFileAs = nullptr;
        QAction* saveAllInWindow = nullptr;
        QAction* closeActiveFileView = nullptr;
        QAction* closeAllInWindow = nullptr;

        // Edit menu
        QAction* undo = nullptr;
        QAction* redo = nullptr;
        QAction* cut = nullptr;
        QAction* copy = nullptr;
        QAction* paste = nullptr;
        QAction* del = nullptr;
        QAction* selectAll = nullptr;

        // View menu
        QAction* previousTab = nullptr;
        QAction* nextTab = nullptr;
    };

    struct GlobalToggles_
    {
        // File menu
        QAction* saveAllInAllWindows = nullptr;
        QAction* closeAllInAllWindows = nullptr;

        // View menu
        QAction* previousWindow = nullptr;
        QAction* nextWindow = nullptr;
    };

    QHash<Window*, PerWindowToggles_> perWindowToggles_{};
    GlobalToggles_ globalToggles_{};

    void initializeFileMenu_(QMenuBar* menuBar, Window* window);
    void initializeEditMenu_(QMenuBar* menuBar, Window* window);
    void initializeViewMenu_(QMenuBar* menuBar, Window* window);
    void initializeSettingsMenu(QMenuBar* menuBar, Window* window);

    template<typename SlotT>
    QAction* addAction_
    (
        const QString& text,
        const QKeyCombination& shortcut,
        QMenu* menu,
        QObject* receiver,
        SlotT slot,
        Qt::ConnectionType connectionType = Qt::AutoConnection
    )
    {
        auto action = menu->addAction(text);
        connect(action, &QAction::triggered, receiver, slot, connectionType);
        action->setAutoRepeat(false);
        action->setShortcut(shortcut);
        return action;
    }

    template<typename SlotT>
    QAction* addAction_
    (
        const QString& text,
        QMenu* menu,
        QObject* receiver,
        SlotT slot,
        Qt::ConnectionType connectionType = Qt::AutoConnection
    )
    {
        return addAction_<SlotT>(text, {}, menu, receiver, slot, connectionType);
    }

    template<typename SlotT>
    QAction* addAction_
    (
        const QString& text,
        const QKeyCombination& shortcut,
        QMenu* menu,
        SlotT slot,
        Qt::ConnectionType connectionType = Qt::AutoConnection
    )
    {
        return addAction_<SlotT>(text, shortcut, menu, this, slot, connectionType);
    }

    template<typename SlotT>
    QAction* addAction_
    (
        const QString& text,
        QMenu* menu,
        SlotT slot,
        Qt::ConnectionType connectionType = Qt::AutoConnection
    )
    {
        return addAction_<SlotT>(text, {}, menu, this, slot, connectionType);
    }
};
