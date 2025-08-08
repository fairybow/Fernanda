#pragma once

#include <functional>

#include <QCloseEvent>
#include <QMainWindow>
#include <QObject>
#include <QPointer>
#include <QWidget>

#include "Coco/Concepts.h"

class Window : public QMainWindow
{
    Q_OBJECT

public:
    friend class WindowManager;
    using CloseAcceptor = std::function<bool(Window*)>;

    explicit Window(QWidget* parent = nullptr);
    virtual ~Window() override;

    CloseAcceptor closeAcceptor() const noexcept { return closeAcceptor_; }
    void setCloseAcceptor(const CloseAcceptor& closeAcceptor) { closeAcceptor_ = closeAcceptor; }

    template <typename ClassT>
    void setCloseAcceptor(ClassT* object, bool (ClassT::* method)(Window*))
    {
        closeAcceptor_ = [object, method](Window* window)
            {
                return (object->*method)(window);
            };
    }

    void activate()
    {
        raise();
        activateWindow();
    }

signals:
    void destroyed(Window*);

protected:
    virtual void closeEvent(QCloseEvent* event) override;

private:
    QPointer<WindowManager> manager_ = nullptr; // Only assigned by WindowManager
    CloseAcceptor closeAcceptor_ = nullptr;
};

template<typename T>
concept WindowPointer = Coco::Concepts::DerivedPointer<Window, T>;
