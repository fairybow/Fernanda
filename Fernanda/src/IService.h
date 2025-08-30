#pragma once

#include <QObject>

namespace Fernanda {

class Commander;
class EventBus;

// Base class for Workspace's Services and Modules, providing protected
// Commander and EventBus member pointers
class IService : public QObject
{
    Q_OBJECT

public:
    explicit IService(
        Commander* commander,
        EventBus* eventBus,
        QObject* parent = nullptr)
        : QObject(parent)
        , commander(commander)
        , eventBus(eventBus)
    {
    }

    virtual ~IService() = default;

protected:
    Commander* commander;
    EventBus* eventBus;
};

} // namespace Fernanda
