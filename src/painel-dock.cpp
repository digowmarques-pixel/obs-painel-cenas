/*
Painel de Cenas para OBS Studio
Copyright (C) 2026

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include "painel-dock.hpp"
#include "scene-display.hpp"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QVBoxLayout>
#include <algorithm>
#include <cstring>

namespace {

const char *kAmarelo = "#FFC400";
const char *kVerde = "#22C55E";
const char *kVermelho = "#EF4444";
const char *kBorda = "#2E2E2E";
const int kGap = 8;

const char *kDockStyle = R"(
#PainelCenas { background: #0D0D0D; }
#PainelCenas QWidget#gridHost { background: #0D0D0D; }
#PainelCenas QScrollArea { border: none; background: #0D0D0D; }
#PainelCenas QWidget#barra, #PainelCenas QWidget#rodape { background: #141414; }
#PainelCenas QPushButton {
	background: #242424; color: #FFFFFF; border: 1px solid #2E2E2E;
	border-radius: 4px; padding: 5px 10px; font-weight: 600;
}
#PainelCenas QPushButton:hover { border-color: #FFC400; }
#PainelCenas QPushButton:disabled { color: #666; border-color: #2E2E2E; }
#PainelCenas QPushButton#btnTransicao { background: #FFC400; color: #0A0A0A; border-color: #FFC400; font-weight: 700; padding: 5px 16px; }
#PainelCenas QPushButton#btnTransicao:hover { background: #FFD43B; }
#PainelCenas QPushButton#btnTransicao:disabled { background: #4A3D00; color: #0A0A0A; border-color: #4A3D00; }
#PainelCenas QPushButton#btnEstudio:checked { background: #FFC400; color: #0A0A0A; border-color: #FFC400; }
#PainelCenas QPushButton[ativo="true"] { background: #EF4444; border-color: #EF4444; color: #FFFFFF; }
#PainelCenas QPushButton[confirmar="true"] { background: #FFC400; border-color: #FFC400; color: #0A0A0A; }
#PainelCenas QComboBox { background: #242424; color: #FFFFFF; border: 1px solid #2E2E2E; border-radius: 4px; padding: 4px 8px; }
#PainelCenas QLabel { color: #FFFFFF; }
#PainelCenas QLabel#dica { color: #9A9A9A; }
#PainelCenas QSlider::groove:horizontal { height: 4px; background: #2E2E2E; border-radius: 2px; }
#PainelCenas QSlider::handle:horizontal { background: #FFC400; width: 14px; margin: -5px 0; border-radius: 7px; }
#PainelCenas QSlider::sub-page:horizontal { background: #FFC400; border-radius: 2px; }
)";

void repolish(QWidget *w)
{
	w->style()->unpolish(w);
	w->style()->polish(w);
	w->update();
}

} // namespace

/* =========================== SceneTile =========================== */

SceneTile::SceneTile(obs_source_t *scene, int number, QWidget *parent) : QFrame(parent)
{
	sceneRef = obs_source_get_ref(scene);

	setCursor(Qt::PointingHandCursor);
	setFocusPolicy(Qt::NoFocus);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(3, 3, 3, 3);
	layout->setSpacing(0);

	display = new SceneDisplay(this);
	display->setSource(sceneRef);
	layout->addWidget(display);

	auto *row = new QWidget(this);
	row->setObjectName("rotulo");
	row->setStyleSheet("#rotulo { background: #1C1C1C; }");
	auto *rowLayout = new QHBoxLayout(row);
	rowLayout->setContentsMargins(6, 4, 6, 4);
	rowLayout->setSpacing(6);

	numberLabel = new QLabel(row);
	numberLabel->setAlignment(Qt::AlignCenter);
	numberLabel->setMinimumWidth(18);
	numberLabel->setStyleSheet(QString("background: %1; color: #0A0A0A; border-radius: 3px; "
					   "font-weight: 700; font-size: 11px; padding: 0 4px;")
					   .arg(kAmarelo));

	nameLabel = new QLabel(row);
	nameLabel->setStyleSheet("color: #FFFFFF; font-weight: 600;");
	nameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

	badge = new QLabel(row);
	badge->setVisible(false);

	rowLayout->addWidget(numberLabel);
	rowLayout->addWidget(nameLabel, 1);
	rowLayout->addWidget(badge);
	layout->addWidget(row);

	setNumber(number);
	refreshName();
	setState(State::Normal);

	connect(display, &SceneDisplay::clicked, this, [this]() { emit clicked(this); });
	connect(display, &SceneDisplay::doubleClicked, this, [this]() { emit doubleClicked(this); });
}

SceneTile::~SceneTile()
{
	release();
}

void SceneTile::release()
{
	if (display)
		display->clearSource();
	sceneRef = nullptr;
}

QString SceneTile::sceneName() const
{
	return sceneRef ? QString::fromUtf8(obs_source_get_name(sceneRef)) : QString();
}

void SceneTile::refreshName()
{
	const QString name = sceneName();
	nameLabel->setText(name);
	setToolTip(name);
}

void SceneTile::setNumber(int number)
{
	numberLabel->setText(QString::number(number));
}

void SceneTile::setDisplayHeight(int h)
{
	display->setFixedHeight(std::max(18, h));
}

void SceneTile::setState(State newState)
{
	state = newState;
	const char *color = kBorda;
	if (state == State::Program) {
		color = kVermelho;
		badge->setText("NO AR");
		badge->setStyleSheet(QString("background: %1; color: #FFFFFF; border-radius: 3px; "
					     "font-size: 10px; font-weight: 800; padding: 1px 6px;")
					     .arg(kVermelho));
		badge->setVisible(true);
	} else if (state == State::Preview) {
		color = kVerde;
		badge->setText(QString::fromUtf8("PRÉVIA"));
		badge->setStyleSheet(QString("background: %1; color: #000000; border-radius: 3px; "
					     "font-size: 10px; font-weight: 800; padding: 1px 6px;")
					     .arg(kVerde));
		badge->setVisible(true);
	} else {
		badge->setVisible(false);
	}
	setStyleSheet(QString("SceneTile { background: %1; border-radius: 6px; }").arg(color));
}

/* =========================== PainelDock =========================== */

PainelDock::PainelDock(QWidget *parent) : QWidget(parent)
{
	setObjectName("PainelCenas");
	setAttribute(Qt::WA_StyledBackground);
	setStyleSheet(kDockStyle);
	setFocusPolicy(Qt::ClickFocus);

	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);

	/* ---- Barra de controles ---- */
	auto *bar = new QWidget(this);
	bar->setObjectName("barra");
	bar->setAttribute(Qt::WA_StyledBackground);
	auto *barLayout = new QHBoxLayout(bar);
	barLayout->setContentsMargins(8, 6, 8, 6);
	barLayout->setSpacing(6);

	studioButton = new QPushButton(QString::fromUtf8("Modo estúdio"), bar);
	studioButton->setObjectName("btnEstudio");
	studioButton->setCheckable(true);
	studioButton->setToolTip(QString::fromUtf8("Liga/desliga o Modo de estúdio"));

	transitionCombo = new QComboBox(bar);
	transitionCombo->setToolTip(QString::fromUtf8("Transição usada no botão TRANSIÇÃO"));

	cutButton = new QPushButton("CORTE", bar);
	cutButton->setToolTip(QString::fromUtf8("Leva a Prévia para o ar sem efeito"));

	transitionButton = new QPushButton(QString::fromUtf8("TRANSIÇÃO"), bar);
	transitionButton->setObjectName("btnTransicao");
	transitionButton->setToolTip(QString::fromUtf8("Leva a Prévia para o ar (Espaço)"));

	recordButton = new QPushButton(QString::fromUtf8("● Gravar"), bar);
	streamButton = new QPushButton("Transmitir", bar);

	barLayout->addWidget(studioButton);
	barLayout->addStretch(1);
	barLayout->addWidget(transitionCombo);
	barLayout->addWidget(cutButton);
	barLayout->addWidget(transitionButton);
	barLayout->addStretch(1);
	barLayout->addWidget(recordButton);
	barLayout->addWidget(streamButton);
	root->addWidget(bar);

	/* ---- Grade de miniaturas ---- */
	scroll = new QScrollArea(this);
	scroll->setWidgetResizable(true);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setFrameShape(QFrame::NoFrame);

	gridHost = new QWidget(scroll);
	gridHost->setObjectName("gridHost");
	gridHost->setAttribute(Qt::WA_StyledBackground);
	grid = new QGridLayout(gridHost);
	grid->setContentsMargins(kGap, kGap, kGap, kGap);
	grid->setSpacing(kGap);
	grid->setAlignment(Qt::AlignTop);
	scroll->setWidget(gridHost);
	scroll->viewport()->installEventFilter(this);
	root->addWidget(scroll, 1);

	/* ---- Rodapé ---- */
	auto *footer = new QWidget(this);
	footer->setObjectName("rodape");
	footer->setAttribute(Qt::WA_StyledBackground);
	auto *footLayout = new QHBoxLayout(footer);
	footLayout->setContentsMargins(8, 4, 8, 4);
	footLayout->setSpacing(8);
	auto *sizeLabel = new QLabel("Tamanho", footer);
	sizeLabel->setObjectName("dica");
	sizeSlider = new QSlider(Qt::Horizontal, footer);
	sizeSlider->setRange(120, 420);
	sizeSlider->setValue(200);
	sizeSlider->setFixedWidth(110);
	auto *hint = new QLabel(QString::fromUtf8("Clique: prévia · Duplo clique: no ar · Teclas 1–9 · Espaço: transição"),
				footer);
	hint->setObjectName("dica");
	hint->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	footLayout->addWidget(sizeLabel);
	footLayout->addWidget(sizeSlider);
	footLayout->addSpacing(12);
	footLayout->addWidget(hint, 1);
	root->addWidget(footer);

	/* ---- Conexões ---- */
	connect(studioButton, &QPushButton::clicked, this, &PainelDock::onStudioToggled);
	connect(transitionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&PainelDock::onTransitionSelected);
	connect(cutButton, &QPushButton::clicked, this, &PainelDock::doCut);
	connect(transitionButton, &QPushButton::clicked, this, &PainelDock::doTransition);
	connect(recordButton, &QPushButton::clicked, this, &PainelDock::onRecordClicked);
	connect(streamButton, &QPushButton::clicked, this, &PainelDock::onStreamClicked);
	connect(sizeSlider, &QSlider::valueChanged, this, [this]() {
		lastColumns = -1;
		relayout();
	});

	confirmTimer.setSingleShot(true);
	connect(&confirmTimer, &QTimer::timeout, this, [this]() {
		if (confirming)
			resetConfirm(confirming);
		refreshOutputs();
	});

	timeTimer.setInterval(1000);
	connect(&timeTimer, &QTimer::timeout, this, &PainelDock::updateTimes);
	timeTimer.start();

	obs_frontend_add_event_callback(frontendEvent, this);
	signal_handler_connect(obs_get_signal_handler(), "source_rename", sourceRenamed, this);
}

PainelDock::~PainelDock()
{
	signal_handler_disconnect(obs_get_signal_handler(), "source_rename", sourceRenamed, this);
	obs_frontend_remove_event_callback(frontendEvent, this);
	clearScenes();
}

/* ---------------- Eventos do OBS ---------------- */

void PainelDock::frontendEvent(enum obs_frontend_event event, void *data)
{
	auto *self = static_cast<PainelDock *>(data);

	switch (event) {
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		self->loaded = true;
		QMetaObject::invokeMethod(self, "rebuildScenes", Qt::QueuedConnection);
		QMetaObject::invokeMethod(self, "refreshTransitions", Qt::QueuedConnection);
		QMetaObject::invokeMethod(self, "refreshOutputs", Qt::QueuedConnection);
		break;

	/* Precisa ser imediato: o OBS vai apagar as cenas logo em seguida */
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
	case OBS_FRONTEND_EVENT_SCRIPTING_SHUTDOWN:
	case OBS_FRONTEND_EVENT_EXIT:
		self->clearScenes();
		if (event == OBS_FRONTEND_EVENT_EXIT)
			self->loaded = false;
		break;

	case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		QMetaObject::invokeMethod(self, "rebuildScenes", Qt::QueuedConnection);
		QMetaObject::invokeMethod(self, "refreshTransitions", Qt::QueuedConnection);
		break;

	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
	case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
	case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
		QMetaObject::invokeMethod(self, "updateStates", Qt::QueuedConnection);
		break;

	case OBS_FRONTEND_EVENT_TRANSITION_CHANGED:
	case OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED:
		QMetaObject::invokeMethod(self, "refreshTransitions", Qt::QueuedConnection);
		break;

	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		QMetaObject::invokeMethod(self, "refreshOutputs", Qt::QueuedConnection);
		break;

	default:
		break;
	}
}

void PainelDock::sourceRenamed(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<PainelDock *>(data), "refreshNames", Qt::QueuedConnection);
}

/* ---------------- Cenas ---------------- */

void PainelDock::clearScenes()
{
	for (SceneTile *tile : tiles) {
		tile->release();
		grid->removeWidget(tile);
		tile->hide();
		tile->deleteLater();
	}
	tiles.clear();
	lastColumns = -1;
}

void PainelDock::rebuildScenes()
{
	if (!loaded)
		return;

	clearScenes();

	obs_frontend_source_list list = {};
	obs_frontend_get_scenes(&list);
	for (size_t i = 0; i < list.sources.num; i++) {
		auto *tile = new SceneTile(list.sources.array[i], (int)i + 1, gridHost);
		connect(tile, &SceneTile::clicked, this, &PainelDock::onTileClicked);
		connect(tile, &SceneTile::doubleClicked, this, &PainelDock::onTileDoubleClicked);
		tiles.push_back(tile);
	}
	obs_frontend_source_list_free(&list);

	relayout();
	updateStates();
}

void PainelDock::refreshNames()
{
	for (SceneTile *tile : tiles)
		tile->refreshName();
}

void PainelDock::updateStates()
{
	const bool studio = obs_frontend_preview_program_mode_active();
	OBSSourceAutoRelease program = obs_frontend_get_current_scene();
	OBSSourceAutoRelease preview = studio ? obs_frontend_get_current_preview_scene() : nullptr;

	for (SceneTile *tile : tiles) {
		if (tile->scene() && tile->scene() == program.Get())
			tile->setState(SceneTile::State::Program);
		else if (tile->scene() && tile->scene() == preview.Get())
			tile->setState(SceneTile::State::Preview);
		else
			tile->setState(SceneTile::State::Normal);
	}

	studioButton->setChecked(studio);
	transitionButton->setEnabled(studio);
	cutButton->setEnabled(studio);
}

void PainelDock::relayout()
{
	if (!grid)
		return;

	const int available = scroll->viewport()->width() - kGap * 2;
	if (available <= 0)
		return;

	const int minWidth = sizeSlider->value();
	const int columns = std::max(1, (available + kGap) / (minWidth + kGap));
	const int tileWidth = (available - kGap * (columns - 1)) / columns;

	obs_video_info ovi = {};
	double aspect = 16.0 / 9.0;
	if (obs_get_video_info(&ovi) && ovi.base_width && ovi.base_height)
		aspect = (double)ovi.base_width / (double)ovi.base_height;
	const int displayHeight = (int)((tileWidth - 6) / aspect);

	if (columns != lastColumns) {
		for (SceneTile *tile : tiles)
			grid->removeWidget(tile);
		for (int c = 0; c < std::max(columns, lastColumns); c++)
			grid->setColumnStretch(c, c < columns ? 1 : 0);
		for (int i = 0; i < tiles.size(); i++) {
			grid->addWidget(tiles[i], i / columns, i % columns);
			tiles[i]->setNumber(i + 1);
			tiles[i]->show();
		}
		lastColumns = columns;
	}

	for (SceneTile *tile : tiles)
		tile->setDisplayHeight(displayHeight);
}

void PainelDock::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	relayout();
}

bool PainelDock::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == scroll->viewport() && event->type() == QEvent::Resize)
		QMetaObject::invokeMethod(this, "relayout", Qt::QueuedConnection);
	return QWidget::eventFilter(obj, event);
}

/* ---------------- Ações ---------------- */

void PainelDock::selectScene(obs_source_t *scene)
{
	if (!scene)
		return;
	if (obs_frontend_preview_program_mode_active())
		obs_frontend_set_current_preview_scene(scene);
	else
		obs_frontend_set_current_scene(scene);
}

void PainelDock::goLive(obs_source_t *scene)
{
	if (!scene)
		return;
	if (obs_frontend_preview_program_mode_active()) {
		obs_frontend_set_current_preview_scene(scene);
		obs_frontend_preview_program_trigger_transition();
	} else {
		obs_frontend_set_current_scene(scene);
	}
}

void PainelDock::onTileClicked(SceneTile *tile)
{
	setFocus(Qt::MouseFocusReason);
	selectScene(tile->scene());
}

void PainelDock::onTileDoubleClicked(SceneTile *tile)
{
	setFocus(Qt::MouseFocusReason);
	goLive(tile->scene());
}

void PainelDock::onStudioToggled()
{
	obs_frontend_set_preview_program_mode(!obs_frontend_preview_program_mode_active());
	QMetaObject::invokeMethod(this, "updateStates", Qt::QueuedConnection);
}

void PainelDock::doTransition()
{
	if (obs_frontend_preview_program_mode_active())
		obs_frontend_preview_program_trigger_transition();
}

void PainelDock::doCut()
{
	if (!obs_frontend_preview_program_mode_active())
		return;

	OBSSourceAutoRelease current = obs_frontend_get_current_transition();
	OBSSourceAutoRelease cut;

	obs_frontend_source_list list = {};
	obs_frontend_get_transitions(&list);
	for (size_t i = 0; i < list.sources.num; i++) {
		obs_source_t *t = list.sources.array[i];
		if (strcmp(obs_source_get_unversioned_id(t), "cut_transition") == 0) {
			cut = obs_source_get_ref(t);
			break;
		}
	}
	obs_frontend_source_list_free(&list);

	if (!cut || cut.Get() == current.Get()) {
		obs_frontend_preview_program_trigger_transition();
		return;
	}

	/* Guarda uma referência fraca da transição atual para voltar a ela depois do corte */
	obs_weak_source_t *keep = current ? obs_source_get_weak_source(current) : nullptr;
	obs_frontend_set_current_transition(cut);
	obs_frontend_preview_program_trigger_transition();

	if (!keep)
		return;
	QTimer::singleShot(150, this, [keep]() {
		OBSSourceAutoRelease prev = obs_weak_source_get_source(keep);
		if (prev)
			obs_frontend_set_current_transition(prev);
		obs_weak_source_release(keep);
	});
}

void PainelDock::onTransitionSelected(int index)
{
	if (updatingTransitions || index < 0)
		return;

	const QByteArray wanted = transitionCombo->itemText(index).toUtf8();
	obs_frontend_source_list list = {};
	obs_frontend_get_transitions(&list);
	for (size_t i = 0; i < list.sources.num; i++) {
		obs_source_t *t = list.sources.array[i];
		if (wanted == obs_source_get_name(t)) {
			obs_frontend_set_current_transition(t);
			break;
		}
	}
	obs_frontend_source_list_free(&list);
}

void PainelDock::refreshTransitions()
{
	updatingTransitions = true;
	transitionCombo->clear();

	OBSSourceAutoRelease current = obs_frontend_get_current_transition();
	obs_frontend_source_list list = {};
	obs_frontend_get_transitions(&list);
	int currentIndex = -1;
	for (size_t i = 0; i < list.sources.num; i++) {
		obs_source_t *t = list.sources.array[i];
		transitionCombo->addItem(QString::fromUtf8(obs_source_get_name(t)));
		if (t == current.Get())
			currentIndex = (int)i;
	}
	obs_frontend_source_list_free(&list);

	if (currentIndex >= 0)
		transitionCombo->setCurrentIndex(currentIndex);
	updatingTransitions = false;
}

/* ---------------- Gravar / Transmitir ---------------- */

bool PainelDock::confirmSecondClick(QPushButton *button, const QString &question)
{
	if (confirming == button) {
		confirmTimer.stop();
		resetConfirm(button);
		return true;
	}
	if (confirming)
		resetConfirm(confirming);
	confirming = button;
	button->setText(question);
	button->setProperty("confirmar", true);
	repolish(button);
	confirmTimer.start(3000);
	return false;
}

void PainelDock::resetConfirm(QPushButton *button)
{
	button->setProperty("confirmar", false);
	repolish(button);
	if (confirming == button)
		confirming = nullptr;
}

void PainelDock::onRecordClicked()
{
	if (obs_frontend_recording_active()) {
		if (confirmSecondClick(recordButton, "Parar? Clique de novo"))
			obs_frontend_recording_stop();
	} else {
		obs_frontend_recording_start();
	}
	refreshOutputs();
}

void PainelDock::onStreamClicked()
{
	const bool active = obs_frontend_streaming_active();
	if (!confirmSecondClick(streamButton, active ? "Encerrar? Clique de novo" : "Ir ao ar? Clique de novo"))
		return;
	if (active)
		obs_frontend_streaming_stop();
	else
		obs_frontend_streaming_start();
	refreshOutputs();
}

QString PainelDock::formatTime(obs_output_t *output)
{
	if (!output)
		return QString();
	obs_video_info ovi = {};
	if (!obs_get_video_info(&ovi) || !ovi.fps_num)
		return QString();
	const uint64_t frames = (uint64_t)obs_output_get_total_frames(output);
	const uint64_t secs = frames * ovi.fps_den / ovi.fps_num;
	return QString("%1:%2:%3")
		.arg(secs / 3600, 2, 10, QChar('0'))
		.arg((secs / 60) % 60, 2, 10, QChar('0'))
		.arg(secs % 60, 2, 10, QChar('0'));
}

void PainelDock::refreshOutputs()
{
	const bool rec = obs_frontend_recording_active();
	const bool live = obs_frontend_streaming_active();

	if (confirming != recordButton) {
		recordButton->setProperty("ativo", rec);
		repolish(recordButton);
	}
	if (confirming != streamButton) {
		streamButton->setProperty("ativo", live);
		repolish(streamButton);
	}
	updateTimes();
}

void PainelDock::updateTimes()
{
	const bool rec = obs_frontend_recording_active();
	const bool live = obs_frontend_streaming_active();

	if (confirming != recordButton) {
		QString text = QString::fromUtf8(rec ? "● Gravando" : "● Gravar");
		if (rec) {
			OBSOutputAutoRelease out = obs_frontend_get_recording_output();
			text += "  " + formatTime(out);
		}
		recordButton->setText(text);
	}
	if (confirming != streamButton) {
		QString text = live ? QString::fromUtf8("● No ar") : QString("Transmitir");
		if (live) {
			OBSOutputAutoRelease out = obs_frontend_get_streaming_output();
			text += "  " + formatTime(out);
		}
		streamButton->setText(text);
	}
}

/* ---------------- Teclado ---------------- */

void PainelDock::keyPressEvent(QKeyEvent *event)
{
	const int key = event->key();
	if (key == Qt::Key_Space || key == Qt::Key_Return || key == Qt::Key_Enter) {
		doTransition();
		event->accept();
		return;
	}
	if (key >= Qt::Key_1 && key <= Qt::Key_9) {
		const int index = key - Qt::Key_1;
		if (index < tiles.size()) {
			selectScene(tiles[index]->scene());
			event->accept();
			return;
		}
	}
	QWidget::keyPressEvent(event);
}
