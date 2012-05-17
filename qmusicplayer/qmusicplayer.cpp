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

	dockList = 0;

	playListManager = new PlayListManager;
	connect(playListManager, SIGNAL(mediaPlayRequested(int/*,const QString &,const QString &*/)), this, SLOT(playMedia(int/*,const QString &,const QString &*/)));
	connect(this, SIGNAL(mediaChanged(const QString &,const QString &,int)), playListManager, SLOT(currentMediaChanged(const QString &,const QString &,int)));
	
	QMusicPlayer::hashDefaultPlayList.PATH = "";

	_newTime = -1;

	startDir = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
	audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
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

	//if (!fileName.isEmpty())
	{
	sources.append(source);
	}

	metaInformationResolver->setCurrentSource(source);
	load(0);

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
	emit mediaChanged(fileName, currentTitle, currentID);
}

void QMusicPlayer::setSource()
{
	QString file = QFileDialog::getOpenFileName(this, tr("Select Music Files"), startDir);

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
	setSource("", currentTitle, currentID);
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
			emit mediaChanged("", currentTitle, currentID);
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
	if (newState == Phonon::ErrorState) {
		QMessageBox::warning(this, tr("Error opening files"),
			metaInformationResolver->errorString());
		while (!sources.isEmpty() &&
			   !(sources.takeLast() == metaInformationResolver->currentSource())) {}  /* loop */;
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

	infoLabel = new QLabel("");

	volumeSlider = new Phonon::VolumeSlider(this);
	volumeSlider->setAudioOutput(audioOutput);
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
	infoLayout->addWidget(infoLabel,0, Qt::AlignCenter);
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
#include <QCryptographicHash>
//static
void QMusicPlayer::insertToPlayList(int mediaID, const QString &mediaPath, const QString &mediaTitle, const QString &/*mediaRelativePath*/, int mediaCurrentTime, const QString &playListName)
{
	QFile file(mediaPath);
	if (!file.exists() || mediaID<0) return;

	SaagharMediaTag *mediaTag = new SaagharMediaTag;
	mediaTag->TITLE = mediaTitle;
	mediaTag->PATH = mediaPath;
	//mediaTag->RELATIVE_PATH = mediaRelativePath;
	mediaTag->time = mediaCurrentTime;

	QFile mediaFile(mediaPath);
	mediaTag->MD5SUM = QString(QCryptographicHash::hash(mediaFile.readAll(), QCryptographicHash::Md5).toHex());

	//d efaultPlayList.insert(mediaID, mediaTag);
	//hashDefaultPlayList.mediaItems.insert(mediaID, mediaTag);

	SaagharPlayList	*playList = QMusicPlayer::playListByName(playListName);
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
	dockList->setWindowTitle(tr("PlayList"));
	dockList->setWidget(playListManager);
	return dockList;
}

void QMusicPlayer::playMedia(int mediaID/*, const QString &fileName, const QString &title*/)
{
	disconnect(this, SIGNAL(mediaChanged(const QString &,const QString &,int)), playListManager, SLOT(currentMediaChanged(const QString &,const QString &,int)));
	emit requestPageContainedMedia(mediaID, true); //(fileName, title, mediaID);
	mediaObject->play();
	connect(this, SIGNAL(mediaChanged(const QString &,const QString &,int)), playListManager, SLOT(currentMediaChanged(const QString &,const QString &,int)));
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
	previousItem = 0;
	mediaList->setColumnCount(2);
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
			mediaItem->setText(1, mediaTag->PATH);
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

void PlayListManager::currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
//	if (!current) return;

//	current->setIcon(0, style()->standardIcon(QStyle::SP_MediaPlay));
//	if (previous)
//	{
//		previous->setIcon(0, style()->standardIcon(QStyle::SP_MediaPause));
//	}
	
//	int id = current->data(0, Qt::UserRole).toInt();
//	emit mediaPlayRequested(id/*, current->text(1), current->text(0)*/);
}

void PlayListManager::currentMediaChanged(const QString &fileName, const QString &title, int mediaID)
{
	if (fileName.isEmpty() || mediaID<=0)
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
				if (!title.isEmpty())
					childItem->setText(0, title);
				childItem->setText(1, fileName);
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
				return;
			}
		}
	}

	QTreeWidgetItem *rootItem = 0;
	if (!itemsAsTopItem)
		rootItem = mediaList->topLevelItem(0);

	if (rootItem || itemsAsTopItem)
	{
		QTreeWidgetItem *newChild = new QTreeWidgetItem;
		newChild->setText(0, title);
		newChild->setText(1, fileName);
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

void PlayListManager::itemPlayRequested(QTreeWidgetItem *item, int /*col*/)
{
	if (!item) return;

	if (item->childCount()>0)
		return;//root item

	if (previousItem)
	{
		if (item == previousItem)
			return;
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