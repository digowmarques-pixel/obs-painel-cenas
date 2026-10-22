/*
Painel de Cenas para OBS Studio
Copyright (C) 2026

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "scene-display.hpp"

#include <QMouseEvent>
#include <QShowEvent>
#include <QResizeEvent>
#include <QWindow>
#include <QScreen>
#include <algorithm>
#include <cmath>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <obs-nix-platform.h>
#endif

SceneDisplay::SceneDisplay(QWidget *parent) : QWidget(parent)
{
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_StaticContents);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_DontCreateNativeAncestors);
	setAttribute(Qt::WA_NativeWindow);
	setCursor(Qt::PointingHandCursor);
	setMinimumSize(32, 18);
}

SceneDisplay::~SceneDisplay()
{
	destroyDisplay();
	clearSource();
}

void SceneDisplay::setSource(obs_source_t *source)
{
	clearSource();
	if (!source)
		return;

	{
		std::lock_guard<std::mutex> lock(sourceMutex);
		weakSource = obs_source_get_weak_source(source);
	}
	showingSource = obs_source_get_ref(source);
	if (showingSource)
		obs_source_inc_showing(showingSource);
}

void SceneDisplay::clearSource()
{
	{
		std::lock_guard<std::mutex> lock(sourceMutex);
		weakSource = nullptr;
	}
	if (showingSource) {
		obs_source_dec_showing(showingSource);
		showingSource = nullptr;
	}
}

QSize SceneDisplay::pixelSize() const
{
	const qreal ratio = devicePixelRatioF();
	return QSize((int)std::lround(width() * ratio), (int)std::lround(height() * ratio));
}

void SceneDisplay::createDisplay()
{
	if (display || !isVisible() || !windowHandle())
		return;

	const QSize size = pixelSize();
	if (size.width() <= 0 || size.height() <= 0)
		return;

	gs_init_data info = {};
	info.cx = (uint32_t)size.width();
	info.cy = (uint32_t)size.height();
	info.format = GS_BGRA;
	info.zsformat = GS_ZS_NONE;

#ifdef _WIN32
	info.window.hwnd = (void *)winId();
#elif defined(__APPLE__)
	info.window.view = (id)winId();
#else
	if (obs_get_nix_platform() == OBS_NIX_PLATFORM_WAYLAND)
		return; // Wayland não é suportado por este painel
	info.window.id = (uint32_t)winId();
	info.window.display = obs_get_nix_platform_display();
#endif

	display = obs_display_create(&info, 0x000000);
	if (display)
		obs_display_add_draw_callback(display, drawCallback, this);
}

void SceneDisplay::destroyDisplay()
{
	if (!display)
		return;
	obs_display_remove_draw_callback(display, drawCallback, this);
	obs_display_destroy(display);
	display = nullptr;
}

void SceneDisplay::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	createDisplay();
}

void SceneDisplay::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	if (display) {
		const QSize size = pixelSize();
		if (size.width() > 0 && size.height() > 0)
			obs_display_resize(display, (uint32_t)size.width(), (uint32_t)size.height());
	} else {
		createDisplay();
	}
}

void SceneDisplay::paintEvent(QPaintEvent *)
{
	createDisplay();
}

bool SceneDisplay::event(QEvent *event)
{
	/* Ao soltar ou encaixar o painel, o Qt recria a janela nativa: refaz o display */
	if (event->type() == QEvent::WinIdChange) {
		destroyDisplay();
		createDisplay();
	}
	return QWidget::event(event);
}

void SceneDisplay::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		emit clicked();
	QWidget::mousePressEvent(event);
}

void SceneDisplay::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		emit doubleClicked();
	QWidget::mouseDoubleClickEvent(event);
}

/* Roda na thread de vídeo do OBS */
void SceneDisplay::drawCallback(void *data, uint32_t cx, uint32_t cy)
{
	auto *self = static_cast<SceneDisplay *>(data);

	OBSSourceAutoRelease source;
	{
		std::lock_guard<std::mutex> lock(self->sourceMutex);
		if (self->weakSource)
			source = obs_weak_source_get_source(self->weakSource);
	}
	if (!source)
		return;

	const uint32_t sw = obs_source_get_width(source);
	const uint32_t sh = obs_source_get_height(source);
	if (!sw || !sh || !cx || !cy)
		return;

	const float scale = std::min((float)cx / (float)sw, (float)cy / (float)sh);
	const int w = (int)std::lround(sw * scale);
	const int h = (int)std::lround(sh * scale);
	const int x = ((int)cx - w) / 2;
	const int y = ((int)cy - h) / 2;

	gs_projection_push();
	gs_viewport_push();
	gs_set_viewport(x, y, w, h);
	gs_ortho(0.0f, (float)sw, 0.0f, (float)sh, -100.0f, 100.0f);
	obs_source_video_render(source);
	gs_viewport_pop();
	gs_projection_pop();
}
