#pragma once

#include <QWidget>

#include <utility>

#define lambdaEmit(signalCall) [&]{signalCall();}

template<typename T = QWidget, typename... Args>
class Widget : public T
{
public:
	explicit Widget(QWidget* parent = nullptr, Args&&... arg)
		: T(parent, std::forward<Args>(arg)...) {}

	explicit Widget(const char* objectName, QWidget* parent = nullptr, Args&&... arg)
		: T(parent, std::forward<Args>(arg)...)
	{
		this->setObjectName(objectName);
	}

	template<typename Sender, typename Receiver, typename Slot, typename... Signals>
	void connectMultipleSignals(Sender sender, Receiver receiver, Slot slot, Signals... signal)
	{
		(QObject::connect(sender, signal, receiver, slot), ...);
	}
};
