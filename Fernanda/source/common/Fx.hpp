#pragma once

#include <QGraphicsBlurEffect>
#include <QGraphicsOpacityEffect>
#include <QWidget>

namespace Fx
{
	inline QGraphicsBlurEffect* blur(int radius = 5, QWidget* parent = nullptr)
	{
		auto effect = new QGraphicsBlurEffect(parent);
		effect->setBlurHints(QGraphicsBlurEffect::QualityHint);
		effect->setBlurRadius(radius);
		parent->setGraphicsEffect(effect);
		return effect;
	}

	inline QGraphicsOpacityEffect* opacify(double opacity = 0.5, QWidget* parent = nullptr)
	{
		auto effect = new QGraphicsOpacityEffect(parent);
		effect->setOpacity(opacity);
		parent->setGraphicsEffect(effect);
		return effect;
	}
}
