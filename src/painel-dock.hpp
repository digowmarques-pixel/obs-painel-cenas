/*
Painel de Cenas para OBS Studio
Copyright (C) 2026

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#pragma once

#include <QFrame>
#include <QWidget>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include <obs.hpp>
#include <obs-frontend-api.h>

class QLabel;
class QPushButton;
class QComboBox;
class QSlider;
class QScrollArea;
class QGridLayout;
class SceneDisplay;

/* Um quadradinho da grade: imagem ao vivo da cena + número + nome */
class SceneTile : public QFrame {
	Q_OBJECT

public:
	enum class State { Normal, Preview, Program };

	SceneTile(obs_source_t *scene, int number, QWidget *parent = nullptr);
	~SceneTile() override;

	obs_source_t *scene() const { return sceneRef; }
	QString sceneName() const;
	void setNumber(int number);
	void setState(State state);
	void setDisplayHeight(int h);
	void refreshName();
	void release();

signals:
	void clicked(SceneTile *tile);
	void doubleClicked(SceneTile *tile);

private:
	OBSSourceAutoRelease sceneRef;
	SceneDisplay *display = nullptr;
	QLabel *numberLabel = nullptr;
	QLabel *nameLabel = nullptr;
	QLabel *badge = nullptr;
	State state = State::Normal;
};

class PainelDock : public QWidget {
	Q_OBJECT

public:
	explicit PainelDock(QWidget *parent = nullptr);
	~PainelDock() override;

	static void frontendEvent(enum obs_frontend_event event, void *data);
	static void sourceRenamed(void *data, calldata_t *cd);

public slots:
	void rebuildScenes();
	void clearScenes();
	void updateStates();
	void refreshTransitions();
	void refreshOutputs();
	void refreshNames();

protected:
	void resizeEvent(QResizeEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
	void onTileClicked(SceneTile *tile);
	void onTileDoubleClicked(SceneTile *tile);
	void onStudioToggled();
	void onTransitionSelected(int index);
	void doTransition();
	void doCut();
	void onRecordClicked();
	void onStreamClicked();
	void updateTimes();
	void relayout();

private:
	void selectScene(obs_source_t *scene);
	void goLive(obs_source_t *scene);
	bool confirmSecondClick(QPushButton *button, const QString &question);
	void resetConfirm(QPushButton *button);
	static QString formatTime(obs_output_t *output);

	QVector<SceneTile *> tiles;
	QScrollArea *scroll = nullptr;
	QWidget *gridHost = nullptr;
	QGridLayout *grid = nullptr;

	QPushButton *studioButton = nullptr;
	QComboBox *transitionCombo = nullptr;
	QPushButton *cutButton = nullptr;
	QPushButton *transitionButton = nullptr;
	QPushButton *recordButton = nullptr;
	QPushButton *streamButton = nullptr;
	QSlider *sizeSlider = nullptr;

	QTimer timeTimer;
	QPointer<QPushButton> confirming;
	QTimer confirmTimer;
	bool updatingTransitions = false;
	bool loaded = false;
	int lastColumns = -1;
};
