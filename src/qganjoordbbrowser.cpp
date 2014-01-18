/***************************************************************************
 *  This file is part of Saaghar, a Persian poetry software                *
 *                                                                         *
 *  Copyright (C) 2010-2014 by S. Razi Alavizadeh                          *
 *  E-Mail: <s.r.alavizadeh@gmail.com>, WWW: <http://pozh.org>             *
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 3 of the License,         *
 *  (at your option) any later version                                     *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details                            *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program; if not, see http://www.gnu.org/licenses/      *
 *                                                                         *
 ***************************************************************************/

#include "qganjoordbbrowser.h"
#include "nodatabasedialog.h"
#include "searchresultwidget.h"

#include <QApplication>
#include <QMessageBox>
#include <QSqlQuery>
#include <QFileInfo>
#include <QFileDialog>
#include <QTime>
#include <QPushButton>
#include <QTimer>
#include <QTreeWidgetItem>

DataBaseUpdater* QGanjoorDbBrowser::dbUpdater = 0;
QString QGanjoorDbBrowser::dBName = QString();
QStringList QGanjoorDbBrowser::dataBasePath = QStringList();

const int minNewPoetID = 1001;
const int minNewCatID = 10001;
const int minNewPoemID = 100001;

const int DatabaseVersion = 1;

#ifdef EMBEDDED_SQLITE
QSQLiteDriver* QGanjoorDbBrowser::sqlDriver = 0;
#else
const QString sqlDriver = "QSQLITE";
#endif

//#include <QStringList>
/***************************/
//from unicode table:
//  "»" cell= 187 row= 0
//  "«" cell= 171 row= 0
//  "." cell= 46 row= 0
//  "؟" cell= 31 row= 6
//  "،" cell= 12 row= 6
//  "؛" cell= 27 row= 6
const QStringList QGanjoorDbBrowser::someSymbols = QStringList()
        << QChar(31, 6) << QChar(12, 6) << QChar(27, 6)
        << QChar(187, 0) << QChar(171, 0) << QChar(46, 0)
        << ")" << "(" << "[" << "]" << ":" << "!" << "-" << "." << "?";

//the zero index of following stringlists is equipped by expected variant!
//  "ؤ" cell= 36 row= 6
//  "و" cell= 72 row= 6
const QStringList QGanjoorDbBrowser::Ve_Variant = QStringList()
        << QChar(72, 6) << QChar(36, 6);

//  "ى" cell= 73 row= 6 //ALEF MAKSURA
//  "ي" cell= 74 row= 6 //Arabic Ye
//  "ئ" cell= 38 row= 6
//  "ی" cell= 204 row= 6 //Persian Ye
const QStringList QGanjoorDbBrowser::Ye_Variant = QStringList()
        << QChar(204, 6) << QChar(38, 6) << QChar(74, 6) << QChar(73, 6);

//  "آ" cell= 34 row= 6
//  "أ" cell= 35 row= 6
//  "إ" cell= 37 row= 6
//  "ا" cell= 39 row= 6
const QStringList QGanjoorDbBrowser::AE_Variant = QStringList()
        << QChar(39, 6) << QChar(37, 6) << QChar(35, 6) << QChar(34, 6);

//  "ة" cell= 41 row= 6
//  "ۀ" cell=192 row= 6
//  "ه" cell= 71 row= 6
const QStringList QGanjoorDbBrowser::He_Variant = QStringList()
        << QChar(71, 6) << QChar(41, 6) << QChar(192, 6);
////////////////////////////
const QRegExp Ve_EXP = QRegExp("[" +
                               QGanjoorDbBrowser::Ve_Variant.join("").remove(QGanjoorDbBrowser::Ve_Variant.at(0))
                               + "]");
const QRegExp Ye_EXP = QRegExp("[" +
                               QGanjoorDbBrowser::Ye_Variant.join("").remove(QGanjoorDbBrowser::Ye_Variant.at(0))
                               + "]");
const QRegExp AE_EXP = QRegExp("[" +
                               QGanjoorDbBrowser::AE_Variant.join("").remove(QGanjoorDbBrowser::AE_Variant.at(0))
                               + "]");
const QRegExp He_EXP = QRegExp("[" +
                               QGanjoorDbBrowser::He_Variant.join("").remove(QGanjoorDbBrowser::He_Variant.at(0))
                               + "]");
const QRegExp SomeSymbol_EXP = QRegExp("[" +
                                       QGanjoorDbBrowser::someSymbols.join("")
                                       + "]+");
/***************************/

QGanjoorDbBrowser::QGanjoorDbBrowser(QString sqliteDbCompletePath, QWidget* splashScreen)
{
    bool flagSelectNewPath = false;
    QString newPath = "";

    QFileInfo dBFile(sqliteDbCompletePath);

#ifdef EMBEDDED_SQLITE
    sqlite3* connection;
    sqlite3_open(sqliteDbCompletePath.toUtf8().data(), &connection);
    sqlDriver = new QSQLiteDriver(connection);
#endif

    //dBName = getIdForDataBase(sqliteDbCompletePath);
    //dBConnection = QSqlDatabase::database(dBName, false);//don't open! we won't it created if not exists
    //dBName = dBFile.fileName();
    dBName = sqliteDbCompletePath;
    dBConnection = QSqlDatabase::addDatabase(sqlDriver, dBName);

    dBConnection.setDatabaseName(sqliteDbCompletePath);
    while (!dBFile.exists() || !dBConnection.open()) {
        if (splashScreen) {
            splashScreen->close();
            splashScreen = 0;
        }
        QString errorString = dBConnection.lastError().text();

        dBConnection = QSqlDatabase();
        qDebug() << __LINE__ << __FUNCTION__;
        QSqlDatabase::removeDatabase(dBName);

        NoDataBaseDialog noDataBaseDialog(0, Qt::WindowStaysOnTopHint);
        noDataBaseDialog.ui->pathLabel->setText(tr("Data Base Path:") + " " + sqliteDbCompletePath);
        noDataBaseDialog.ui->errorLabel->setText(tr("Error:") + " " + (errorString.isEmpty() ? tr("No Error!") : errorString));

//      QMessageBox warnDataBaseOpen(0);
//      warnDataBaseOpen.setWindowTitle(tr("Cannot open Database File!"));
//      warnDataBaseOpen.setIcon(QMessageBox::Information);
//      QAbstractButton *browseButton = 0;
//      QAbstractButton *downloadButton = 0;
//      if ( errorString.simplified().isEmpty() /*contains("Driver not loaded")*/ )
//      {
//          warnDataBaseOpen.setText(tr("Cannot open database file...\nDataBase Path=%1\nSaaghar needs its database for working properly. Press 'OK' if you want to terminate application or press 'Browse' for select a new path for database.").arg(sqliteDbCompletePath));
//          browseButton = warnDataBaseOpen.addButton(tr("Browse..."), QMessageBox::ActionRole);
//          downloadButton = warnDataBaseOpen.addButton(tr("Download..."), QMessageBox::ActionRole);
//      }
//      else
//          warnDataBaseOpen.setText(tr("Cannot open database file...\nThere is an error!\nError: %1\nDataBase Path=%2").arg(errorString).arg(sqliteDbCompletePath));

//      warnDataBaseOpen.setStandardButtons(QMessageBox::Ok);
//      warnDataBaseOpen.setEscapeButton(QMessageBox::Ok);
//      warnDataBaseOpen.setDefaultButton(QMessageBox::Ok);
//      warnDataBaseOpen.setWindowFlags(Qt::WindowStaysOnTopHint);
//      int ret = warnDataBaseOpen.exec();
        QtWin::easyBlurUnBlur(&noDataBaseDialog, Settings::READ("UseTransparecy").toBool());
        int ret = noDataBaseDialog.exec();
        if ( /*ret != QMessageBox::Ok &&*/
            noDataBaseDialog.clickedButton() != noDataBaseDialog.ui->exitPushButton
            &&
            errorString.simplified().isEmpty()) {
            //if (warnDataBaseOpen.clickedButton() == browseButton)
            if (noDataBaseDialog.clickedButton() == noDataBaseDialog.ui->selectDataBase) {
                QString dir = QFileDialog::getExistingDirectory(0, tr("Add Path For Data Base"), dBFile.absoluteDir().path(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
                if (!dir.isEmpty()) {
                    dir.replace(QString("\\"), QString("/"));
                    if (!dir.endsWith('/')) {
                        dir += "/";
                    }
                    dBFile.setFile(dir + "/ganjoor.s3db");
//                  dBName = getIdForDataBase(dir+"/ganjoor.s3db");
//                  dBConnection = QSqlDatabase::database(dBName, false);//don't open! we won't it created if not exists
                    //dBName = dBFile.fileName();
                    dBName = dir + "/ganjoor.s3db";
                    dBConnection = QSqlDatabase::addDatabase(sqlDriver, dBName);
                    dBConnection.setDatabaseName(dir + "/ganjoor.s3db");

                    flagSelectNewPath = true;
                    newPath = dir;
                }
                else {
                    exit(1);
                }
            }
            //else if (warnDataBaseOpen.clickedButton() == downloadButton)
            else if (noDataBaseDialog.clickedButton() == noDataBaseDialog.ui->createDataBaseFromLocal
                     ||
                     noDataBaseDialog.clickedButton() == noDataBaseDialog.ui->createDataBaseFromRemote) {
                //do download job!!
                QFileDialog getDir(0, tr("Select Save Location"), dBFile.absoluteDir().path());
                getDir.setFileMode(QFileDialog::Directory);
                getDir.setOptions(QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks /*| QFileDialog::DontUseNativeDialog*/);
                //getDir.setOptions(getDir.options() & ~QFileDialog::DontUseNativeDialog);
                //getDir.setLayoutDirection(Qt::LeftToRight);
                getDir.setAcceptMode(QFileDialog::AcceptOpen);
                QtWin::easyBlurUnBlur(&getDir, Settings::READ("UseTransparecy").toBool());
                getDir.exec();
                QString dir = "";
                if (!getDir.selectedFiles().isEmpty()) {
                    dir = getDir.selectedFiles().at(0);
                }
                //QString dir = QFileDialog::getExistingDirectory(0,tr("Select Save Location"), dBFile.absoluteDir().path(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);
                while (dir.isEmpty() || QFile::exists(dir + "/ganjoor.s3db")) {
                    getDir.exec();
                    if (!getDir.selectedFiles().isEmpty()) {
                        dir = getDir.selectedFiles().at(0);
                    }
//                  dir = QFileDialog::getExistingDirectory(0,tr("Select Save Location"), dir.isEmpty() ? dBFile.absoluteDir().path() : dir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::DontUseNativeDialog);
                }
                // now dir is not empty and it doesn't contain 'ganjoor.s3db'
                dir.replace(QString("\\"), QString("/"));
                if (!dir.endsWith('/')) {
                    dir += "/";
                }
                dBFile.setFile(dir + "/ganjoor.s3db");
//              dBName = getIdForDataBase(dir+"/ganjoor.s3db");
//              dBConnection = QSqlDatabase::database(dBName, true);//it doesn't exist, it's created.
//              dBName = dBFile.fileName();
                dBName = dir + "/ganjoor.s3db";
                dBConnection = QSqlDatabase::addDatabase(sqlDriver, dBName);
                dBConnection.setDatabaseName(dir + "/ganjoor.s3db");
                if (!dBConnection.open()) {
                    QMessageBox::information(0, tr("Cannot open Database File!"), tr("Cannot open database file, please check if you have write permisson.\nError: %1\nDataBase Path=%2").arg(errorString).arg(dir));
                    exit(1);
                }
                //insert main tables
                bool tablesInserted = createEmptyDataBase(dBName);
                qDebug() << "insert main tables: " << tablesInserted;
                if (!tablesInserted) {
                    QFile::remove(dir + "/ganjoor.s3db");
                    exit(1);
                }
                m_addRemoteDataSet = noDataBaseDialog.clickedButton() == noDataBaseDialog.ui->createDataBaseFromRemote;
                QTimer::singleShot(0, this, SLOT(addDataSets()));
                flagSelectNewPath = true;
                newPath = dir;
            }
        }
        else {
            exit(1);
        }
    }

    dBConnection.close();
    qDebug() << __LINE__ << __FUNCTION__;
    QSqlDatabase::removeDatabase(dBName);
    dBName = getIdForDataBase(dBConnection.databaseName());
    dBConnection = QSqlDatabase::database(dBName, true);

    qDebug() << "dBName=" << dBName;

    if (flagSelectNewPath) {
        QGanjoorDbBrowser::dataBasePath.clear();//in this version Saaghar just use its first search path
        QGanjoorDbBrowser::dataBasePath << newPath;
    }

    cachedMaxCatID = cachedMaxPoemID = 0;
}

QGanjoorDbBrowser::~QGanjoorDbBrowser()
{
}

bool QGanjoorDbBrowser::isConnected(const QString &connectionID)
{
    QString connectionName = dBName;
    if (!connectionID.isEmpty()) {
        connectionName = connectionID;
    }
    dBConnection = QSqlDatabase::database(connectionName, true);//dBName
    return dBConnection.isOpen();
}

bool QGanjoorDbBrowser::isValid(QString connectionID)
{
    if (connectionID.isEmpty()) {
        connectionID = dBName;
    }
    QSqlDatabase db = QSqlDatabase::database(connectionID);
    if (db.open()) {
        return db.tables().contains("cat");
    }

    return false;
}

QList<GanjoorPoet*> QGanjoorDbBrowser::getPoets(const QString &connectionID, bool sort)
{
    QList<GanjoorPoet*> poets;
    if (isConnected(connectionID)) {
        QSqlDatabase dataBaseObject = dBConnection;
        if (!connectionID.isEmpty()) {
            dataBaseObject = QSqlDatabase::database(connectionID);
        }
        QSqlQuery q(dataBaseObject);
        bool descriptionExists = false;
        if (q.exec("SELECT id, name, cat_id, description FROM poet")) {
            descriptionExists = true;
        }
        else {
            q.exec("SELECT id, name, cat_id FROM poet");
        }

        q.first();
        while (q.isValid() && q.isActive()) {
            GanjoorPoet* gPoet = new GanjoorPoet();
            QSqlRecord qrec = q.record();
            if (descriptionExists) {
                gPoet->init(qrec.value(0).toInt(),    qrec.value(1).toString(), qrec.value(2).toInt(), qrec.value(3).toString());
            }
            else {
                gPoet->init(qrec.value(0).toInt(),    qrec.value(1).toString(), qrec.value(2).toInt(), "");
            }
            poets.append(gPoet);
            if (!q.next()) {
                break;
            }
        }
        if (sort) {
            qSort(poets.begin(), poets.end(), comparePoetsByName);
        }
    }
    return poets;
}

bool QGanjoorDbBrowser::comparePoetsByName(GanjoorPoet* poet1, GanjoorPoet* poet2)
{
    return (QString::localeAwareCompare(poet1->_Name, poet2->_Name) < 0);
}

GanjoorCat QGanjoorDbBrowser::getCategory(int CatID)
{
    GanjoorCat gCat;
    gCat.init();
    if (isConnected()) {
        QSqlQuery q(dBConnection);
        q.exec("SELECT poet_id, text, parent_id, url FROM cat WHERE id = " + QString::number(CatID));
        q.first();
        if (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            gCat.init(
                CatID,
                (qrec.value(0)).toInt(),
                (qrec.value(1)).toString(),
                (qrec.value(2)).toInt(),
                (qrec.value(3)).toString());
            return gCat;
        }
    }
    return gCat;
}

bool QGanjoorDbBrowser::compareCategoriesByName(GanjoorCat* cat1, GanjoorCat* cat2)
{
    return (QString::localeAwareCompare(cat1->_Text, cat2->_Text) < 0);
}

QList<GanjoorCat*> QGanjoorDbBrowser::getSubCategories(int CatID)
{
    QList<GanjoorCat*> lst;
    if (isConnected()) {
        QSqlQuery q(dBConnection);
        q.exec("SELECT poet_id, text, url, ID FROM cat WHERE parent_id = " + QString::number(CatID));
        q.first();
        while (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            GanjoorCat* gCat = new GanjoorCat();
            gCat->init((qrec.value(3)).toInt(), (qrec.value(0)).toInt(), (qrec.value(1)).toString(), CatID, (qrec.value(2)).toString());
            lst.append(gCat);
            if (!q.next()) {
                break;
            }
        }
        if (CatID == 0) {
            qSort(lst.begin(), lst.end(), compareCategoriesByName);
        }
    }
    return lst;
}

QList<GanjoorCat> QGanjoorDbBrowser::getParentCategories(GanjoorCat Cat)
{
    QList<GanjoorCat> lst;
    if (isConnected()) {
        while (!Cat.isNull() && Cat._ParentID != 0) {
            Cat = getCategory(Cat._ParentID);
            lst.insert(0, Cat);
        }
        GanjoorCat gCat;
        gCat.init(0, 0, tr("Home"), 0, QString());
        lst.insert(0, gCat);
    }
    return lst;
}

QList<GanjoorPoem*> QGanjoorDbBrowser::getPoems(int CatID)
{
    QList<GanjoorPoem*> lst;
    if (isConnected()) {
        QString selectQuery = QString("SELECT ID, title, url FROM poem WHERE cat_id = %1 ORDER BY ID").arg(CatID);
        QSqlQuery q(dBConnection);
        q.exec(selectQuery);
        q.first();
        QSqlRecord qrec;
        while (q.isValid() && q.isActive()) {
            qrec = q.record();
            GanjoorPoem* gPoem = new GanjoorPoem();
            gPoem->init((qrec.value(0)).toInt(), CatID, (qrec.value(1)).toString(), (qrec.value(2)).toString(), false, "");
            lst.append(gPoem);
            if (!q.next()) {
                break;
            }
        }
    }
    return lst;
}

QList<GanjoorVerse*> QGanjoorDbBrowser::getVerses(int PoemID)
{
    return getVerses(PoemID, 0);
}

QString QGanjoorDbBrowser::getFirstMesra(int PoemID) //just first Mesra
{
    if (isConnected()) {
        QSqlQuery q(dBConnection);
        QString selectQuery = QString("SELECT vorder, text FROM verse WHERE poem_id = %1 order by vorder LIMIT 1").arg(PoemID);
        q.exec(selectQuery);
        q.first();
        while (q.isValid() && q.isActive()) {
            return snippedText(q.record().value(1).toString(), "", 0, 12, true);
        }
    }
    return "";
}

QList<GanjoorVerse*> QGanjoorDbBrowser::getVerses(int PoemID, int Count)
{
    QList<GanjoorVerse*> lst;
    if (isConnected()) {
        QSqlQuery q(dBConnection);
        QString selectQuery = QString("SELECT vorder, position, text FROM verse WHERE poem_id = %1 order by vorder" + (Count > 0 ? " LIMIT " + QString::number(Count) : "")).arg(PoemID);
        q.exec(selectQuery);
        q.first();
        while (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            GanjoorVerse* gVerse = new GanjoorVerse();
            gVerse->init(PoemID, (qrec.value(0)).toInt(), (VersePosition)((qrec.value(1)).toInt()), (qrec.value(2)).toString());
            lst.append(gVerse);
            if (!q.next()) {
                break;
            }
        }
    }
    return lst;
}

GanjoorPoem QGanjoorDbBrowser::getPoem(int PoemID)
{
    GanjoorPoem gPoem;
    gPoem.init();
    if (isConnected()) {
        QSqlQuery q(dBConnection);
        q.exec("SELECT cat_id, title, url FROM poem WHERE ID = " + QString::number(PoemID));
        q.first();
        if (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            gPoem.init(PoemID, (qrec.value(0)).toInt(), (qrec.value(1)).toString(), (qrec.value(2)).toString(), false, QString(""));
            return gPoem;
        }
    }
    return gPoem;
}

GanjoorPoem QGanjoorDbBrowser::getNextPoem(int PoemID, int CatID)
{
    GanjoorPoem gPoem;
    gPoem.init();
    if (isConnected() && PoemID != -1) { // PoemID==-1 when getNextPoem(GanjoorPoem poem) pass null poem
        QSqlQuery q(dBConnection);
        q.exec(QString("SELECT ID FROM poem WHERE cat_id = %1 AND id>%2 LIMIT 1").arg(CatID).arg(PoemID));
        q.first();
        if (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            gPoem = getPoem((qrec.value(0)).toInt());
            return gPoem;
        }
    }
    return gPoem;
}

GanjoorPoem QGanjoorDbBrowser::getNextPoem(GanjoorPoem poem)
{
    return getNextPoem(poem._ID, poem._CatID);
}

GanjoorPoem QGanjoorDbBrowser::getPreviousPoem(int PoemID, int CatID)
{
    GanjoorPoem gPoem;
    gPoem.init();
    if (isConnected() && PoemID != -1) { // PoemID==-1 when getPreviousPoem(GanjoorPoem poem) pass null poem
        QSqlQuery q(dBConnection);
        q.exec(QString("SELECT ID FROM poem WHERE cat_id = %1 AND id<%2 ORDER BY ID DESC LIMIT 1").arg(CatID).arg(PoemID));
        q.first();
        if (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            gPoem = getPoem((qrec.value(0)).toInt());
            return gPoem;
        }
    }
    return gPoem;
}

GanjoorPoem QGanjoorDbBrowser::getPreviousPoem(GanjoorPoem poem)
{
    return getPreviousPoem(poem._ID, poem._CatID);
}

GanjoorPoet QGanjoorDbBrowser::getPoetForCat(int CatID)
{
    GanjoorPoet gPoet;
    gPoet.init();
    if (CatID == 0) {
        gPoet.init(0, tr("All"), 0);
        return gPoet;
    }
    if (isConnected()) {
        QSqlQuery q(dBConnection);
        q.exec("SELECT poet_id FROM cat WHERE id = " + QString::number(CatID));
        q.first();
        if (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            return getPoet((qrec.value(0)).toInt());
        }
    }
    return gPoet;
}

GanjoorPoet QGanjoorDbBrowser::getPoet(int PoetID)
{
    GanjoorPoet gPoet;
    gPoet.init();
    if (PoetID == 0) {
        gPoet.init(0, tr("All"), 0);
        return gPoet;
    }
    if (isConnected()) {
        QSqlQuery q(dBConnection);

        bool descriptionExists = false;
        if (q.exec("SELECT id, name, cat_id, description FROM poet WHERE id = " + QString::number(PoetID))) {
            descriptionExists = true;
        }
        else {
            q.exec("SELECT id, name, cat_id FROM poet WHERE id = " + QString::number(PoetID));
        }

        q.first();
        if (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            if (descriptionExists) {
                gPoet.init((qrec.value(0)).toInt(), (qrec.value(1)).toString(), (qrec.value(2)).toInt(), (qrec.value(3)).toString());
            }
            else {
                gPoet.init((qrec.value(0)).toInt(), (qrec.value(1)).toString(), (qrec.value(2)).toInt(), "");
            }

            return gPoet;
        }
    }
    return gPoet;
}

QString QGanjoorDbBrowser::getPoetDescription(int PoetID)
{
    if (PoetID <= 0) {
        return "";
    }
    if (isConnected()) {
        QSqlQuery q(dBConnection);
        q.exec("SELECT description FROM poet WHERE id = " + QString::number(PoetID));
        q.first();
        if (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            return (qrec.value(0)).toString();
        }
    }
    return "";
}

QString QGanjoorDbBrowser::getPoemMediaSource(int PoemID)
{
    if (PoemID <= 0) {
        return "";
    }
    if (isConnected()) {
        QSqlQuery q(dBConnection);
        q.exec("SELECT mediasource FROM poem WHERE id = " + QString::number(PoemID));
        q.first();
        if (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            return (qrec.value(0)).toString();
        }
    }
    return "";
}

void QGanjoorDbBrowser::setPoemMediaSource(int PoemID, const QString &fileName)
{
    if (PoemID <= 0) {
        return;
    }
    if (isConnected()) {
        QSqlQuery q(dBConnection);
        QString strQuery = "";
        if (!dBConnection.record("poem").contains("mediasource")) {
            strQuery = QString("ALTER TABLE %1 ADD COLUMN %2 %3").arg("poem").arg("mediasource").arg("NVARCHAR(255)");
            q.exec(strQuery);
            //q.finish();
        }

        strQuery = QString("UPDATE poem SET mediasource = \"%1\" WHERE id = %2 ;").arg(fileName).arg(PoemID);
        //q.exec(strQuery);
        //PRAGMA TABLE_INFO(poem);


        //QString("REPLACE INTO poem (id, cat_id, title, url, mediasource) VALUES (%1, %2, \"%3\", \"%4\", \"%5\");").arg(newPoem->_ID).arg(newPoem->_CatID).arg(newPoem->_Title).arg(newPoem->_Url).arg(fileName);
        //qDebug() << "setPoemMediaSource="<< q.exec(strQuery);
//      q.first();
//      if (q.isValid() && q.isActive())
//      {
//          QSqlRecord qrec = q.record();
//          return (qrec.value(0)).toString();
//      }
    }
    return;
}

GanjoorPoet QGanjoorDbBrowser::getPoet(QString PoetName)
{
    GanjoorPoet gPoet;
    gPoet.init();
    if (isConnected()) {
        QSqlQuery q(dBConnection);

        bool descriptionExists = false;
        if (q.exec("SELECT id, name, cat_id, description FROM poet WHERE name = \'" + PoetName + "\'")) {
            descriptionExists = true;
        }
        else {
            q.exec("SELECT id, name, cat_id FROM poet WHERE name = \'" + PoetName + "\'");
        }

        q.first();
        if (q.isValid() && q.isActive()) {
            QSqlRecord qrec = q.record();
            if (descriptionExists) {
                gPoet.init((qrec.value(0)).toInt(), (qrec.value(1)).toString(), (qrec.value(2)).toInt(), (qrec.value(3)).toString());
            }
            else {
                gPoet.init((qrec.value(0)).toInt(), (qrec.value(1)).toString(), (qrec.value(2)).toInt(), "");
            }

            return gPoet;
        }
    }
    return gPoet;
}

int QGanjoorDbBrowser::getRandomPoemID(int* CatID)
{
    if (isConnected()) {
        QList<GanjoorCat*> subCats = getSubCategories(*CatID);
        int randIndex = QGanjoorDbBrowser::getRandomNumber(0, subCats.size());
        if (randIndex >= 0 && randIndex != subCats.size()) { // '==' is when we want to continue with current 'CatID'. notice maybe 'randIndex == subCats.size() == 0'.
            *CatID = subCats.at(randIndex)->_ID;
            return getRandomPoemID(CatID);
        }

        QString strQuery = "SELECT MIN(id), MAX(id) FROM poem";
        if (*CatID != 0) {
            strQuery += " WHERE cat_id=" + QString::number(*CatID);
        }
        QSqlQuery q(dBConnection);
        q.exec(strQuery);
        if (q.next()) {
            QSqlRecord qrec = q.record();
            int minID = qrec.value(0).toInt();
            int maxID = qrec.value(1).toInt();

            if (maxID <= 0 || minID < 0) {
                QList<GanjoorCat*> subCats = getSubCategories(*CatID);
                *CatID = subCats.at(QGanjoorDbBrowser::getRandomNumber(0, subCats.size() - 1))->_ID;
                return getRandomPoemID(CatID);
            }
            return QGanjoorDbBrowser::getRandomNumber(minID, maxID);
        }
    }
    return -1;
}

int QGanjoorDbBrowser::getRandomNumber(int minBound, int maxBound)
{
    if ((maxBound < minBound) || (minBound < 0)) {
        return -1;
    }
    if (minBound == maxBound) {
        return minBound;
    }

    //Generate a random number in the range [0.5f, 1.0f).
    unsigned int ret = 0x3F000000 | (0x7FFFFF & ((qrand() << 8) ^ qrand()));
    unsigned short coinFlips;

    //If the coin is tails, return the number, otherwise
    //divide the random number by two by decrementing the
    //exponent and keep going. The exponent starts at 63.
    //Each loop represents 15 random bits, a.k.a. 'coin flips'.
#define RND_INNER_LOOP() \
    if( coinFlips & 1 ) break; \
    coinFlips >>= 1; \
    ret -= 0x800000
    for (;;) {
        coinFlips = qrand();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        //At this point, the exponent is 60, 45, 30, 15, or 0.
        //If the exponent is 0, then the number equals 0.0f.
        if (!(ret & 0x3F800000)) {
            return 0.0f;
        }
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
        RND_INNER_LOOP();
    }

    float rand = *((float*)(&ret));
    return (int)(rand * (maxBound - minBound + 1) + minBound);
}

QList<GanjoorPoet*> QGanjoorDbBrowser::getDataBasePoets(const QString fileName)
{
    QList<GanjoorPoet*> poetsInDataBase;

    if (!QFile::exists(fileName)) {
        return poetsInDataBase;
    }

    QString dataBaseID = getIdForDataBase(fileName);
    QSqlDatabase databaseObject = QSqlDatabase::database(dataBaseID);
    if (!databaseObject.open()) {
        qDebug() << __LINE__ << __FUNCTION__;
        QSqlDatabase::removeDatabase(dataBaseID);
        return poetsInDataBase;
    }

    poetsInDataBase = getPoets(dataBaseID, false);
    QSqlQuery q(databaseObject);

    databaseObject.close();
    qDebug() << __LINE__ << __FUNCTION__;
    QSqlDatabase::removeDatabase(dataBaseID);

    return poetsInDataBase;
}

QString QGanjoorDbBrowser::getIdForDataBase(const QString &sqliteDataBaseName)
{
    //QFileInfo dataBaseFile(sqliteDataBaseName);
    //QString connectionID = dataBaseFile.fileName();//we need to be sure this name is unique
    QString connectionID = QGanjoorDbBrowser::getLongPathName(sqliteDataBaseName);

    if (!QSqlDatabase::contains(connectionID)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(sqlDriver, connectionID);
        db.setDatabaseName(sqliteDataBaseName);
    }

    return connectionID;
}

QList<GanjoorPoet*> QGanjoorDbBrowser::getConflictingPoets(const QString fileName)
{
    QList<GanjoorPoet*> conflictList;
    if (isConnected()) {
        QList<GanjoorPoet*> dataBasePoets = getDataBasePoets(fileName);
        QList<GanjoorPoet*> installedPoets = getPoets();
        foreach (GanjoorPoet* poet, installedPoets) {
            foreach (GanjoorPoet* dbPoet, dataBasePoets) {
                if (dbPoet->_ID == poet->_ID) {
                    conflictList << poet;
                    break;
                }
            }
        }
    }

    return conflictList;
}

void QGanjoorDbBrowser::removePoetFromDataBase(int PoetID)
{
    if (isConnected()) {
        QString strQuery;
        QSqlQuery q(dBConnection);

        removeCatFromDataBase(getCategory(getPoet(PoetID)._CatID));

        strQuery = "DELETE FROM poet WHERE id=" + QString::number(PoetID);
        q.exec(strQuery);
    }
}

void QGanjoorDbBrowser::removeCatFromDataBase(const GanjoorCat &gCat)
{
    if (gCat.isNull()) {
        return;
    }
    QList<GanjoorCat*> subCats = getSubCategories(gCat._ID);
    foreach (GanjoorCat* cat, subCats) {
        removeCatFromDataBase(*cat);
    }
    if (isConnected()) {
        QString strQuery;
        QSqlQuery q(dBConnection);
        strQuery = "DELETE FROM verse WHERE poem_id IN (SELECT id FROM poem WHERE cat_id=" + QString::number(gCat._ID) + ")";
        q.exec(strQuery);
        strQuery = "DELETE FROM poem WHERE cat_id=" + QString::number(gCat._ID);
        q.exec(strQuery);
        strQuery = "DELETE FROM cat WHERE id=" + QString::number(gCat._ID);
        q.exec(strQuery);
    }
}

int QGanjoorDbBrowser::getNewPoetID()
{
    int newPoetID = -1;

    if (isConnected()) {
        QString strQuery = "SELECT MAX(id) FROM poet";
        QSqlQuery q(dBConnection);
        q.exec(strQuery);

        if (q.first()) {
            bool ok = false;
            newPoetID = q.value(0).toInt(&ok) + 1;
            if (!ok) {
                newPoetID = minNewPoetID + 1;
            }
        }
    }

    if (newPoetID < minNewPoetID) {
        newPoetID = minNewPoetID + 1;
    }

    return newPoetID;
}
int QGanjoorDbBrowser::getNewPoemID()
{
    int newPoemID = -1;

    if (cachedMaxPoemID == 0) {
        if (isConnected()) {
            QString strQuery = "SELECT MAX(id) FROM poem";
            QSqlQuery q(dBConnection);
            q.exec(strQuery);

            if (q.first()) {
                bool ok = false;
                newPoemID = q.value(0).toInt(&ok) + 1;
                if (!ok) {
                    newPoemID = minNewPoemID + 1;
                }
            }
        }

        if (newPoemID < minNewPoemID) {
            newPoemID = minNewPoemID + 1;
        }

        cachedMaxPoemID = newPoemID;
    }
    else {
        newPoemID = ++cachedMaxPoemID;
    }

    return newPoemID;
}

int QGanjoorDbBrowser::getNewCatID()
{
    int newCatID = -1;

    if (cachedMaxCatID == 0) {
        if (isConnected()) {
            QString strQuery = "SELECT MAX(id) FROM cat";
            QSqlQuery q(dBConnection);
            q.exec(strQuery);

            if (q.first()) {
                bool ok = false;
                newCatID = q.value(0).toInt(&ok) + 1;
                if (!ok) {
                    newCatID = minNewCatID + 1;
                }
            }
        }

        if (newCatID < minNewCatID) {
            newCatID = minNewCatID + 1;
        }

        cachedMaxCatID = newCatID;
    }
    else {
        newCatID = ++cachedMaxCatID;
    }

    return newCatID;
}

bool QGanjoorDbBrowser::importDataBase(const QString fileName)
{
    if (!QFile::exists(fileName)) {
        return false;
    }

    QString connectionID = getIdForDataBase(fileName);
    {
        QSqlDatabase dataBaseObject = QSqlDatabase::database(connectionID);
        if (!dataBaseObject.open() || !isValid(connectionID)) {
            qDebug() << __LINE__ << __FUNCTION__;
            QSqlDatabase::removeDatabase(connectionID);
            return false;
        }

        QList<GanjoorPoet*> poets = getPoets(connectionID, false);

        QString strQuery;
        QSqlQuery queryObject(dataBaseObject);
        QMap<int, int> mapPoets;
        QMap<int, int> mapCats;

        foreach (GanjoorPoet* newPoet, poets) {
            bool insertNewPoet = true;

            //skip every null poet(poet without subcat)
            if (!poetHasSubCats(newPoet->_ID, connectionID)) {
                continue;
            }

            GanjoorPoet poet = getPoet(newPoet->_Name);
            if (!poet.isNull()) { //conflict on Names
                if (poet._ID == newPoet->_ID) {
                    insertNewPoet = false;
                    mapPoets.insert(newPoet->_ID, newPoet->_ID);
                    GanjoorCat poetCat = getCategory(newPoet->_CatID);
                    if (!poetCat.isNull()) {
                        if (poetCat._PoetID == newPoet->_ID) {
                            mapCats.insert(newPoet->_CatID, newPoet->_CatID);
                        }
                        else {
                            int aRealyNewCatID = getNewCatID();
                            mapCats.insert(newPoet->_CatID, getNewCatID());
                            newPoet->_CatID = aRealyNewCatID;
                        }
                    }
                }
                else {
                    mapPoets.insert(newPoet->_ID, poet._ID);
                    mapPoets.insert(newPoet->_CatID, poet._CatID);
                    newPoet->_CatID = poet._CatID;
                }
            }
            else {
                if (!getPoet(newPoet->_ID).isNull()) { //conflict on IDs
                    int aRealyNewPoetID = getNewPoetID();
                    mapPoets.insert(newPoet->_ID, aRealyNewPoetID);
                    newPoet->_ID = aRealyNewPoetID;

                    int aRealyNewCatID = getNewCatID();
                    mapCats.insert(newPoet->_CatID, aRealyNewCatID);
                    newPoet->_CatID = aRealyNewCatID;
                }
                else { //no conflict, insertNew
                    mapPoets.insert(newPoet->_ID, newPoet->_ID);
                    GanjoorCat newPoetCat = getCategory(newPoet->_CatID);
                    if (newPoetCat.isNull()) {
                        mapCats.insert(newPoet->_CatID, newPoet->_CatID);
                    }
                    else {
                        int aRealyNewCatID = getNewCatID();
                        mapCats.insert(newPoet->_CatID, getNewCatID());
                        newPoet->_CatID = aRealyNewCatID;
                    }
                }
            }

            if (insertNewPoet && isConnected()) {
                strQuery = QString("INSERT INTO poet (id, name, cat_id, description) VALUES (%1, \"%2\", %3, \"%4\");").arg(newPoet->_ID).arg(newPoet->_Name).arg(newPoet->_CatID).arg(newPoet->_Description);
                QSqlQuery q(dBConnection);
                q.exec(strQuery);
            }
        }

        strQuery = QString("SELECT id, poet_id, text, parent_id, url FROM cat");
        queryObject.exec(strQuery);

        QList<GanjoorCat*> gCatList;
        while (queryObject.next()) {
            GanjoorCat* gCat = new GanjoorCat();
            gCat->init(queryObject.value(0).toInt(), queryObject.value(1).toInt(), queryObject.value(2).toString(), queryObject.value(3).toInt(), queryObject.value(4).toString());
            gCatList.append(gCat);
        }

        foreach (GanjoorCat* newCat, gCatList) {
            newCat->_PoetID = mapPoets.value(newCat->_PoetID, newCat->_PoetID);
            newCat->_ParentID = mapCats.value(newCat->_ParentID, newCat->_ParentID);

            bool insertNewCategory = true;
            GanjoorCat gCat = getCategory(newCat->_ID);
            if (!gCat.isNull()) {
                if (gCat._PoetID == newCat->_PoetID) {
                    insertNewCategory = false;

                    if (mapCats.value(newCat->_ID, -1) == -1) {
                        mapCats.insert(newCat->_ID, newCat->_ID);
                    }
                }
                else {
                    int aRealyNewCatID = getNewCatID();
                    mapCats.insert(newCat->_ID, aRealyNewCatID);
                    newCat->_ID = aRealyNewCatID;
                }
            }
            else {
                if (mapCats.value(newCat->_ID, -1) == -1) {
                    mapCats.insert(newCat->_ID, newCat->_ID);
                }
            }

            if (insertNewCategory && isConnected()) {
                strQuery = QString("INSERT INTO cat (id, poet_id, text, parent_id, url) VALUES (%1, %2, \"%3\", %4, \"%5\");").arg(newCat->_ID).arg(newCat->_PoetID).arg(newCat->_Text).arg(newCat->_ParentID).arg(newCat->_Url);
                QSqlQuery q(dBConnection);
                q.exec(strQuery);
            }

            /*if ( getPoet(newCat->_PoetID).isNull() )
            {
                //missing poet
                int poetCat;
                if (newCat->_ParentID == 0)
                    poetCat = newCat->_ID;
                else
                {
                    //this is not good:
                    poetCat = newCat->_ParentID;
                }
                qDebug() << "a new poet should be created";
                this.NewPoet("شاعر " + PoetID.ToString(), PoetID, poetCat);
            }*/
        }

        QMap<int, int> dicPoemID;
        strQuery = "SELECT id, cat_id, title, url FROM poem";
        queryObject.exec(strQuery);
        QList<GanjoorPoem*> poemList;

        while (queryObject.next()) {
            GanjoorPoem* gPoem = new GanjoorPoem();
            gPoem->init(queryObject.value(0).toInt(), queryObject.value(1).toInt(), queryObject.value(2).toString(), queryObject.value(3).toString());
            poemList.append(gPoem);
        }

        foreach (GanjoorPoem* newPoem, poemList) {
            int tmp = newPoem->_ID;
            if (!getPoem(newPoem->_ID).isNull()) {
                newPoem->_ID = getNewPoemID();
            }
            newPoem->_CatID = mapCats.value(newPoem->_CatID, newPoem->_CatID);
            dicPoemID.insert(tmp, newPoem->_ID);

            if (isConnected()) {
                strQuery = QString("INSERT INTO poem (id, cat_id, title, url) VALUES (%1, %2, \"%3\", \"%4\");").arg(newPoem->_ID).arg(newPoem->_CatID).arg(newPoem->_Title).arg(newPoem->_Url);
                QSqlQuery q(dBConnection);
                q.exec(strQuery);
            }
        }

        strQuery = "SELECT poem_id, vorder, position, text FROM verse";
        queryObject.exec(strQuery);
        QList<GanjoorVerse*> verseList;

        while (queryObject.next()) {
            GanjoorVerse* gVerse = new GanjoorVerse();
            gVerse->init(queryObject.value(0).toInt(), queryObject.value(1).toInt(), (VersePosition)queryObject.value(2).toInt(), queryObject.value(3).toString());
            verseList.append(gVerse);
        }

        if (isConnected()) {
            strQuery = "INSERT INTO verse (poem_id, vorder, position, text) VALUES (:poem_id,:vorder,:position,:text)";
            QSqlQuery q(dBConnection);
            q.prepare(strQuery);

            foreach (GanjoorVerse* newVerse, verseList) {
                newVerse->_PoemID = dicPoemID.value(newVerse->_PoemID, newVerse->_PoemID);
                q.bindValue(":poem_id", newVerse->_PoemID);
                q.bindValue(":vorder", newVerse->_Order);
                q.bindValue(":position", newVerse->_Position);
                q.bindValue(":text", newVerse->_Text);
                q.exec();
            }
        }
    }
    //qDebug() << "BEFORE CLOSE";
    //dataBaseObject.close();
    qDebug() << "AFTER CLOSE - BEFORE REMOVE";
    qDebug() << __LINE__ << __FUNCTION__;
    QSqlDatabase::removeDatabase(connectionID);
    qDebug() << "AFTER REMOVE";
    return true;
}

QString QGanjoorDbBrowser::cleanString(const QString &text, const QStringList &excludeList)
{
    QString cleanedText = text;

    cleanedText.replace(QChar(0x200C), "", Qt::CaseInsensitive);//replace ZWNJ by ""

    // 14s-->10s
    if (SearchResultWidget::skipVowelLetters) {
        cleanedText.replace(Ve_EXP, QChar(72, 6) /*Ve_Variant.at(0)*/);
        cleanedText.replace(Ye_EXP, QChar(204, 6) /*Ye_Variant.at(0)*/);
        cleanedText.replace(AE_EXP, QChar(39, 6) /*AE_Variant.at(0)*/);
        cleanedText.replace(He_EXP, QChar(71, 6) /*He_Variant.at(0)*/);
    }

    for (int i = 0; i < cleanedText.size(); ++i) {
        QChar tmpChar = cleanedText.at(i);

        if (excludeList.contains(tmpChar)) {
            continue;
        }

        QChar::Direction chDir = tmpChar.direction();

        //someSymbols.contains(tmpChar) consumes lots of time, 8s --> 10s
        if ((SearchResultWidget::skipVowelSigns && chDir == QChar::DirNSM) ||
                tmpChar.isSpace() || someSymbols.contains(tmpChar)) {
            cleanedText.remove(tmpChar);
            --i;
            continue;
        }
    }
    return cleanedText;
}

QString QGanjoorDbBrowser::cleanStringFast(const QString &text, const QStringList &excludeList)
{
    QString cleanedText = text;

    cleanedText.remove(QChar(0x200C));//.replace(QChar(0x200C), "", Qt::CaseInsensitive);//replace ZWNJ by ""

    // 14s-->10s
    if (SearchResultWidget::skipVowelLetters) {
        cleanedText.replace(Ve_EXP, QChar(72, 6) /*Ve_Variant.at(0)*/);
        cleanedText.replace(Ye_EXP, QChar(204, 6) /*Ye_Variant.at(0)*/);
        cleanedText.replace(AE_EXP, QChar(39, 6) /*AE_Variant.at(0)*/);
        cleanedText.replace(He_EXP, QChar(71, 6) /*He_Variant.at(0)*/);
    }
    cleanedText.remove(SomeSymbol_EXP);

    for (int i = 0; i < cleanedText.size(); ++i) {
        QChar tmpChar = cleanedText.at(i);

        if (excludeList.contains(tmpChar)) {
            continue;
        }

        QChar::Direction chDir = tmpChar.direction();

        //someSymbols.contains(tmpChar) consumes lots of time, 8s --> 10s
        if ((SearchResultWidget::skipVowelSigns && chDir == QChar::DirNSM) || tmpChar.isSpace()) {
            cleanedText.remove(tmpChar);
            --i;
            continue;
        }
    }
    return cleanedText;
}

QMap<int, QString> QGanjoorDbBrowser::getPoemIDsByPhrase(int PoetID, const QStringList &phraseList, const QStringList &excludedList,
        bool* Canceled, int resultCount, bool slowSearch)
{
    QMap<int, QString> idList;
    if (isConnected()) {
        QString strQuery;

        if (phraseList.isEmpty()) {
            return idList;
        }

        bool findRhyme = false;
        if (phraseList.contains("=", Qt::CaseInsensitive)) {
            findRhyme = true;
        }

        QString firstPhrase = phraseList.at(0);

        QStringList excludeWhenCleaning("");
        if (!firstPhrase.contains("=")) {
            excludeWhenCleaning << " ";
        }
        else {
            firstPhrase.remove("=");
        }

        QString joiner;
        if (SearchResultWidget::skipVowelSigns) {
            joiner = QLatin1String("%");
        }
        //replace characters that have some variants with anyWord replaceholder!
        //we have not to worry about this replacement, because phraseList.at(0) is tested again!
        bool variantPresent = false;
        if (SearchResultWidget::skipVowelLetters) {
            QRegExp variantEXP("[" + Ve_Variant.join("") + AE_Variant.join("") + He_Variant.join("") + Ye_Variant.join("") + "]+");
            if (firstPhrase.contains(variantEXP)) {
                firstPhrase.replace(variantEXP, "%");
                variantPresent = true;
            }
        }

        QStringList anyWordedList = firstPhrase.split("%%", QString::SkipEmptyParts);
        for (int i = 0; i < anyWordedList.size(); ++i) {
            QString subPhrase = anyWordedList.at(i);
            if (SearchResultWidget::skipVowelSigns) {
                subPhrase.remove("%");
            }
            //TODO: remove skipNonAlphabet and replace it with NEAR operator
            //search for firstPhrase then go for other ones
            subPhrase = subPhrase.simplified();
            if (!subPhrase.contains(" ") || variantPresent) {
                subPhrase = QGanjoorDbBrowser::cleanString(subPhrase, excludeWhenCleaning).split("", QString::SkipEmptyParts).join(joiner);
            }
            subPhrase.replace("% %", "%");
            anyWordedList[i] = subPhrase;
        }

        QString searchQueryPhrase = anyWordedList.join("%");

        int andedPhraseCount = phraseList.size();
        int excludedCount = excludedList.size();
        int numOfFounded = 0;

        if (PoetID == 0) {
            strQuery = QString("SELECT poem_id, text, vorder FROM verse WHERE text LIKE \'%" + searchQueryPhrase + "%\' ORDER BY poem_id");
        }
        else if (PoetID == -1000) { //reserved for titles!!!
            strQuery = QString("SELECT id, title FROM poem WHERE title LIKE \'%" + searchQueryPhrase + "%\' ORDER BY id");
        }
        else {
            strQuery = QString("SELECT verse.poem_id,verse.text, verse.vorder FROM (verse INNER JOIN poem ON verse.poem_id=poem.id) INNER JOIN cat ON cat.id =cat_id WHERE verse.text LIKE \'%" + searchQueryPhrase + "%\' AND poet_id=" + QString::number(PoetID) + " ORDER BY poem_id");
        }

        QSqlQuery q(dBConnection);
#ifdef SAAGHAR_DEBUG
        int start = QDateTime::currentDateTime().toTime_t() * 1000 + QDateTime::currentDateTime().time().msec();
#endif
        q.exec(strQuery);
#ifdef SAAGHAR_DEBUG
        int end = QDateTime::currentDateTime().toTime_t() * 1000 + QDateTime::currentDateTime().time().msec();
        int miliSec = end - start;
        qDebug() << "duration=" << miliSec;
#endif
        int numOfNearResult = 0, nextStep = 0, stepLenght = 300;
        if (slowSearch) {
            stepLenght = 30;
        }

        int lastPoemID = -1;
        QList<GanjoorVerse*> verses;

        while (q.next()) {
            ++numOfNearResult;
            if (numOfNearResult > nextStep) {
                nextStep += stepLenght; //500
                emit searchStatusChanged(QGanjoorDbBrowser::tr("Search Result(s): %1").arg(numOfFounded + resultCount));
                QApplication::processEvents(QEventLoop::AllEvents);
            }

            QSqlRecord qrec = q.record();
            int poemID = qrec.value(0).toInt();
//          if (idList.contains(poemID))
//              continue;//we need just first result

            QString verseText = qrec.value(1).toString();
            // assume title's order is zero!
            int verseOrder = (PoetID == -1000 ? 0 : qrec.value(2).toInt());

            QString foundedVerse = QGanjoorDbBrowser::cleanStringFast(verseText, excludeWhenCleaning);
            // for whole word option when word is in the start or end of verse
            foundedVerse = " " + foundedVerse + " ";

            //excluded list
            bool excludeCurrentVerse = false;
            for (int t = 0; t < excludedCount; ++t) {
                if (foundedVerse.contains(excludedList.at(t))) {
                    excludeCurrentVerse = true;
                    break;
                }
            }

            if (!excludeCurrentVerse) {
                for (int t = 0; t < andedPhraseCount; ++t) {
                    QString tphrase = phraseList.at(t);
                    if (tphrase.contains("==")) {
                        tphrase.remove("==");
                        if (lastPoemID != poemID/* && findRhyme*/) {
                            lastPoemID = poemID;
                            int versesSize = verses.size();
                            for (int j = 0; j < versesSize; ++j) {
                                delete verses[j];
                                verses[j] = 0;
                            }
                            verses = getVerses(poemID);
                        }
                        excludeCurrentVerse = !isRadif(verses, tphrase, verseOrder);
                        break;
                    }
                    if (tphrase.contains("=")) {
                        tphrase.remove("=");
                        if (lastPoemID != poemID/* && findRhyme*/) {
                            lastPoemID = poemID;
                            int versesSize = verses.size();
                            for (int j = 0; j < versesSize; ++j) {
                                delete verses[j];
                                verses[j] = 0;
                            }
                            verses = getVerses(poemID);
                        }
                        excludeCurrentVerse = !isRhyme(verses, tphrase, verseOrder);
                        break;
                    }
                    if (!tphrase.contains("%")) {
                        //QChar(71,6): Simple He
                        //QChar(204,6): Persian Ye
                        QString YeAsKasre = QString(QChar(71, 6)) + " ";
                        if (tphrase.contains(YeAsKasre)) {
                            tphrase.replace(YeAsKasre, QString(QChar(71, 6)) + "\\s*" + QString(QChar(204, 6)) +
                                            "{0,2}\\s+");

                            QRegExp anySearch(".*" + tphrase + ".*", Qt::CaseInsensitive);

                            if (!anySearch.exactMatch(foundedVerse)) {
                                excludeCurrentVerse = true;
                                break;
                            }
                        }
                        else {
                            if (!foundedVerse.contains(tphrase))
                                //the verse doesn't contain an ANDed phrase
                                //maybe for ++ and +++ this should be removed
                            {
                                excludeCurrentVerse = true;
                                break;
                            }
                        }
                    }
                    else {
                        tphrase = tphrase.replace("%%", ".*");
                        tphrase = tphrase.replace("%", "\\S*");
                        QRegExp anySearch(".*" + tphrase + ".*", Qt::CaseInsensitive);
                        if (!anySearch.exactMatch(foundedVerse)) {
                            excludeCurrentVerse = true;
                            break;
                        }
                    }
                }
            }

            if (Canceled && *Canceled) {
                break;
            }

            if (excludeCurrentVerse) {
#ifdef Q_OS_X11
                QApplication::processEvents(QEventLoop::WaitForMoreEvents , 3);//max wait 3 miliseconds
#endif
                continue;
            }

            ++numOfFounded;
            GanjoorPoem gPoem = getPoem(poemID);
            idList.insertMulti(poemID, "verseText=" + verseText + "|poemTitle=" + gPoem._Title + "|poetName=" + getPoetForCat(gPoem._CatID)._Name);
#ifdef Q_OS_X11
            QApplication::processEvents(QEventLoop::WaitForMoreEvents , 3);//max wait 3 miliseconds
#endif

        }

        //for the last result
        emit searchStatusChanged(QGanjoorDbBrowser::tr("Search Result(s): %1").arg(numOfFounded + resultCount));

        int versesSize = verses.size();
        for (int j = 0; j < versesSize; ++j) {
            delete verses[j];
            verses[j] = 0;
        }

        return idList;
    }
    return idList;
}

QString QGanjoorDbBrowser::justifiedText(const QString &text, const QFontMetrics &fontmetric, int width)
{
    int textWidth = fontmetric.width(text);
    QChar tatweel = QChar(0x0640);
    int tatweelWidth = fontmetric.width(tatweel);
    if (tatweel == QChar(QChar::ReplacementCharacter)) { //Current font has not a TATWEEL character
        tatweel = QChar(QChar::Null);
        tatweelWidth = 0;
    }

//force justification by space on MacOSX
#ifdef Q_OS_MAC
    tatweel = QChar(QChar::Null);
    tatweelWidth = 0;
#endif

    int spaceWidth = fontmetric.width(" ");

    if (textWidth >= width || width <= 0) {
        return text;
    }

    int numOfTatweels = tatweelWidth ? (width - textWidth) / tatweelWidth : 0;
    int numOfSpaces = (width - (textWidth + numOfTatweels * tatweelWidth)) / spaceWidth;

    QList<int> tatweelPositions;
    QList<int> spacePositions;
    QStringList charsOfText = text.split("", QString::SkipEmptyParts);

    //founding suitable position for inserting TATWEEL or SPACE characters.
    for (int i = 0; i < charsOfText.size(); ++i) {
        if (i > 0 && i < charsOfText.size() - 1 && charsOfText.at(i) == " ") {
            spacePositions << i;
            continue;
        }

        if (charsOfText.at(i).at(0).joining() != QChar::Dual) {
            continue;
        }

        if (i < charsOfText.size() - 1 && ((charsOfText.at(i + 1).at(0).joining() == QChar::Dual) ||
                                           (charsOfText.at(i + 1).at(0).joining() == QChar::Right))) {
            tatweelPositions << i;
            continue;
        }
    }

    if (tatweelPositions.isEmpty()) {
        numOfTatweels = 0;
        numOfSpaces = (width - textWidth) / spaceWidth;
    }

    if (numOfTatweels > 0) {
        ++numOfTatweels;
        width = numOfSpaces * spaceWidth + numOfTatweels * tatweelWidth + textWidth;
    }
    else {
        ++numOfSpaces;
        width = numOfSpaces * spaceWidth + numOfTatweels * tatweelWidth + textWidth;
    }

    for (int i = 0; i < tatweelPositions.size(); ++i) {
        if (numOfTatweels <= 0) {
            break;
        }
        charsOfText[tatweelPositions.at(i)] = charsOfText.at(tatweelPositions.at(i)) + tatweel;
        --numOfTatweels;
        if (i == tatweelPositions.size() - 1) {
            i = -1;
        }
    }
    for (int i = 0; i < spacePositions.size(); ++i) {
        if (numOfSpaces <= 0) {
            break;
        }
        charsOfText[spacePositions.at(i)] = charsOfText.at(spacePositions.at(i)) + " ";
        --numOfSpaces;
        if (i == spacePositions.size() - 1) {
            i = -1;
        }
    }
    return charsOfText.join("");
}

QString QGanjoorDbBrowser::snippedText(const QString &text, const QString &str, int from, int maxNumOfWords, bool elided, Qt::TextElideMode elideMode)
{
    if (!str.isEmpty() && !text.contains(str)) {
        return "";
    }

    QString elideString = "";
    if (elided) {
        elideString = "...";
    }

    if (!str.isEmpty() && text.contains(str)) {
        int index = text.indexOf(str, from);
        if (index == -1) {
            return "";    //it's not possible
        }
        int partMaxNumOfWords = maxNumOfWords / 2;
        if (text.right(text.size() - str.size() - index).split(" ", QString::SkipEmptyParts).size() < partMaxNumOfWords) {
            partMaxNumOfWords = maxNumOfWords - text.right(text.size() - str.size() - index).split(" ", QString::SkipEmptyParts).size();
        }
        QString leftPart = QGanjoorDbBrowser::snippedText(text.left(index), "", 0, partMaxNumOfWords, elided, Qt::ElideLeft);
        //int rightPartMaxNumOfWords = maxNumOfWords/2;
        if (leftPart.split(" ", QString::SkipEmptyParts).size() < maxNumOfWords / 2) {
            partMaxNumOfWords = maxNumOfWords - leftPart.split(" ", QString::SkipEmptyParts).size();
        }
        QString rightPart = QGanjoorDbBrowser::snippedText(text.right(text.size() - str.size() - index), "", 0, partMaxNumOfWords, elided, Qt::ElideRight);
        /*if (index == text.size()-1)
            return elideString+leftPart+str+rightPart;
        else if (index == 0)
            return leftPart+str+rightPart+elideString;
        else
            return elideString+leftPart+str+rightPart+elideString;*/
        return leftPart + str + rightPart;
    }

    QStringList words = text.split(QRegExp("\\b"), QString::SkipEmptyParts);
    int textWordCount = words.size();
    if (textWordCount < maxNumOfWords || maxNumOfWords <= 0) {
        return text;
    }

    QStringList snippedList;
    int numOfInserted = 0;
    for (int i = 0; i < textWordCount; ++i) {
        if (numOfInserted >= maxNumOfWords) {
            break;
        }

        if (elideMode == Qt::ElideLeft) {
            //words = words.mid(words.size()-maxNumOfWords, maxNumOfWords);
            snippedList.prepend(words.at(textWordCount - 1 - i));
            if (words.at(i) != " ") {
                ++numOfInserted;
            }
        }
        else {
            snippedList << words.at(i);
            if (words.at(i) != " ") {
                ++numOfInserted;
            }
        }
    }
    /*if (elideMode == Qt::ElideLeft)
    {
        words = words.mid(words.size()-maxNumOfWords, maxNumOfWords);
    }
    else
        words = words.mid(0, maxNumOfWords);*/
    QString snippedString = snippedList.join("");
    if (snippedString == text) {
        return text;
    }
    if (elideMode == Qt::ElideLeft) {
        return elideString + snippedList.join("");
    }
    else {
        return snippedList.join("") + elideString;
    }
}

bool QGanjoorDbBrowser::isRadif(const QList<GanjoorVerse*> &verses, const QString &phrase, int verseOrder)
{
    //verseOrder starts from 1 to verses.size()
    if (verseOrder <= 0 || verseOrder > verses.size()) {
        return false;
    }

    //QList<GanjoorVerse *> verses = getVerses(PoemID);

    QString cleanedVerse = QGanjoorDbBrowser::cleanStringFast(verses.at(verseOrder - 1)->_Text, QStringList(""));
    //cleanedVerse = " "+cleanedVerse+" ";//just needed for whole word
    QString cleanedPhrase = QGanjoorDbBrowser::cleanStringFast(phrase, QStringList(""));

    if (!cleanedVerse.contains(cleanedPhrase)) {
        //verses.clear();
        return false;
    }

    QString secondMesra = "";
    QList<int> mesraOrdersForCompare;
    mesraOrdersForCompare  << -1 << -1 << -1;
    int secondMesraOrder = -1;

    switch (verses.at(verseOrder - 1)->_Position) {
    case Right:
    case CenteredVerse1:
        if (verseOrder == verses.size()) { //last single beyt! there is no 'CenteredVerse2' or a database error
            //verses.clear();
            return false;
        }
        if (verses.at(verseOrder)->_Position == Left || verses.at(verseOrder)->_Position == CenteredVerse2) {
            mesraOrdersForCompare[0] = verseOrder;
        }
        else {
            //verses.clear();
            return false;
        }//again database error
        break;
    case Left:
    case CenteredVerse2:
        if (verseOrder == 1) { //there is just one beyt!! more probably a database error
            //verses.clear();
            return false;
        }
        if (verses.at(verseOrder - 2)->_Position == Right || verses.at(verseOrder - 2)->_Position == CenteredVerse1) {
            mesraOrdersForCompare[0] = verseOrder - 2;
        }
        if (verseOrder - 3 >= 0 && (verses.at(verseOrder - 3)->_Position == Left || verses.at(verseOrder - 3)->_Position == CenteredVerse2)) {
            mesraOrdersForCompare[1] = verseOrder - 3;    //mesra above current mesra
        }
        if (verseOrder + 1 <= verses.size() - 1 && (verses.at(verseOrder + 1)->_Position == Left || verses.at(verseOrder + 1)->_Position == CenteredVerse2)) {
            mesraOrdersForCompare[2] = verseOrder + 1;    //mesra above current mesra
        }
        if (mesraOrdersForCompare.at(0) == -1 && mesraOrdersForCompare.at(1) == -1 && mesraOrdersForCompare.at(2) == -1) {
            //verses.clear();
            return false;
        }//again database error
        break;

    default:
        break;
    }

    for (int i = 0; i < 3; ++i) {
        secondMesraOrder = mesraOrdersForCompare.at(i);
        if (secondMesraOrder == -1) {
            continue;
        }
        secondMesra = QGanjoorDbBrowser::cleanStringFast(verses.at(secondMesraOrder)->_Text, QStringList(""));

        if (!secondMesra.contains(cleanedPhrase)) {
            //verses.clear();
            //return false;
            continue;
        }

        QString firstEnding = cleanedVerse.mid(cleanedVerse.lastIndexOf(cleanedPhrase) + cleanedPhrase.size());
        QString secondEnding = secondMesra.mid(secondMesra.lastIndexOf(cleanedPhrase) + cleanedPhrase.size());

        if (firstEnding != "" || secondEnding != "") { //they're not empty or RADIF
            continue;    //return false;
        }

        int tmp1 = cleanedVerse.lastIndexOf(cleanedPhrase) - 1;
        int tmp2 = secondMesra.lastIndexOf(cleanedPhrase) - 1;
        if (tmp1 < 0 || tmp2 < 0) {
            continue;    //return false;
        }

        return true;
    }
    return false;
}

bool QGanjoorDbBrowser::isRhyme(const QList<GanjoorVerse*> &verses, const QString &phrase, int verseOrder)
{
    //verseOrder starts from 1 to verses.size()
    if (verseOrder <= 0 || verseOrder > verses.size()) {
        return false;
    }

    QString cleanedVerse = QGanjoorDbBrowser::cleanStringFast(verses.at(verseOrder - 1)->_Text, QStringList(""));
    QString cleanedPhrase = QGanjoorDbBrowser::cleanStringFast(phrase, QStringList(""));

    if (!cleanedVerse.contains(cleanedPhrase)) {
        //verses.clear();
        return false;
    }

    QString secondMesra = "";
    QList<int> mesraOrdersForCompare;
    mesraOrdersForCompare  << -1 << -1 << -1;
    int secondMesraOrder = -1;

    switch (verses.at(verseOrder - 1)->_Position) {
    case Right:
    case CenteredVerse1:
        if (verseOrder == verses.size()) { //last single beyt! there is no 'CenteredVerse2' or a database error
            //verses.clear();
            return false;
        }
        if (verses.at(verseOrder)->_Position == Left || verses.at(verseOrder)->_Position == CenteredVerse2) {
            mesraOrdersForCompare[0] = verseOrder;
        }
        else {
            //verses.clear();
            return false;
        }//again database error
        break;
    case Left:
    case CenteredVerse2:
        if (verseOrder == 1) { //there is just one beyt!! more probably a database error
            //verses.clear();
            return false;
        }
        if (verses.at(verseOrder - 2)->_Position == Right || verses.at(verseOrder - 2)->_Position == CenteredVerse1) {
            mesraOrdersForCompare[0] = verseOrder - 2;
        }
        if (verseOrder - 3 >= 0 && (verses.at(verseOrder - 3)->_Position == Left || verses.at(verseOrder - 3)->_Position == CenteredVerse2)) {
            mesraOrdersForCompare[1] = verseOrder - 3;    //mesra above current mesra
        }
        if (verseOrder + 1 <= verses.size() - 1 && (verses.at(verseOrder + 1)->_Position == Left || verses.at(verseOrder + 1)->_Position == CenteredVerse2)) {
            mesraOrdersForCompare[2] = verseOrder + 1;    //mesra above current mesra
        }
        if (mesraOrdersForCompare.at(0) == -1 && mesraOrdersForCompare.at(1) == -1 && mesraOrdersForCompare.at(2) == -1) {
            //verses.clear();
            return false;
        }//again database error
        break;

    default:
        break;
    }

    for (int i = 0; i < 3; ++i) {
        secondMesraOrder = mesraOrdersForCompare.at(i);
        if (secondMesraOrder == -1) {
            continue;
        }
        secondMesra = QGanjoorDbBrowser::cleanStringFast(verses.at(secondMesraOrder)->_Text, QStringList(""));

        int indexInSecondMesra = secondMesra.lastIndexOf(cleanedPhrase);
        int offset = cleanedPhrase.size();
        while (indexInSecondMesra < 0) {
            --offset;
            if (offset < 1) {
                break;
            }
            indexInSecondMesra = secondMesra.lastIndexOf(cleanedPhrase.right(offset));
        }
        if (indexInSecondMesra < 0) {
            continue;
        }

        QString firstEnding = cleanedVerse.mid(cleanedVerse.lastIndexOf(cleanedPhrase) + cleanedPhrase.size());
        QString secondEnding = secondMesra.mid(indexInSecondMesra + cleanedPhrase.right(offset).size()); //secondMesra.mid(secondMesra.lastIndexOf(cleanedPhrase)+cleanedPhrase.size());

        if (firstEnding != secondEnding) { //they're not empty or RADIF
            continue;    //return false;
        }

        int tmp1 = cleanedVerse.lastIndexOf(cleanedPhrase) - 1;
        int tmp2 = indexInSecondMesra - 1;
        if (tmp1 < 0 || tmp2 < 0) {
            continue;    //return false;
        }

        if (cleanedVerse.at(tmp1) == secondMesra.at(tmp2) &&
                firstEnding.isEmpty()) { // and so secondEnding
            //if last character before phrase are similar this is not a Rhyme maybe its RADIF
            //or a part!!! of Rhyme.
            //the following algorithm works good for Rhyme with similar spell and diffrent meanings.
            continue;//return false;
        }
        else {
            return true;
        }
    }
    return false;
}

QVariantList QGanjoorDbBrowser::importGanjoorBookmarks()
{
    QVariantList lst;
    if (isConnected()) {
        QString selectQuery = QString("SELECT poem_id, verse_id FROM fav");
        QSqlQuery q(dBConnection);
        q.exec(selectQuery);
        QSqlRecord qrec;
        while (q.next()) {
            qrec = q.record();
            int poemID = qrec.value(0).toInt();
            int verseID = qrec.value(1).toInt();
            QString comment = QObject::tr("From Desktop Ganjoor");
            GanjoorPoem poem = getPoem(poemID);
            if (verseID == -1) {
                verseID = 1;
            }
            QString verseText = getBeyt(poemID, verseID, "[newline]").simplified();
            verseText.replace("[newline]", "\n");
            verseText.prepend(poem._Title + "\n");
            QStringList data;
            data << QString::number(poemID) << QString::number(verseID) << verseText << poem._Url + "/#" + QString::number(verseID) << comment;
            lst << data;
        }
    }
    return lst;
}

QString QGanjoorDbBrowser::getBeyt(int poemID, int firstMesraID, const QString &separator)
{
    QStringList mesras;
    if (isConnected()) {
        QSqlQuery q(dBConnection);
        QString selectQuery = QString("SELECT vorder, position, text FROM verse WHERE poem_id = %1 and vorder >= %2 ORDER BY vorder LIMIT 2").arg(poemID).arg(firstMesraID);
        q.exec(selectQuery);
        QSqlRecord qrec;
        bool centeredVerse1 = false;
        while (q.next()) {
            qrec = q.record();
            VersePosition mesraPosition = VersePosition(qrec.value(1).toInt());
            if (!centeredVerse1) {
                mesras << qrec.value(2).toString();
            }
            bool breakLoop = false;

            switch (mesraPosition) {
            case Single:
            case Paragraph:
                breakLoop = true;
                break;
            case Right:
                break;
            case Left:
                breakLoop = true;
                break;
            case CenteredVerse1:
                centeredVerse1 = true;
                break;
            case CenteredVerse2:
                if (centeredVerse1) {
                    mesras << qrec.value(2).toString();
                }
                breakLoop = true;
                break;
            default:
                break;
            }
            if (breakLoop) {
                break;
            }
        }
    }
    if (!mesras.isEmpty()) {
        return mesras.join(separator);
    }
    return "";
}

bool QGanjoorDbBrowser::poetHasSubCats(int poetID, const QString &connectionID)
{
    if (isConnected(connectionID)) {
        QSqlDatabase dataBaseObject = dBConnection;
        if (!connectionID.isEmpty()) {
            dataBaseObject = QSqlDatabase::database(connectionID);
        }
        QSqlQuery q(dataBaseObject);
        q.exec("SELECT id, text FROM cat WHERE poet_id = " + QString::number(poetID));

        q.first();
        if (q.isValid() && q.isActive()) {
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}

QList<QTreeWidgetItem*> QGanjoorDbBrowser::loadOutlineFromDataBase(int parentID)
{
    QList<QTreeWidgetItem*> items;
    QList<GanjoorCat*> parents = getSubCategories(parentID);
    if (parents.isEmpty()) {
        return items;
    }
    for (int i = 0; i < parents.size(); ++i) {
        QTreeWidgetItem* item = new QTreeWidgetItem(QStringList() << parents.at(i)->_Text);
        item->setToolTip(0, parents.at(i)->_Text);
        item->setData(0, Qt::UserRole, parents.at(i)->_ID);
        QList<QTreeWidgetItem*> children = loadOutlineFromDataBase(parents.at(i)->_ID);
        if (!children.isEmpty()) {
            item->addChildren(children);
        }
        items << item;
    }
    return items;
}

void QGanjoorDbBrowser::addDataSets()
{
    if (!QGanjoorDbBrowser::dbUpdater) {
        QGanjoorDbBrowser::dbUpdater = new DataBaseUpdater(0, Qt::WindowStaysOnTopHint);
    }

    if (!m_addRemoteDataSet) {
        //select data sets existing
        QStringList fileList = QFileDialog::getOpenFileNames(0, tr("Select data sets to install"), QDir::homePath(), "Supported Files (*.gdb *.s3db *.zip);;Ganjoor DataBase (*.gdb *.s3db);;Compressed Data Sets (*.zip);;All Files (*.*)");
        if (!fileList.isEmpty()) {
            foreach (const QString &file, fileList) {
                QGanjoorDbBrowser::dbUpdater->installItemToDB(file);
                QApplication::processEvents();
            }
        }
    }
    else if (m_addRemoteDataSet) {
        //download dialog
        QtWin::easyBlurUnBlur(QGanjoorDbBrowser::dbUpdater, Settings::READ("UseTransparecy").toBool());
        QGanjoorDbBrowser::dbUpdater->exec();
    }
}

QString QGanjoorDbBrowser::simpleCleanString(const QString &text)
{
    QString simpleCleaned = text.simplified();
    QChar tatweel = QChar(0x0640);
    simpleCleaned.remove(tatweel);

    return simpleCleaned;
}

bool QGanjoorDbBrowser::createEmptyDataBase(const QString &connectionID)
{
    if (isConnected(connectionID)) {
        QSqlDatabase dataBaseObject = dBConnection;
        if (!connectionID.isEmpty()) {
            dataBaseObject = QSqlDatabase::database(connectionID);
        }
        qDebug() << "tabels==" << dataBaseObject.tables(QSql::Tables);
        if (!dataBaseObject.tables(QSql::Tables).isEmpty()) {
            qWarning() << "The Data Base is NOT empty: " << dataBaseObject.databaseName();
            return false;
        }
        dataBaseObject.transaction();
        QSqlQuery q(dataBaseObject);
        q.exec("CREATE TABLE [cat] ([id] INTEGER  PRIMARY KEY NOT NULL,[poet_id] INTEGER  NULL,[text] NVARCHAR(100)  NULL,[parent_id] INTEGER  NULL,[url] NVARCHAR(255)  NULL);");
        q.exec("CREATE TABLE poem (id INTEGER PRIMARY KEY, cat_id INTEGER, title NVARCHAR(255), url NVARCHAR(255));");
        q.exec("CREATE TABLE [poet] ([id] INTEGER  PRIMARY KEY NOT NULL,[name] NVARCHAR(20)  NULL,[cat_id] INTEGER  NULL  NULL, [description] TEXT);");
        q.exec("CREATE TABLE [verse] ([poem_id] INTEGER  NULL,[vorder] INTEGER  NULL,[position] INTEGER  NULL,[text] TEXT  NULL);");
        q.exec("CREATE INDEX cat_pid ON cat(parent_id ASC);");
        q.exec("CREATE INDEX poem_cid ON poem(cat_id ASC);");
        q.exec("CREATE INDEX verse_pid ON verse(poem_id ASC);");
        return dataBaseObject.commit();
    }
    return false;
}

#ifdef Q_OS_WIN
#define BUFSIZE 4096
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <tchar.h>
#endif
/*static*/
QString QGanjoorDbBrowser::getLongPathName(const QString &fileName)
{
#ifndef Q_OS_WIN
    QFileInfo file(fileName);
    if (!file.exists()) {
        return fileName;
    }
    else {
        return file.canonicalFilePath();
    }
#else
    DWORD  retval = 0;
#ifdef D_MSVC_CC
    TCHAR  bufLongFileName[BUFSIZE] = TEXT("");
    TCHAR  bufFileName[BUFSIZE] = TEXT("");
#else
    TCHAR  bufLongFileName[BUFSIZE] = TEXT(L"");
    TCHAR  bufFileName[BUFSIZE] = TEXT(L"");
#endif

    QString winFileName = fileName;
    winFileName.replace("/", "\\");
    winFileName.toWCharArray(bufFileName);

    retval = GetLongPathName(bufFileName, bufLongFileName, BUFSIZE);

    if (retval == 0) {
        qWarning("GetLongPathName failed (%d)\n", GetLastError());
        QFileInfo file(fileName);
        if (!file.exists()) {
            return fileName;
        }
        else {
            return file.canonicalFilePath();
        }
    }
    else {
        QString longFileName = QString::fromWCharArray(bufLongFileName);
        longFileName.replace("\\", "/");

        return longFileName;
    }
#endif
}
