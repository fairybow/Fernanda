/*#include "Splitter.h"

Splitter::Splitter(const char* name, Qt::Orientation orientation, QWidgetList widgets, QWidget* parent)
	: Widget(name, parent)
{
	m_trueSplitter->setObjectName(name);
	Layout::box(m_trueSplitter, this);
	m_trueSplitter->setOrientation(orientation);
	for (auto& widget : widgets)
		m_trueSplitter->addWidget(widget);
	connect(m_trueSplitter, &TrueSplitter::splitterMoved, this, [&] { moveButtons(); });
	connect(m_trueSplitter, &TrueSplitter::resized, this, [&] { moveButtons(); });
	connect(m_trueSplitter, &TrueSplitter::widgetVisibilityChanged, this, &Splitter::showOrHideButtons);
}

void Splitter::initialize(QVector<double> fallbacks, int centralWidgetIndex)
{
	m_centralWidgetIndex = centralWidgetIndex;
	for (auto i = 0; i < m_trueSplitter->count(); ++i) {
		auto widget_i = m_trueSplitter->widget(i);
		auto meta = Meta{ i };
		if (i != centralWidgetIndex) {
			auto button = new QPushButton(QString(isLeft(i) ? "Left Collapse" : "Right Collapse"), this);
			//connect(button, &QPushButton::clicked, this, [&] { emit askChangeMetaState(button); });
			meta.handleButton = button;
		}
		m_metas << meta;
		m_trueSplitter->setCollapsible(i, (i != centralWidgetIndex));
		m_trueSplitter->setStretchFactor(i, (isCentral(i) || isRight(i)));
	}
	m_trueSplitter->setSizes(verifyFallbacks(fallbacks));
}

Splitter::Alignment Splitter::findAlignment(int widgetIndex) const
{
	auto alignment = widgetIndex <=> m_centralWidgetIndex;
	if (alignment == std::strong_ordering::less)
		return Alignment::Left;
	else if (alignment == std::strong_ordering::greater)
		return Alignment::Right;
	else
		return Alignment::Central;
}

QVector<int> Splitter::verifyFallbacks(QVector<double> fallbacks)
{
	auto total = std::accumulate(fallbacks.begin(), fallbacks.end(), 0.0);
	if (total != 1.0) {
		auto size = fallbacks.size();
		auto adjustment = (1.0 - total) / size;
		for (auto i = 0; i < size; ++i)
			fallbacks[i] += adjustment;
	}
	QVector<int> sizes;
	auto width = emit askWindowSize().width() - (m_trueSplitter->handleWidth() * (m_trueSplitter->count() - 1));
	for (auto& fallback : fallbacks)
		sizes << static_cast<int>(width * fallback);
	return sizes;
}

void Splitter::moveButtons()
{
	for (auto& m_meta : m_metas) {
		auto button = m_meta.button();
		if (button == nullptr) continue;
		auto half_y = height() / 2;
		auto half_button_x = button->width() / 2;
		auto half_button_y = button->height() / 2;
		auto handle_x = m_trueSplitter->handle(associatedHandle(m_meta.widgetIndex))->x();
		button->move(handle_x - half_button_x, half_y - half_button_y);
	}
}

void Splitter::showOrHideButtons(int widgetIndex, TrueSplitter::WidgetWas visibility)
{
	for (auto& m_meta : m_metas) {
		if (m_meta.widgetIndex != widgetIndex) continue;
		auto button = m_meta.button();
		if (button == nullptr) continue;
		(visibility == TrueSplitter::WidgetWas::Hidden) ? button->hide() : button->show();
	}
}
*/
