/*
Painel de Cenas para OBS Studio
Copyright (C) 2026

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include <QWidget>
#include <mutex>
#include <obs.hpp>

/*
 * Widget nativo que desenha uma cena do OBS usando a placa de vídeo,
 * do mesmo jeito que a Prévia e a Visualização múltipla do OBS.
 */
class SceneDisplay : public QWidget {
	Q_OBJECT

public:
	explicit SceneDisplay(QWidget *parent = nullptr);
	~SceneDisplay() override;

	void setSource(obs_source_t *source);
	void clearSource();

	QPaintEngine *paintEngine() const override { return nullptr; }

signals:
	void clicked();
	void doubleClicked();

protected:
	bool event(QEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
	static void drawCallback(void *data, uint32_t cx, uint32_t cy);
	void createDisplay();
	void destroyDisplay();
	QSize pixelSize() const;

	obs_display_t *display = nullptr;
	std::mutex sourceMutex;
	OBSWeakSourceAutoRelease weakSource;
	OBSSourceAutoRelease showingSource; // mantém a cena "visível" para que suas fontes rendam
};
