/***************************************************************************
 *  This file is part of Saaghar, a Persian poetry software                *
 *  It's based on "qmusicplayer" example of Qt ToolKit                     *
 *                                                                         *
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).       *
 * All rights reserved.                                                    *
 * Contact: Nokia Corporation (qt-info@nokia.com)                          *
 *                                                                         *
 * This file is part of the examples of the Qt Toolkit.                    *
 *                                                                         *
 *  Copyright (C) 2010-2012 by S. Razi Alavizadeh                          *
 *  E-Mail: <s.r.alavizadeh@gmail.com>, WWW: <http://pozh.org>             *
 *                                                                         *
 * $QT_BEGIN_LICENSE:BSD$                                                  *
 * You may use this file under the terms of the BSD license as follows:    *
 *                                                                         *
 * "Redistribution and use in source and binary forms, with or without     *
 * modification, are permitted provided that the following conditions are  *
 * met:                                                                    *
 *   * Redistributions of source code must retain the above copyright      *
 *     notice, this list of conditions and the following disclaimer.       *
 *   * Redistributions in binary form must reproduce the above copyright   *
 *     notice, this list of conditions and the following disclaimer in     *
 *     the documentation and/or other materials provided with the          *
 *     distribution.                                                       *
 *   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor  *
 *     the names of its contributors may be used to endorse or promote     *
 *     products derived from this software without specific prior written  *
 *     permission.                                                         *
 *                                                                         *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT       *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   *
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT    *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."   *
 * $QT_END_LICENSE$                                                        *
 *                                                                         *
 ***************************************************************************/

#include "qmusicplayer.h"

#include <QDesktopServices>
#include <QLCDNumber>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QAction>
#include <QStyle>
#include <QLabel>
#include <QVBoxLayout>
#include <QTime>
#include <QSettings>
#include <QToolButton>
#include <QMenu>
#include <QDockWidget>

QHash<QString, QVariant> QMusicPlayer::listOfPlayList = QHash<QString, QVariant>();
//QHash<int, QMusicPlayer::SaagharMediaTag> QMusicPlayer::d efaultPlayList = QHash<int, QMusicPlayer::SaagharMediaTag>();
QMusicPlayer::SaagharPlayList QMusicPlayer::hashDefaultPlayList = QMusicPlayer::SaagharPlayList();
//QHash<QString, QHash<int, QMusicPlayer::SaagharMediaTag> > QMusicPlayer::playLists = QHash<QString, QHash<int, QMusicPlayer::SaagharMediaTag> >();
QHash<QString, QMusicPlayer::SaagharPlayList *> QMusicPlayer::hashPlayLists = QHash<QString, QMusicPlayer::SaagharPlayList *>();
//QString("default"), QHash<int, QMusicPlayer::SaagharMediaTag>()
//![0]
QMusicPlayer::QMusicPlayer(QWidget *parent) : QToolBar(parent)
{
	setObjectName("QMusicPlayer");

	setStyleSheet("background-image:url(\":/resources/images/transp.png\"); border:none;");

	notLoaded = true;
	dockList = 0;

	playListManager = new PlayListManager;
	connect(playListManager, SIGNAL(mediaPlayRequested(int/*,const QString &,const QString &*/)), this, SLOT(playMedia(int/*,const QString &,const QString &*/)));
	connect(this, SIGNAL(mediaChanged(const QString &,const QString &,int,bool)), playListManager, SLOT(currentMediaChanged(const QString &,const QString &,int,bool)));
	
	QMusicPlayer::hashDefaultPlayList.PATH = "";

	_newTime = -1;

	startDir = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
	audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
	QList<Phonon::AudioOutputDevice> audioOutputDevices = Phonon::BackendCapabilities::availableAudioOutputDevices();
	foreach (const Phonon::AudioOutputDevice newAudioOutput, audioOutputDevices)
	{
		if (newAudioOutput.name().contains("waveout", Qt::CaseInsensitive))
		{
			audioOutput->setOutputDevice(newAudioOutput);
			break;
		}
	}
	qDebug() << "audioOutput=="<<audioOutputDevices<<"output="<<audioOutput->outputDevice().name();
	mediaObject = new Phonon::MediaObject(this);
	metaInformationResolver = new Phonon::MediaObject(this);

	mediaObject->setTickInterval(1000);
//![0]
//![2]
	playListManager->setMediaObject(mediaObject);

	connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));
	connect(mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
			this, SLOT(stateChanged(Phonon::State,Phonon::State)));
	connect(metaInformationResolver, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
			this, SLOT(metaStateChanged(Phonon::State,Phonon::State)));
	connect(mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
			this, SLOT(sourceChanged(Phonon::MediaSource)));
	//connect(mediaObject, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));
	connect(mediaObject, SIGNAL(finished()), this, SLOT(aboutToFinish()));

	qDebug()<<"connect="<< connect(mediaObject, SIGNAL(seekableChanged(bool)), this, SLOT(seekableChanged(bool)));
//![2]

//![1]
	Phonon::createPath(mediaObject, audioOutput);
//![1]

	setupActions();
//	setupMenus();
	setupUi();
	timeLcd->display("00:00"); 
}

//![6]
void QMusicPlayer::seekableChanged(bool seekable)
{
	qDebug() << "seekableChanged"<<seekable<<"_newTime="<<_newTime;
	if (_newTime > 0 && seekable)
	{
		mediaObject->seek(_newTime);
		_newTime = -1;
	}
}

qint64 QMusicPlayer::currentTime()
{
	return mediaObject->currentTime();
}

void QMusicPlayer::setCurrentTime(qint64 time)
{
	_newTime = time;
	/*
	Phonon::State currentState = mediaObject->state();
	qDebug() <<"isSeakable="<<mediaObject->isSeekable()<< "setCTime-state="<<currentState<<"time="<<mediaObject->currentTime()<<"newTime="<<time;
	//stateChanged(Phonon::PausedState, Phonon::PausedState);
	mediaObject->play();
	mediaObject->seek(time);
	qDebug() <<"isSeakable="<<mediaObject->isSeekable() << "setCTime-state="<<currentState<<"time="<<mediaObject->currentTime()<<"newTime="<<time;*/
}

QString QMusicPlayer::source()
{
	return metaInformationResolver->currentSource().fileName();
}

void QMusicPlayer::setSource(const QString &fileName, const QString &title, int mediaID)
{
	_newTime = -1;
	Phonon::MediaSource source(fileName);
	sources.clear();
	infoLabel->setText("");

	if (!fileName.isEmpty())
	{
		notLoaded = false;
		sources.append(source);
		metaInformationResolver->setCurrentSource(source);
		load(0);
	}
	else
	{
		notLoaded = true;
		mediaObject->clear();
		metaInformationResolver->clear();
		qDebug() << "DISABLE";
	}

////	if (!sources.isEmpty())
////	{

////	}
////	else
//	if (fileName.isEmpty())
//	{
//		stopAction->setEnabled(false);
//		togglePlayPauseAction->setEnabled(false);
//		//pauseAction->setEnabled(false);
//		togglePlayPauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
//		togglePlayPauseAction->setText(tr("Play"));
//		disconnect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
//		disconnect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()));
//		timeLcd->display("00:00");
//	}

	if (!title.isEmpty())
		currentTitle = title;
	currentID = mediaID;
	emit mediaChanged(fileName, currentTitle, currentID, false);
}

void QMusicPlayer::setSource()
{
	qDebug() <<"setSource="<<currentID<<"\nAvailable Mime Types=\n"<<Phonon::BackendCapabilities::availableMimeTypes()/*.join("#!#")*/<<"\n=====================";

	QString file = QFileDialog::getOpenFileName(this, tr("Select Music Files"), startDir,
					"Common Supported Files ("+QMusicPlayer::commonSupportedMedia().join(" ") +
					");;Audio Files ("+QMusicPlayer::commonSupportedMedia("audio").join(" ") +
					");;Video Files (Audio Stream) ("+QMusicPlayer::commonSupportedMedia("video").join(" ") +
					");;All Files (*.*)");

	QFileInfo selectedFile(file);
	startDir = selectedFile.path();

	if (file.isEmpty())
		return;

//	int index = sources.size();
//	foreach (QString string, files) {
//			Phonon::MediaSource source(string);

//		sources.append(source);
//	}

	setSource(file, currentTitle, currentID);

//	Phonon::MediaSource source(file);
//	sources.clear();
//	sources.append(source);

//	if (!sources.isEmpty())
//	{
//		setSource(file);
//		//metaInformationResolver->setCurrentSource(sources.at(0/*index*/));
//		//load(0);
//	}

}

void QMusicPlayer::removeSource()
{
	_newTime = -1;
	sources.clear();
	infoLabel->setText("");
	notLoaded = true;
	mediaObject->clear();
	metaInformationResolver->clear();
	qDebug() << "removeSource->DISABLE";

	emit mediaChanged("", "", currentID, true);

	QMusicPlayer::removeFromPlayList(currentID);
}

//![6]

//void QMusicPlayer::about()
//{
//	QMessageBox::information(this, tr("About Music Player"),
//		tr("The Music Player example shows how to use Phonon - the multimedia"
//		   " framework that comes with Qt - to create a simple music player."));
//}

//![9]
void QMusicPlayer::stateChanged(Phonon::State newState, Phonon::State /* oldState */)
{
	switch (newState) {
		case Phonon::ErrorState:
			emit mediaChanged("", currentTitle, currentID, false);
		//errors are handled in metaStateChanged()
//			if (mediaObject->errorType() == Phonon::FatalError) {
//				QMessageBox::warning(this, tr("Fatal Error"),
//				mediaObject->errorString());
//			} else {
//				QMessageBox::warning(this, tr("Error"),
//				mediaObject->errorString());
//			}
			break;
//![9]
//![10]
		case Phonon::PlayingState:
				//togglePlayPauseAction->setEnabled(false);
				//pauseAction->setEnabled(true);
				stopAction->setEnabled(true);
				togglePlayPauseAction->setEnabled(true);
				togglePlayPauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
				togglePlayPauseAction->setText(tr("Pause"));
				disconnect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
				connect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
				break;
		case Phonon::StoppedState:
				stopAction->setEnabled(false);
				togglePlayPauseAction->setEnabled(true);
				//pauseAction->setEnabled(false);
				togglePlayPauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
				togglePlayPauseAction->setText(tr("Play"));
				connect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
				disconnect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
				timeLcd->display("00:00");
				break;
		case Phonon::PausedState:
				//pauseAction->setEnabled(false);
				stopAction->setEnabled(true);
				togglePlayPauseAction->setEnabled(true);
				togglePlayPauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
				togglePlayPauseAction->setText(tr("Play"));
				connect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
				disconnect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
				break;
//![10]
		case Phonon::BufferingState:
				break;
		default:
			;
	}

	if (notLoaded)
	{
		infoLabel->setText("");
		stopAction->setEnabled(false);
		togglePlayPauseAction->setEnabled(false);
		togglePlayPauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
		togglePlayPauseAction->setText(tr("Play"));
		disconnect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
		disconnect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
		timeLcd->display("00:00");
	}

	if (mediaObject->currentSource().type() == Phonon::MediaSource::Invalid)
	{
		stopAction->setEnabled(false);
		togglePlayPauseAction->setEnabled(false);
		togglePlayPauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
		togglePlayPauseAction->setText(tr("Play"));
	}
}

//![11]
void QMusicPlayer::tick(qint64 time)
{
	QTime displayTime(0, (time / 60000) % 60, (time / 1000) % 60);

	timeLcd->display(displayTime.toString("mm:ss"));
}
//![11]

//![12]
void QMusicPlayer::load(int index)
{
	bool wasPlaying = mediaObject->state() == Phonon::PlayingState;

	mediaObject->stop();
	mediaObject->clearQueue();

	if (index >= sources.size())
		return;

	mediaObject->setCurrentSource(sources.at(index));

	if (wasPlaying)
		mediaObject->play();
	else
		mediaObject->stop();
}
//![12]

//![13]
void QMusicPlayer::sourceChanged(const Phonon::MediaSource &source)
{
	//musicTable->selectRow(sources.indexOf(source));
	timeLcd->display("00:00");
}
//![13]

//![14]
void QMusicPlayer::metaStateChanged(Phonon::State newState, Phonon::State /* oldState */)
{
	qDebug() << "metaStateChanged";
	if (newState == Phonon::ErrorState)
	{
		//qWarning() << "Error opening files: " << metaInformationResolver->errorString();
		infoLabel->setText("Error opening files: "+metaInformationResolver->errorString());
		while (!sources.isEmpty() &&
			!(sources.takeLast() == metaInformationResolver->currentSource())
		)
		{}  /* loop */;
		return;
	}

	if (newState != Phonon::StoppedState && newState != Phonon::PausedState)
		return;

	if (metaInformationResolver->currentSource().type() == Phonon::MediaSource::Invalid)
			return;

	QMap<QString, QString> metaData = metaInformationResolver->metaData();

	QString title = metaData.value("TITLE");
	if (title == "")
		title = metaInformationResolver->currentSource().fileName();
	//qDebug() << "metaData-keys="<<QStringList(metaData.keys()).join("!#!");
	//qDebug() <<"---"<<metaInformationResolver->metaData(Phonon::DateMetaData).join("{}");
	//qDebug() << "metaData-values="<<QStringList(metaData.values()).join("!#!");
	QStringList metaInfos;
	metaInfos << title << metaData.value("ARTIST") << metaData.value("ALBUM") << metaData.value("DATE");
	metaInfos.removeDuplicates();
	QString tmp = metaInfos.join("-");
	metaInfos = tmp.split("-", QString::SkipEmptyParts);
	infoLabel->setText(metaInfos.join(" - "));

//	QTableWidgetItem *titleItem = new QTableWidgetItem(title);
//	titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);
//	QTableWidgetItem *artistItem = new QTableWidgetItem(metaData.value("ARTIST"));
//	artistItem->setFlags(artistItem->flags() ^ Qt::ItemIsEditable);
//	QTableWidgetItem *albumItem = new QTableWidgetItem(metaData.value("ALBUM"));
//	albumItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);
//	QTableWidgetItem *yearItem = new QTableWidgetItem(metaData.value("DATE"));
//	yearItem->setFlags(yearItem->flags() ^ Qt::ItemIsEditable);
//![14]

//	int currentRow = musicTable->rowCount();
//	musicTable->insertRow(currentRow);
//	musicTable->setItem(currentRow, 0, titleItem);
//	musicTable->setItem(currentRow, 1, artistItem);
//	musicTable->setItem(currentRow, 2, albumItem);
//	musicTable->setItem(currentRow, 3, yearItem);

////![15]
//	if (musicTable->selectedItems().isEmpty()) {
//		musicTable->selectRow(0);
//		mediaObject->setCurrentSource(metaInformationResolver->currentSource());
//	}

	Phonon::MediaSource source = metaInformationResolver->currentSource();
	int index = sources.indexOf(metaInformationResolver->currentSource()) + 1;
	if (sources.size() > index) {
		metaInformationResolver->setCurrentSource(sources.at(index));
	}
//	else {
//		musicTable->resizeColumnsToContents();
//		if (musicTable->columnWidth(0) > 300)
//			musicTable->setColumnWidth(0, 300);
//	}

	if (notLoaded)
	{
		infoLabel->setText("");
		stopAction->setEnabled(false);
		togglePlayPauseAction->setEnabled(false);
		togglePlayPauseAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
		togglePlayPauseAction->setText(tr("Play"));
		disconnect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
		disconnect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
		timeLcd->display("00:00");
	}
}
//![15]

//![16]
void QMusicPlayer::aboutToFinish()
{
	mediaObject->stop();
//	int index = sources.indexOf(mediaObject->currentSource()) + 1;
//	if (sources.size() > index) {
//		mediaObject->enqueue(sources.at(index));
//	}
}
//![16]

void QMusicPlayer::setupActions()
{
	togglePlayPauseAction = new QAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("Play"), this);
	togglePlayPauseAction->setShortcut(tr("Ctrl+P"));
	togglePlayPauseAction->setDisabled(true);
//	pauseAction = new QAction(style()->standardIcon(QStyle::SP_MediaPause), tr("Pause"), this);
//	pauseAction->setShortcut(tr("Ctrl+A"));
//	pauseAction->setDisabled(true);
	stopAction = new QAction(style()->standardIcon(QStyle::SP_MediaStop), tr("Stop"), this);
	stopAction->setShortcut(tr("Ctrl+S"));
	stopAction->setDisabled(true);
	nextAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipForward), tr("Next"), this);
	nextAction->setShortcut(tr("Ctrl+N"));
	previousAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipBackward), tr("Previous"), this);
	previousAction->setShortcut(tr("Ctrl+R"));
	setSourceAction = new QAction(tr("&Set Audio..."), this);
	removeSourceAction = new QAction(tr("&Remove Audio"), this);

	//setSourceAction->setShortcut(tr("Ctrl+F"));
//	exitAction = new QAction(tr("E&xit"), this);
//	exitAction->setShortcuts(QKeySequence::Quit);
//	aboutAction = new QAction(tr("A&bout"), this);
//	aboutAction->setShortcut(tr("Ctrl+B"));
//	aboutQtAction = new QAction(tr("About &Qt"), this);
//	aboutQtAction->setShortcut(tr("Ctrl+Q"));

//![5]
	connect(togglePlayPauseAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
	//connect(pauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
	connect(stopAction, SIGNAL(triggered()), mediaObject, SLOT(stop()));
//![5]
	connect(setSourceAction, SIGNAL(triggered()), this, SLOT(setSource()));
	connect(removeSourceAction, SIGNAL(triggered()), this, SLOT(removeSource()));
	
//	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
//	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
//	connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

//void QMusicPlayer::setupMenus()
//{
//	QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
//	fileMenu->addAction(setSourceAction);
//	fileMenu->addSeparator();
//	fileMenu->addAction(exitAction);

//	QMenu *aboutMenu = menuBar()->addMenu(tr("&Help"));
//	aboutMenu->addAction(aboutAction);
//	aboutMenu->addAction(aboutQtAction);
//}

//![3]
void QMusicPlayer::setupUi()
{//return;
//![3]
//	QToolBar *bar = new QToolBar(this);

//	bar->addAction(togglePlayPauseAction);
//	bar->addAction(pauseAction);
//	bar->addAction(stopAction);

	QToolButton *options = new QToolButton;
	options->setIcon(QIcon(":/images/options.png"));
	options->setPopupMode(QToolButton::InstantPopup);
	options->setStyleSheet("QToolButton::menu-indicator{image: none;}");
	QMenu *menu = new QMenu;
	menu->addAction(setSourceAction);
	menu->addAction(removeSourceAction);
	menu->addSeparator();
	if (playListManagerDock())
		menu->addAction(playListManagerDock()->toggleViewAction());
	options->setMenu(menu);

	addWidget(options);
	addSeparator();
	addAction(togglePlayPauseAction);
	//addAction(pauseAction);
	addAction(stopAction);
	addSeparator();

//![4]
	seekSlider = new Phonon::SeekSlider(this);
	seekSlider->setMediaObject(mediaObject);

//	infoLabel = new QLabel("");
//	infoLabel->setScaledContents(true);
//QLabel *tmpLabel = new QLabel("");
//tmpLabel->setScaledContents(true);
//tmpLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	//QHBoxLayout *hbl = new QHBoxLayout(tmpLabel);
	infoLabel = new ScrollText(this);
	//infoLabel->setSeparator("---");
	//infoLabel->setMinimumWidth(50);
	infoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	infoLabel->resize(seekSlider->width(), infoLabel->height());

	//hbl->addWidget(infoLabel);
	//tmpLabel->setLayout(hbl);

	volumeSlider = new Phonon::VolumeSlider(this);
	volumeSlider->setAudioOutput(audioOutput);
	connect(audioOutput, SIGNAL(mutedChanged(bool)), this, SLOT(mutedChanged(bool)));
//![4]
	volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	//QLabel *volumeLabel = new QLabel;
	//volumeLabel->setPixmap(QPixmap("images/volume.png"));

	QPalette palette;
	palette.setBrush(QPalette::Light, Qt::darkGray);

	timeLcd = new QLCDNumber;
	timeLcd->setPalette(palette);

	//QStringList headers;
	//headers << tr("Title") << tr("Artist") << tr("Album") << tr("Year");

//	musicTable = new QTableWidget(0, 4);
//	musicTable->setFixedSize(1,1);
//	musicTable->hide();
//	musicTable->setHorizontalHeaderLabels(headers);
//	musicTable->setSelectionMode(QAbstractItemView::SingleSelection);
//	musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);
//	connect(musicTable, SIGNAL(cellPressed(int,int)),
//			this, SLOT(tableClicked(int,int)));

//	QHBoxLayout *seekerLayout = new QHBoxLayout;
//	seekerLayout->addWidget(seekSlider);
//	seekerLayout->addWidget(timeLcd);

//	QHBoxLayout *playbackLayout = new QHBoxLayout;
//	playbackLayout->addWidget(bar);
//	playbackLayout->addStretch();
//	playbackLayout->addWidget(volumeLabel);
//	playbackLayout->addWidget(volumeSlider);

	QVBoxLayout *infoLayout = new QVBoxLayout;
	infoLayout->addWidget(seekSlider);
	infoLayout->addWidget(infoLabel/*tmpLabel*/,0, 0 /* Qt::AlignCenter*/);
	infoLayout->setSpacing(0);
	infoLayout->setContentsMargins(0,0,0,0);

	QWidget *infoWidget = new QWidget;
	infoWidget->setLayout(infoLayout);

//	QHBoxLayout *mainLayout = new QHBoxLayout;
//	mainLayout->addWidget(seekSlider);
//	mainLayout->addWidget(timeLcd);
//	mainLayout->addWidget(volumeSlider);

	addWidget(infoWidget);
	addWidget(timeLcd);
	addWidget(volumeSlider);
	

	setLayoutDirection(Qt::LeftToRight);

	//addWidget(widget);
	//setCentralWidget(widget);
	//setWindowTitle("Phonon Music Player");
}

void QMusicPlayer::readPlayerSettings(QSettings *settingsObject)
{
	settingsObject->beginGroup("QMusicPlayer");
	listOfPlayList = settingsObject->value("List Of PlayList").toHash();
	if (audioOutput)
	{
		audioOutput->setMuted(settingsObject->value("muted",false).toBool());
		audioOutput->setVolume(settingsObject->value("volume",0.4).toReal());
	}
	settingsObject->endGroup();
}

void QMusicPlayer::savePlayerSettings(QSettings *settingsObject)
{
	settingsObject->beginGroup("QMusicPlayer");
	settingsObject->setValue("List Of PlayList", listOfPlayList);
	if (audioOutput)
	{
		settingsObject->setValue("muted", audioOutput->isMuted());
		settingsObject->setValue("volume", audioOutput->volume());
	}
	settingsObject->endGroup();
}
#include<QDebug>
void QMusicPlayer::loadPlayList(const QString &fileName, const QString &/*playListName*/, const QString &/*format*/)
{
	/*******************************************************************************/
	//tags for Version-0.1 of Saaghar media playlist
	//#SAAGHAR!PLAYLIST!			//start of playlist
	//#SAAGHAR!PLAYLIST!TITLE!		//title tag
	//#SAAGHAR!ITEMS!				//start of items
	//#SAAGHAR!TITLE!				//item's title tag
	//#SAAGHAR!ID!					//item's id tag
	//#SAAGHAR!RELATIVEPATH!		//item's path relative to playlist file
	//#SAAGHAR!MD5SUM!				//item's MD5SUM hash
	/*******************************************************************************/
	QFile file(fileName);
	if (!file.exists())
		return;

	if (!file.open(QFile::ReadOnly | QFile::Text))
	{
		QMessageBox::information(this->parentWidget(), tr("Read Error!"), tr("Can't load playlist!\nError: %1").arg(file.errorString()));
	}

	QString playListName;
	QTextStream out(&file);
	out.setCodec("UTF-8");
	QString line = out.readLine();
	if (!line.startsWith("#SAAGHAR!PLAYLIST!")) return;

	line = out.readLine();
	while ( !line.startsWith("#SAAGHAR!PLAYLIST!TITLE!") )
	{
		line = out.readLine();
	}
	
	playListName = line.remove("#SAAGHAR!PLAYLIST!TITLE!");

	//QHash<int, SaagharMediaTag> playList;
	SaagharPlayList *playList = new SaagharPlayList;
	playList->PATH = fileName;
	listOfPlayList.insert(playListName, fileName);

	while ( !line.startsWith("#SAAGHAR!ITEMS!") )//start of items
	{
		line = out.readLine();
	}
//		struct SaagharMediaTag {
//			int time;
//			QString TITLE;
//			QString PATH;
//			QString RELATIVE_PATH;
//			QString MD5SUM;
//		};
	//lines = out.readAll();//read all items!
	int ID;
	QString TITLE;
	QString RELATIVE_PATH;
	QString MD5SUM;
	bool ok = false;
	while (!out.atEnd())
	{
		line = out.readLine();
		if (line.startsWith("#"))
		{
			if (!line.startsWith("#SAAGHAR!"))
			{
				continue;
			}
			else
			{
				line.remove("#SAAGHAR!");
				if (line.startsWith("TITLE!"))
				{
					line.remove("TITLE!");
					TITLE = line;
				}
				else if (line.startsWith("ID!"))
				{
					line.remove("ID!");
					if (line.isEmpty()) continue;
					ID = line.toInt(&ok);
					if (!ok || ID < 0) continue;
				}
				else if (line.startsWith("RELATIVEPATH!"))
				{
					line.remove("RELATIVEPATH!");
					RELATIVE_PATH = line;
				}
				else if (line.startsWith("MD5SUM!"))
				{
					line.remove("MD5SUM!");
					MD5SUM = line;
				}
			}
		}
		else
		{
			line = line.trimmed();
			if (ID !=-1 && !line.isEmpty())//at least path and id is necessary!!
			{
				SaagharMediaTag *mediaTag = new SaagharMediaTag;
				mediaTag->time = 0;
				mediaTag->PATH = line;
				mediaTag->TITLE = TITLE;
				TITLE = "";
				mediaTag->MD5SUM = MD5SUM;
				MD5SUM = "";
				mediaTag->RELATIVE_PATH = RELATIVE_PATH;
				RELATIVE_PATH = "";
				playList->mediaItems.insert(ID, mediaTag);
				qDebug()<<"id="<<ID<<"struct="<<mediaTag->PATH<<mediaTag->TITLE<<mediaTag->MD5SUM<<mediaTag->RELATIVE_PATH;
				ID = -1;
			}
		}
	}

	file.close();
	qDebug()<<"size="<<playList->mediaItems.size()<<"name="<<playListName;
	pushPlayList(playList, playListName);

	playListManager->setPlayLists(hashPlayLists);
//PATH=(\s[^#]+)
//#SAAGHAR!TITLE!
//#SAAGHAR!ID!
//#SAAGHAR!RELATIVE!
//#SAAGHAR!MD5SUM!
//#SAAGHAR!(TITLE|ID|MD5SUM|RELATIVE)!(\s[^#]+)
}

void QMusicPlayer::savePlayList(const QString &fileName, const QString &playListName, const QString &/*format*/)
{
	SaagharPlayList	*playList = playListByName(playListName);

	if (!playList)
		return;

	QString playListFileName = fileName;
	if (playListFileName.isEmpty())
		playListFileName = playList->PATH;

	listOfPlayList.insert(playListName, playListFileName);

	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text))
	{
		QMessageBox::information(this->parentWidget(), tr("Save Error!"), tr("Can't save playlist!\nError: %1").arg(file.errorString()));
	}

	QTextStream out(&file);
	out.setCodec("UTF-8");
	QString playListContent = QString("#SAAGHAR!PLAYLIST!V%1\n#SAAGHAR!PLAYLIST!TITLE!%2\n##################\n#SAAGHAR!ITEMS!\n").arg("0.1").arg(playListName);
	//QHash<int, SaagharMediaTag>
	QHash<int, SaagharMediaTag *>::const_iterator it = playList->mediaItems.constBegin(); //playList.constBegin();
	 while (it != playList->mediaItems.constEnd()) {
		 QString itemStr = QString(
					"#SAAGHAR!TITLE!%1\n"
					"#SAAGHAR!ID!%2\n"
					"#SAAGHAR!MD5SUM!%3\n"
					"#SAAGHAR!RELATIVEPATH!%4\n"
					"%5\n")
				 .arg(it.value()->TITLE).arg(it.key()).arg(it.value()->MD5SUM).arg(it.value()->RELATIVE_PATH).arg(it.value()->PATH);
		 playListContent+=itemStr;
		++it;
	 }
	 qDebug() << "======================================";
	 qDebug() << "playListContent=\n"<<playListContent;
	 qDebug() << "======================================";
	 out << playListContent;
	 file.close();
}

/*static*/
void QMusicPlayer::removeFromPlayList(int mediaID, const QString &playListName)
{
	SaagharPlayList	*playList = QMusicPlayer::playListByName(playListName);
	if (playList)
	{
		qDebug() << "insertToPlayList mediaID removed";
		playList->mediaItems.remove(mediaID);
	}
}

//#include <QCryptographicHash>
//static
void QMusicPlayer::insertToPlayList(int mediaID, const QString &mediaPath, const QString &mediaTitle, const QString &/*mediaRelativePath*/, int mediaCurrentTime, const QString &playListName)
{
	SaagharPlayList	*playList = QMusicPlayer::playListByName(playListName);
//	if (playList && mediaPath.isEmpty()) //remove from playlist
//	{
//		qDebug() << "insertToPlayList mediaID removed";
//		playList->mediaItems.remove(mediaID);
//		return;
//	}

	QFile file(mediaPath);
	if (!file.exists() || mediaID<0) return;

	SaagharMediaTag *mediaTag = new SaagharMediaTag;
	mediaTag->TITLE = mediaTitle;
	mediaTag->PATH = mediaPath;
	//mediaTag->RELATIVE_PATH = mediaRelativePath;
	mediaTag->time = mediaCurrentTime;

/**********************************************************/
//compute media file MD5SUM, reserved for future versions
//	QFile mediaFile(mediaPath);
//	if (mediaFile.open(QFile::ReadOnly))
//		mediaTag->MD5SUM = QString(
//		QCryptographicHash::hash(mediaFile.readAll(),
//		QCryptographicHash::Md5).toHex());
/**********************************************************/

	//d efaultPlayList.insert(mediaID, mediaTag);
	//hashDefaultPlayList.mediaItems.insert(mediaID, mediaTag);

	if (!playList)
	{
		playList = new SaagharPlayList;
		playList->PATH = listOfPlayList.value(playListName).toString();
		playList->mediaItems.insert(mediaID, mediaTag);
		pushPlayList(playList, playListName);
	}
	else
	{
		playList->mediaItems.insert(mediaID, mediaTag);
		//pushPlayList(playList, playListName);//playList is a pointer we don't need to push it!
	}
}

//static
void QMusicPlayer::getFromPlayList(int mediaID, QString *mediaPath, QString *mediaTitle, QString *mediaRelativePath, int *mediaCurrentTime, const QString &playListName)
{
	if (mediaID<0 || !mediaPath || !playListContains(mediaID, playListName)) return;
	//SaagharMediaTag mediaTag = d efaultPlayList.value(mediaID);
	//SaagharPlayList	playList = playListByName(playListName);
	SaagharMediaTag *mediaTag = playListByName(playListName)->mediaItems.value(mediaID); //.value(mediaID);
	if (!mediaTag)
	{
		*mediaPath = "";
		*mediaTitle = "";
		*mediaRelativePath = "";
		*mediaCurrentTime = 0;
		return;
	}

	*mediaPath = mediaTag->PATH;
	if (mediaTitle)
		*mediaTitle = mediaTag->TITLE;
	if (mediaRelativePath)
		*mediaRelativePath = mediaTag->RELATIVE_PATH;
	if (mediaCurrentTime)
		*mediaCurrentTime = mediaTag->time;
}

//static
bool QMusicPlayer::playListContains(int mediaID, const QString &playListName)
{
	//return d efaultPlayList.contains(mediaID);
	//return hashDefaultPlayList.mediaItems.contains(mediaID);
	SaagharPlayList	*playList = playListByName(playListName);

	qDebug()<<"playListName="<<playListName << "playListContains:\nmediaID="<<mediaID<<"PlayLists="<< hashPlayLists.values() <<"IDs="<< hashPlayLists.keys();
	if (!playList)
		return false;
	qDebug() << "mediaID="<<mediaID<<"pointer="<< playList<<"*po="<<playList->mediaItems.keys();
	bool ret = playList->mediaItems.contains(mediaID);
	return ret;//playListByName(playListName).mediaItems.contains(mediaID);
}

QDockWidget *QMusicPlayer::playListManagerDock()
{
	if (!playListManager) return 0;
	if (dockList) return dockList;
	dockList = new QDockWidget;
	dockList->setObjectName("PlayListManagerDock");
	dockList->setStyleSheet("QDockWidget::title { background: transparent; text-align: left; padding: 0 10 0 10;}"
		"QDockWidget::close-button, QDockWidget::float-button { background: transparent;}");
	dockList->setWindowTitle(tr("PlayList"));
	dockList->setWidget(playListManager);
	return dockList;
}

void QMusicPlayer::playMedia(int mediaID/*, const QString &fileName, const QString &title*/)
{
	disconnect(this, SIGNAL(mediaChanged(const QString &,const QString &,int,bool)), playListManager, SLOT(currentMediaChanged(const QString &,const QString &,int,bool)));
	emit requestPageContainedMedia(mediaID, true); //(fileName, title, mediaID);
	mediaObject->play();
	connect(this, SIGNAL(mediaChanged(const QString &,const QString &,int,bool)), playListManager, SLOT(currentMediaChanged(const QString &,const QString &,int,bool)));
}

void QMusicPlayer::resizeEvent(QResizeEvent *e)
{
	//infoLabel->resize(seekSlider->width(), infoLabel->height());
//	infoLabel->setFixedWidth(50);
//	seekSlider->resize(50, seekSlider->height());
//	seekSlider->update();
//	infoLabel->update();
//	infoLabel->hide();
//	QApplication::processEvents();
	QToolBar::resizeEvent(e);
	
	infoLabel->resize(seekSlider->width(), infoLabel->height());
//	qDebug() << "after ToolBar Resize"<<Q_FUNC_INFO;
//	qDebug() << "seekSlider-w"<<seekSlider->width()<<"seekSlider-hintW="<<seekSlider->sizeHint().width();
//	qDebug() << "1infoLabel-w"<<infoLabel->width()<<"1hintW="<<infoLabel->sizeHint().width();
//	infoLabel->setFixedWidth(seekSlider->width());
//	infoLabel->show();
	//	qDebug() << "2infoLabel-w"<<infoLabel->width()<<"2hintW="<<infoLabel->sizeHint().width();
}

/*static*/
QStringList QMusicPlayer::commonSupportedMedia(const QString &type)
{
	QStringList supportedExtentions;
	const QStringList commonMediaExtentions = QStringList()
			<< "mp3" << "wav" << "wma" << "ogg" << "mp4" << "mpg" << "mid"
			<< "asf" << "3gp" << "wmv" << "avi";

	const QString sep = "|";
	QString supportedMimeTypes = Phonon::BackendCapabilities::availableMimeTypes().join(sep);
	for (int i=0; i<commonMediaExtentions.size();++i)
	{
		QString extention = commonMediaExtentions.at(i);
		if (supportedMimeTypes.contains(QRegExp( QString("(%1/[^%2]*%3[^%2]*)").arg(type).arg(sep).arg(extention) )))
		{
			if (type == "audio" && (extention == "mpg" || extention == "3gp") )
				continue;

			supportedExtentions << "*."+extention;
		}
	}
	qDebug() << "commonSupportedMedia-"<<type<<"="<<supportedExtentions;
	return supportedExtentions;
}

void QMusicPlayer::stop()
{
	if (mediaObject)
		mediaObject->stop();
	infoLabel->setText("");
}

/*******************************
// class PlayListManager
********************************/

#include <QTreeWidget>
#include <QVBoxLayout>

PlayListManager::PlayListManager(QWidget *parent) : QWidget(parent)
{
	setObjectName("PlayListManager");

	mediaList = new QTreeWidget;
	mediaList->setLayoutDirection(Qt::RightToLeft);
	mediaList->headerItem()->setHidden(true);
	previousItem = 0;
	mediaList->setColumnCount(1);
	hLayout = new QVBoxLayout;

	hLayout->addWidget(mediaList);
	this->setLayout(hLayout);
	connect(mediaList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemPlayRequested(QTreeWidgetItem*,int)));
	connect(mediaList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
}

void PlayListManager::setPlayLists(const QHash<QString, QMusicPlayer::SaagharPlayList *> &playLists, bool /*justMediaList*/)
{
	if (playLists.isEmpty()) return;
	//if (justMediaList) //not implemented!
	itemsAsTopItem = playLists.size() == 1 ? true : false;
	QHash<QString, QMusicPlayer::SaagharPlayList *>::const_iterator playListIterator = playLists.constBegin();
	while (playListIterator != playLists.constEnd())
	{
		QMusicPlayer::SaagharPlayList *playList = playListIterator.value();
		if (!playList)
		{
			++playListIterator;
			continue;
		}

		QTreeWidgetItem *playListRoot = 0;
		if (!itemsAsTopItem)
		{
			playListRoot = new QTreeWidgetItem();
			mediaList->addTopLevelItem(playListRoot);
			playListRoot->setText(0, playListIterator.key());
		}

		QHash<int, QMusicPlayer::SaagharMediaTag *> items = playList->mediaItems;
		QHash<int, QMusicPlayer::SaagharMediaTag *>::const_iterator mediaIterator = items.constBegin();
		while (mediaIterator != items.constEnd())
		{
			QMusicPlayer::SaagharMediaTag *mediaTag = mediaIterator.value();
			if (!mediaTag)
			{
				++mediaIterator;
				continue;
			}

			QTreeWidgetItem *mediaItem = new QTreeWidgetItem();
			mediaItem->setIcon(0, style()->standardIcon(QStyle::SP_MediaPause));
			mediaItem->setText(0, mediaTag->TITLE);
//			mediaItem->setText(1, mediaTag->PATH);
			mediaItem->setData(0, Qt::UserRole, mediaIterator.key());

			if (!itemsAsTopItem)
				playListRoot->addChild(mediaItem);
			else
				mediaList->addTopLevelItem(mediaItem);
			++mediaIterator;
		}
		++playListIterator;
	}
}

void PlayListManager::currentMediaChanged(const QString &fileName, const QString &title, int mediaID, bool removeRequest)
{
	if (/*fileName.isEmpty() ||*/ mediaID<=0) // fileName.isEmpty() for delete request!!
		return;

	for (int i=0; i<mediaList->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *rootItem = mediaList->topLevelItem(i);
		if (!rootItem)
			continue;
		int end = rootItem->childCount();
		if (itemsAsTopItem)
			end = 1;
		for (int j=0; j<end; ++j)
		{
			QTreeWidgetItem *childItem;
			if (itemsAsTopItem)
				childItem = rootItem;
			else
				childItem = rootItem->child(j);

			if (!childItem)
				continue;
			int childID = childItem->data(0, Qt::UserRole).toInt();
			if (childID != mediaID)
				continue;
			else
			{
				if (removeRequest)
				{
					if (previousItem == childItem)
						previousItem = 0;
					rootItem->takeChild(j);
					delete childItem;
					childItem = 0;
				}
				else if (!fileName.isEmpty())
				{
					if (!title.isEmpty())
						childItem->setText(0, title);
//					childItem->setText(1, fileName);
					if (playListMediaObject && playListMediaObject->state() == Phonon::PlayingState)
						childItem->setIcon(0, style()->standardIcon(QStyle::SP_MediaPlay));
					if (mediaList->currentItem())
					{
						mediaList->currentItem()->setIcon(0, style()->standardIcon(QStyle::SP_MediaPause));
					}
					disconnect(mediaList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
					mediaList->setCurrentItem(childItem, 0);
					previousItem = childItem;
					connect(mediaList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
				}
				return;
			}
		}
	}

	if (!fileName.isEmpty())
	{
		QTreeWidgetItem *rootItem = 0;
		if (!itemsAsTopItem)
			rootItem = mediaList->topLevelItem(0);

		if (rootItem || itemsAsTopItem)
		{
			QTreeWidgetItem *newChild = new QTreeWidgetItem;
			newChild->setText(0, title);
//			newChild->setText(1, fileName);
			newChild->setData(0, Qt::UserRole, mediaID);
			if (playListMediaObject && playListMediaObject->state() == Phonon::PlayingState)
				newChild->setIcon(0, style()->standardIcon(QStyle::SP_MediaPlay));
	
			if (rootItem)
				rootItem->addChild(newChild);
			else //itemsAsTopItem == true
				mediaList->addTopLevelItem(newChild);
	
			if (mediaList->currentItem())
			{
				mediaList->currentItem()->setIcon(0, style()->standardIcon(QStyle::SP_MediaPause));
			}
			disconnect(mediaList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
			mediaList->setCurrentItem(newChild, 0);
			previousItem = newChild;
			connect(mediaList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
		}
	}
}

void PlayListManager::itemPlayRequested(QTreeWidgetItem *item, int /*col*/)
{
	if (!item) return;

	if (item->childCount()>0)
		return;//root item

	if (previousItem)
	{
//		if (item == previousItem)
//			return;
		previousItem->setIcon(0, style()->standardIcon(QStyle::SP_MediaPause));
	}
	
	item->setIcon(0, style()->standardIcon(QStyle::SP_MediaPlay));
	int id = item->data(0, Qt::UserRole).toInt();
	previousItem = item;
	emit mediaPlayRequested(id/*, current->text(1), current->text(0)*/);
}

void PlayListManager::setMediaObject(Phonon::MediaObject *MediaObject)
{
	playListMediaObject = MediaObject;
	connect(playListMediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
			this, SLOT(mediaObjectStateChanged(Phonon::State,Phonon::State)));
}

void PlayListManager::mediaObjectStateChanged(Phonon::State newState, Phonon::State oldState)
{
	//QTreeWidgetItem *item = mediaList->; mediaList->currentIndex()
	if (previousItem)
	{
		if (newState == Phonon::PlayingState)
			previousItem->setIcon(0, style()->standardIcon(QStyle::SP_MediaPlay));
		else
			previousItem->setIcon(0, style()->standardIcon(QStyle::SP_MediaPause));
	}
}


/***********************************************************
// class ScrollText by Sebastian Lehmann <http://l3.ms>
// link: http://stackoverflow.com/a/10655396
************************************************************/
#include <QPainter>

ScrollText::ScrollText(QWidget *parent) :
	QWidget(parent), scrollPos(0)
{
#if QT_VERSION < 0x040700
	staticText.hide();
#endif

	staticText.setTextFormat(Qt::PlainText);

	setFixedHeight(fontMetrics().height());
	leftMargin = height() / 3;

	setSeparator("   ---   ");

	connect(&timer, SIGNAL(timeout()), this, SLOT(timer_timeout()));
	timer.setInterval(60);
}

QString ScrollText::text() const
{
	return _text;
}

void ScrollText::setText(QString text)
{
	_text = text;
	updateText();
	update();
}

QString ScrollText::separator() const
{
	return _separator;
}

void ScrollText::setSeparator(QString separator)
{
	_separator = separator;
	updateText();
	update();
}

void ScrollText::updateText()
{
	timer.stop();

	singleTextWidth = fontMetrics().width(_text);
	scrollEnabled = (singleTextWidth > width() - leftMargin);

	if(scrollEnabled)
	{
		scrollPos = -64;
		staticText.setText(_text + _separator);
		timer.start();
	}
	else
		staticText.setText(_text);
#if QT_VERSION >= 0x040700
	staticText.prepare(QTransform(), font());
#endif
	wholeTextSize = QSize(fontMetrics().width(staticText.text()), fontMetrics().height());
}

void ScrollText::paintEvent(QPaintEvent*)
{
	QPainter p(this);

	if(scrollEnabled)
	{
		buffer.fill(qRgba(0, 0, 0, 0));
		QPainter pb(&buffer);
		pb.setPen(p.pen());
		pb.setFont(p.font());

		int x = qMin(-scrollPos, 0) + leftMargin;
		while(x < width())
		{
#if QT_VERSION >= 0x040700
			pb.drawStaticText(QPointF(x, (height() - wholeTextSize.height()) / 2) /*+ QPoint(2, 2)*/, staticText);
#else
			pb.drawText(QPointF(x, height()-2- ( (height() - wholeTextSize.height()) / 2) ) /*+ QPoint(2, 2)*/, staticText.text());
#endif
			x += wholeTextSize.width();
		}

		//Apply Alpha Channel
		pb.setCompositionMode(QPainter::CompositionMode_DestinationIn);
		pb.setClipRect(width() - 15, 0, 15, height());
		pb.drawImage(0, 0, alphaChannel);
		pb.setClipRect(0, 0, 15, height());
		//initial situation: don't apply alpha channel in the left half of the image at all; apply it more and more until scrollPos gets positive
		if(scrollPos < 0)
			pb.setOpacity((qreal)(qMax(-8, scrollPos) + 8) / 8.0);
		pb.drawImage(0, 0, alphaChannel);

		//pb.end();
		p.drawImage(0, 0, buffer);
	}
	else
	{
#if QT_VERSION >= 0x040700
		p.drawStaticText(QPointF(leftMargin+(width()-(wholeTextSize.width()+leftMargin) )/2, (height() - wholeTextSize.height()) / 2), staticText);
#else
		p.drawText(QPointF(leftMargin+(width()-(wholeTextSize.width()+leftMargin) )/2,height()-2-( (height() - wholeTextSize.height()) / 2) ), staticText.text());
#endif
	}
}

void ScrollText::resizeEvent(QResizeEvent*)
{
	//When the widget is resized, we need to update the alpha channel.
	alphaChannel = QImage(size(), QImage::Format_ARGB32_Premultiplied);
	buffer = QImage(size(), QImage::Format_ARGB32_Premultiplied);

	//Create Alpha Channel:
	if(width() > 64)
	{
		//create first scanline
		QRgb* scanline1 = (QRgb*)alphaChannel.scanLine(0);
		for(int x = 1; x < 16; ++x)
			scanline1[x - 1] = scanline1[width() - x] = qRgba(0, 0, 0, x << 4);
		for(int x = 15; x < width() - 15; ++x)
			scanline1[x] = qRgb(0, 0, 0);
		//copy scanline to the other ones
		for(int y = 1; y < height(); ++y)
			memcpy(alphaChannel.scanLine(y), (uchar*)scanline1, width() * 4);
	}
	else
		alphaChannel.fill(qRgb(0, 0, 0));


	//Update scrolling state
	bool newScrollEnabled = (singleTextWidth > width() - leftMargin);
	if(newScrollEnabled != scrollEnabled)
		updateText();
}

void ScrollText::timer_timeout()
{
	scrollPos = (scrollPos + 2)
				% wholeTextSize.width();
	update();
}

// //for future
// //Windows 7 volume notification code
//static const GUID AudioSessionVolumeCtx =
//{ 0x2715279f, 0x4139, 0x4ba0, { 0x9c, 0xb1, 0xb3, 0x51, 0xf1, 0xb5, 0x8a, 0x4a } };


//CAudioSessionVolume::CAudioSessionVolume(
//	UINT uNotificationMessage,
//	HWND hwndNotification
//	)
//	: m_cRef(1),
//	  m_uNotificationMessage(uNotificationMessage),
//	  m_hwndNotification(hwndNotification),
//	  m_bNotificationsEnabled(FALSE),
//	  m_pAudioSession(NULL),
//	  m_pSimpleAudioVolume(NULL)
//{
//}

//CAudioSessionVolume::~CAudioSessionVolume()
//{
//	EnableNotifications(FALSE);

//	SafeRelease(&m_pAudioSession);
//	SafeRelease(&m_pSimpleAudioVolume);
//};


////  Creates an instance of the CAudioSessionVolume object.

///* static */
//HRESULT CAudioSessionVolume::CreateInstance(
//	UINT uNotificationMessage,
//	HWND hwndNotification,
//	CAudioSessionVolume **ppAudioSessionVolume
//	)
//{

//	CAudioSessionVolume *pAudioSessionVolume = new (std::nothrow)
//		CAudioSessionVolume(uNotificationMessage, hwndNotification);

//	if (pAudioSessionVolume == NULL)
//	{
//		return E_OUTOFMEMORY;
//	}

//	HRESULT hr = pAudioSessionVolume->Initialize();
//	if (SUCCEEDED(hr))
//	{
//		*ppAudioSessionVolume = pAudioSessionVolume;
//	}
//	else
//	{
//		pAudioSessionVolume->Release();
//	}

//	return hr;
//}


////  Initializes the CAudioSessionVolume object.

//HRESULT CAudioSessionVolume::Initialize()
//{
//	HRESULT hr = S_OK;

//	IMMDeviceEnumerator *pDeviceEnumerator = NULL;
//	IMMDevice *pDevice = NULL;
//	IAudioSessionManager *pAudioSessionManager = NULL;

//	// Get the enumerator for the audio endpoint devices.
//	hr = CoCreateInstance(
//		__uuidof(MMDeviceEnumerator),
//		NULL,
//		CLSCTX_INPROC_SERVER,
//		IID_PPV_ARGS(&pDeviceEnumerator)
//		);

//	if (FAILED(hr))
//	{
//		goto done;
//	}

//	// Get the default audio endpoint that the SAR will use.
//	hr = pDeviceEnumerator->GetDefaultAudioEndpoint(
//		eRender,
//		eConsole,   // The SAR uses 'eConsole' by default.
//		&pDevice
//		);

//	if (FAILED(hr))
//	{
//		goto done;
//	}

//	// Get the session manager for this device.
//	hr = pDevice->Activate(
//		__uuidof(IAudioSessionManager),
//		CLSCTX_INPROC_SERVER,
//		NULL,
//		(void**) &pAudioSessionManager
//		);

//	if (FAILED(hr))
//	{
//		goto done;
//	}

//	// Get the audio session.
//	hr = pAudioSessionManager->GetAudioSessionControl(
//		&GUID_NULL,     // Get the default audio session.
//		FALSE,          // The session is not cross-process.
//		&m_pAudioSession
//		);


//	if (FAILED(hr))
//	{
//		goto done;
//	}

//	hr = pAudioSessionManager->GetSimpleAudioVolume(
//		&GUID_NULL, 0, &m_pSimpleAudioVolume
//		);

//done:
//	SafeRelease(&pDeviceEnumerator);
//	SafeRelease(&pDevice);
//	SafeRelease(&pAudioSessionManager);
//	return hr;
//}

//STDMETHODIMP CAudioSessionVolume::QueryInterface(REFIID riid, void **ppv)
//{
//	static const QITAB qit[] =
//	{
//		QITABENT(CAudioSessionVolume, IAudioSessionEvents),
//		{ 0 },
//	};
//	return QISearch(this, qit, riid, ppv);
//}

//STDMETHODIMP_(ULONG) CAudioSessionVolume::AddRef()
//{
//	return InterlockedIncrement(&m_cRef);
//}

//STDMETHODIMP_(ULONG) CAudioSessionVolume::Release()
//{
//	LONG cRef = InterlockedDecrement( &m_cRef );
//	if (cRef == 0)
//	{
//		delete this;
//	}
//	return cRef;
//}


//// Enables or disables notifications from the audio session. For example, the
//// application is notified if the user mutes the audio through the system
//// volume-control program (Sndvol).

//HRESULT CAudioSessionVolume::EnableNotifications(BOOL bEnable)
//{
//	HRESULT hr = S_OK;

//	if (m_hwndNotification == NULL || m_pAudioSession == NULL)
//	{
//		return E_FAIL;
//	}

//	if (m_bNotificationsEnabled == bEnable)
//	{
//		// No change.
//		return S_OK;
//	}

//	if (bEnable)
//	{
//		hr = m_pAudioSession->RegisterAudioSessionNotification(this);
//	}
//	else
//	{
//		hr = m_pAudioSession->UnregisterAudioSessionNotification(this);
//	}

//	if (SUCCEEDED(hr))
//	{
//		m_bNotificationsEnabled = bEnable;
//	}

//	return hr;
//}


//// Gets the session volume level.

//HRESULT CAudioSessionVolume::GetVolume(float *pflVolume)
//{
//	if ( m_pSimpleAudioVolume == NULL)
//	{
//		return E_FAIL;
//	}
//	else
//	{
//		return m_pSimpleAudioVolume->GetMasterVolume(pflVolume);
//	}
//}

////  Sets the session volume level.
////
////  flVolume: Ranges from 0 (silent) to 1 (full volume)

//HRESULT CAudioSessionVolume::SetVolume(float flVolume)
//{
//	if (m_pSimpleAudioVolume == NULL)
//	{
//		return E_FAIL;
//	}
//	else
//	{
//		return m_pSimpleAudioVolume->SetMasterVolume(
//			flVolume,
//			&AudioSessionVolumeCtx  // Event context.
//			);
//	}
//}


////  Gets the muting state of the session.

//HRESULT CAudioSessionVolume::GetMute(BOOL *pbMute)
//{
//	if (m_pSimpleAudioVolume == NULL)
//	{
//		return E_FAIL;
//	}
//	else
//	{
//		return m_pSimpleAudioVolume->GetMute(pbMute);
//	}
//}

////  Mutes or unmutes the session audio.

//HRESULT CAudioSessionVolume::SetMute(BOOL bMute)
//{
//	if (m_pSimpleAudioVolume == NULL)
//	{
//		return E_FAIL;
//	}
//	else
//	{
//		return m_pSimpleAudioVolume->SetMute(
//			bMute,
//			&AudioSessionVolumeCtx  // Event context.
//			);
//	}
//}

////  Sets the display name for the session audio.

//HRESULT CAudioSessionVolume::SetDisplayName(const WCHAR *wszName)
//{
//	if (m_pAudioSession == NULL)
//	{
//		return E_FAIL;
//	}
//	else
//	{
//		return m_pAudioSession->SetDisplayName(wszName, NULL);
//	}
//}


////  Called when the session volume level or muting state changes.
////  (Implements IAudioSessionEvents::OnSimpleVolumeChanged.)

//HRESULT CAudioSessionVolume::OnSimpleVolumeChanged(
//	float NewVolume,
//	BOOL NewMute,
//	LPCGUID EventContext
//	)
//{
//	// Check if we should post a message to the application.

//	if ( m_bNotificationsEnabled &&
//		(*EventContext != AudioSessionVolumeCtx) &&
//		(m_hwndNotification != NULL)
//		)
//	{
//		// Notifications are enabled, AND
//		// We did not trigger the event ourselves, AND
//		// We have a valid window handle.

//		// Post the message.
//		::PostMessage(
//			m_hwndNotification,
//			m_uNotificationMessage,
//			*((WPARAM*)(&NewVolume)),  // Coerce the float.
//			(LPARAM)NewMute
//			);
//	}
//	return S_OK;
//}
