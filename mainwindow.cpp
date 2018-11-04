#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDate>

//#include <QDebug>


#define MAX_TEXT_SIZE 1000000
//sudo apt install python-pip
//pip install youtube-dl
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    nbVideosToDownload(0),
    programPath(QDir::currentPath()+"/"),
    mode(Nothing),
    gettingFileName(false),
    process(this)
{
    ui->setupUi(this);

#ifdef QDEBUG_H
    ui->lienLineEdit->setText("https://www.youtube.com/watch?v=Pj9h3XS9fFI&t=0s&list=PLlhHgl_9ES1gr6We_09CQmNEk_QMMtzJW&index=7");
#endif

    ui->destinationLineEdit->setText(QDir::homePath());
    ui->optionsCheckBox->hide();
    //Récupération des préférences
    QFile prefs(".prefs");
    if(prefs.open(QFile::ReadOnly))
    {
        QString dest = prefs.readLine();
        dest.remove('\n');
        ui->destinationLineEdit->setText(dest);
        if(bool(QString(prefs.readLine()).toInt()))
        {
            optionChecked(Qt::Checked);
        }
        else
        {
            optionUnchecked(false);
        }
        ui->logCheckBox->setChecked(bool(QString(prefs.readLine()).toInt()));
        ui->outputPlainTextEdit->setVisible(ui->logCheckBox->isChecked());
        ui->titleCorrectorCheckBox->setChecked(bool(QString(prefs.readLine()).toInt()));
        ui->actionAlertes->setChecked(!bool(QString(prefs.readLine()).toInt()));
        prefs.close();
    }
    /*else if(prefs.open(QFile::WriteOnly))
    {
        QTextStream out(&prefs);
        out << QDir::homePath() << endl << 0 << endl << 1 << endl << 1;
        prefs.close();
    }*/

    //Etablissement des connexions entre les objets
    connect(ui->openPushButton, SIGNAL(clicked()), this, SLOT(openDestinationFolder()));
    connect(ui->downloadPushButton, SIGNAL(clicked()), this, SLOT(startProcess()));
    connect(ui->stopPushButton, SIGNAL(clicked()), this, SLOT(stopProcess()));
    connect(&process, SIGNAL(readyReadStandardOutput()), this, SLOT(outputRead()));
    connect(&process, SIGNAL(readyReadStandardError()), this, SLOT(errorRead()));
    connect(&process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int)));
    connect(&process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(errorOccured()));
    connect(ui->optionsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(optionChecked(int)));
    connect(ui->optionsGroupBox, SIGNAL(clicked(bool)), this, SLOT(optionUnchecked(bool)));
    connect(ui->showSubsPushButton, SIGNAL(clicked()), this, SLOT(listSubtitles()));
    connect(ui->showVideoFormatsPushButton, SIGNAL(clicked()), this, SLOT(listVideoFormats()));
    connect(ui->subsCheckBox, SIGNAL(stateChanged(int)), this, SLOT(subsChecked(int)));
    connect(ui->audioOnlyCheckBox, SIGNAL(stateChanged(int)), this, SLOT(audioOnlyChecked(int)));
    connect(ui->videoFormatCheckBox, SIGNAL(stateChanged(int)), this, SLOT(videoFormatChecked(int)));
    connect(ui->logCheckBox, SIGNAL(stateChanged(int)), this, SLOT(logChecked(int)));
    connect(ui->lienLineEdit, SIGNAL(returnPressed()), this, SLOT(startProcess()));
    connect(ui->playlistCheckBox, SIGNAL(stateChanged(int)), this, SLOT(playlistChecked(int)));

    //Initialisation des objets à désactiver durant le téléchargement
    widgetsToDisableOnRun.append(ui->lienLineEdit);
    widgetsToDisableOnRun.append(ui->destinationLineEdit);
    widgetsToDisableOnRun.append(ui->openPushButton);
    widgetsToDisableOnRun.append(ui->playlistCheckBox);
    widgetsToDisableOnRun.append(ui->showSubsPushButton);
    widgetsToDisableOnRun.append(ui->showVideoFormatsPushButton);
    widgetsToDisableOnRun.append(ui->subsCheckBox);
    widgetsToDisableOnRun.append(ui->videoFormatCheckBox);
    widgetsToDisableOnRun.append(ui->autosubCheckBox);
    widgetsToDisableOnRun.append(ui->audioOnlyCheckBox);
    widgetsToDisableOnRun.append(ui->titleCorrectorCheckBox);

    //Résolution de l'affichage
    ui->progressBar->hide();
    ui->globalProgressBar->hide();
    ui->stopPushButton->hide();
    ui->downloadingLabel->hide();

    //Vérification de l'existence de 'youtube-dl'
    QDir dir;
#ifdef _WIN32
    if(!dir.exists("youtube-dl"))
    {
        showMessageBox("Attention !","Le fichier youtube-dl est inexistant dans le dossier du programme.");
        ui->downloadingLabel->setText("Attention : le fichier youtube-dl est inexistant dans le dossier du programme.");
        ui->downloadingLabel->show();
    }
#else
    if(!dir.exists(QDir::homePath() + "/.local/bin/youtube-dl"))
    {
        showMessageBox("Attention !","youtube-dl ne semple pas être installé sur cette machine.\nPour avoir la dernière version :\nsudo apt install python-pip\npip install youtube-dl");
        ui->downloadingLabel->setText("Attention : le programme youtube-dl ne semble pas être installé.");
        ui->downloadingLabel->show();
    }
    if(!dir.exists("/usr/bin/ffmpeg"))
    {
        showMessageBox("Attention !","ffmpeg ne semble pas être installé sur cette machine.\nPour l'installer :\nsudo apt install ffmpeg");
        ui->downloadingLabel->setText(ui->downloadingLabel->text().append("\nAttention : le programme ffmpeg ne semble pas être installé."));
        ui->downloadingLabel->show();
    }
#endif

    //Nécessaire pour afficher la barre de progression dans l'icône de la barre des tâches
#ifdef _WIN32
    taskButton = new QWinTaskbarButton(this);
#endif
    //button->setOverlayIcon(QIcon(":/loading.png"));
}

MainWindow::~MainWindow()
{
    process.kill();

    //Sauvegarde des préférences
    QDir::setCurrent(programPath);
    QFile prefs(".prefs");
    if(prefs.open(QFile::WriteOnly))
    {
        QTextStream out(&prefs);
        QString dest = ui->destinationLineEdit->text();
        dest.remove('\n');
        out << dest << endl << int(!ui->optionsGroupBox->isHidden()) << endl << int(ui->logCheckBox->isChecked()) << endl << int(ui->titleCorrectorCheckBox->isChecked()) << endl << int(!ui->actionAlertes->isChecked());
        prefs.close();
    }
    delete ui;
}

void MainWindow::writeOutput(QString text)
{
    outputText += text;
    if(outputText.length() > MAX_TEXT_SIZE)
    {
        outputText = outputText.right(MAX_TEXT_SIZE);
        ui->outputPlainTextEdit->clear();
#ifdef _WIN32
        if(mode == Downloading)
            ui->outputPlainTextEdit->appendHtml(outputText);
        else
            ui->outputPlainTextEdit->appendPlainText(outputText);
#else
        ui->outputPlainTextEdit->appendPlainText(outputText);
#endif
    }
    else
    {
#ifdef _WIN32
        if(mode == Downloading)
            ui->outputPlainTextEdit->appendHtml(text);
        else
            ui->outputPlainTextEdit->appendPlainText(text);
#else
        ui->outputPlainTextEdit->appendPlainText(text);
#endif
    }
}

void MainWindow::openDestinationFolder()
{
    QString dossier = QFileDialog::getExistingDirectory(this, "Ouvrir le dossier de destination", ui->destinationLineEdit->text());
    if(dossier != "")
    {
        ui->destinationLineEdit->setText(dossier);
    }
}

void MainWindow::outputRead()
{
    QString msg = process.readAllStandardOutput();
    writeOutput(msg);
    if(mode == Downloading)
    {
        if(gettingFileName)
        {
            getFileNames(msg);
        }
        else if(msg.contains("[download] Downloading video "))
        {
            changeActualDownloadingVideo(msg);
        }
        else if(msg.contains("[download] "))
        {
            setProgressValue(msg);
        }
        else if(msg.contains("because of --no-playlist"))
        {
            ui->globalProgressBar->setValue(1);
            ui->downloadingLabel->setText("Téléchargement de : " + correctedTitle[0]);
        }
    }
}

void MainWindow::errorRead()
{
    writeOutput(process.readAllStandardError());
}

void MainWindow::startProcess()
{
    if(ui->destinationLineEdit->text() != "" && ui->lienLineEdit->text() != "")
    {
        if(mode == Nothing)
            mode = Downloading;
        gettingFileName = !gettingFileName;
        if(gettingFileName)
        {
            ui->progressBar->show();
            ui->globalProgressBar->show();
            ui->downloadingLabel->show();
#ifdef _WIN32
            taskButton->setWindow(this->windowHandle());
            taskProgressBar = taskButton->progress();
            connect(ui->progressBar, SIGNAL(valueChanged(int)), taskProgressBar, SLOT(setValue(int)));
            taskProgressBar->setMaximum(100);
            taskProgressBar->show();
#endif
            ui->progressBar->setValue(0);
        }
        arguments.clear();
        if(!QDir::setCurrent(ui->destinationLineEdit->text()))
        {
            writeOutput("Le changement de dossier a échoué.");
        }
        arguments << ui->lienLineEdit->text();
#ifdef _WIN32
        if(ui->titleCorrectorCheckBox->isChecked())
            arguments << "--restrict-filenames";
#endif
        if(gettingFileName)
        {
            arguments << "--get-filename" << "--get-id";
            receivingIdOrTitle = false;
        }
        if(!ui->playlistCheckBox->isChecked())
        {
            arguments << "--no-playlist";
        }
        if(ui->subsCheckBox->isChecked())
        {
            if(ui->autosubCheckBox->isChecked())
                arguments << "--write-auto-sub";
            else
                arguments << "--write-sub";
            if(ui->subsLineEdit->text() != "")
            {
                arguments << "--sub-lang" << ui->subsLineEdit->text();
            }
        }
        if(ui->videoFormatCheckBox->isChecked())
        {
            arguments << "-f" << QString::number(ui->videoCodeSpinBox->value());
        }
        if(ui->audioOnlyCheckBox->isChecked())
        {
            arguments << "-x" << "--audio-format";
            if(ui->audioFormatLineEdit->text() != "")
                arguments << ui->audioFormatLineEdit->text();
            else
                arguments << "mp3";
        }
        if(!gettingFileName)
            writeOutput("Et c'est parti !");
        else
        {
            ui->globalProgressBar->setValue(0);
            ui->globalProgressBar->setMaximum(0);
            ui->downloadingLabel->setText("Récupération des titres de vidéo...");
        }
        setWidgetsEnabled(false);
        ui->subsLineEdit->setEnabled(false);
        ui->videoCodeSpinBox->setEnabled(false);
        ui->downloadPushButton->hide();
        ui->stopPushButton->show();
#ifdef _WIN32
        process.start(programPath + "youtube-dl",arguments);
#else
        process.start("youtube-dl",arguments);
#endif
    }
    else
    {
        showMessageBox("Attention !","Un champ est vide.");
    }
}

void MainWindow::stopProcess()
{
    writeOutput("Arrêt demandé.");
    process.kill();
    //processFinished(-1);
}

void MainWindow::processFinished(int exitCode)
{
    if(exitCode == 0)
    {
        if(gettingFileName)
        {
            startProcess();
        }
        else
        {
            ui->progressBar->setValue(100);
            if(ui->titleCorrectorCheckBox->isChecked())
            {
                for(int i = 0 ; i < title.length() ; i++)
                {
                    if(QFile::rename(QDir::currentPath() + "/" + title[i], correctedTitle[i]))
                        writeOutput("Nom du fichier corrigé.");
                    else
                    {
                        writeOutput("Erreur lors du renommage du fichier de sortie.");
                        writeOutput(title[i].toUtf8() + " -> " + correctedTitle[i].toUtf8());
                    }
                }
            }
            title.clear();
            correctedTitle.clear();
            writeOutput("Terminé avec succès.");
            setWidgetsEnabled(true);
            if(ui->subsCheckBox->isChecked())
                ui->subsLineEdit->setEnabled(true);
            if(ui->videoFormatCheckBox->isChecked())
                ui->videoCodeSpinBox->setEnabled(true);
            if(ui->audioOnlyCheckBox->isChecked())
                ui->audioFormatLineEdit->setEnabled(true);
            ui->stopPushButton->hide();
            ui->downloadPushButton->show();
#ifdef _WIN32
            if(mode == Downloading)
                taskProgressBar->hide();
#endif
            nbVideosToDownload = 0;
            ui->downloadingLabel->setText("Terminé !");
            mode = Nothing;
            QApplication::alert(this);
        }
    }
    else
    {
        gettingFileName = false;
        ui->progressBar->hide();
        ui->globalProgressBar->hide();
        ui->globalProgressBar->setMaximum(0);
        writeOutput("Le programme s'est arrêté suite à une erreur.");
        setWidgetsEnabled(true);
        if(ui->subsCheckBox->isChecked())
            ui->subsLineEdit->setEnabled(true);
        ui->stopPushButton->hide();
        ui->downloadPushButton->show();
#ifdef _WIN32
        taskProgressBar->hide();
#endif
        nbVideosToDownload = 0;
        ui->downloadingLabel->setText("Une erreur est survenue.");
        mode = Nothing;
        QApplication::alert(this);
    }
}

void MainWindow::errorOccured()
{
    writeOutput("Une erreur est survenue.");
    showMessageBox("Alerte !","Une erreur est survenue.");
}

void MainWindow::setWidgetsEnabled(bool enabled)
{
    for(int i = 0 ; i < widgetsToDisableOnRun.count() ; i++)
    {
        widgetsToDisableOnRun[i]->setEnabled(enabled);
    }
}

void MainWindow::optionChecked(int checked)
{
    if(checked == Qt::Checked)
    {
        ui->optionsCheckBox->hide();
        ui->optionsGroupBox->show();
        ui->optionsGroupBox->setChecked(true);
    }
}

void MainWindow::optionUnchecked(bool checked)
{
    if(!checked)
    {
        ui->optionsGroupBox->hide();
        ui->optionsCheckBox->show();
        ui->optionsCheckBox->setChecked(false);
    }
}

void MainWindow::listSubtitles()
{
    if(ui->lienLineEdit->text() != "")
    {
        arguments.clear();
        QDir::setCurrent(ui->destinationLineEdit->text());
        arguments << ui->lienLineEdit->text();
        arguments << "--list-subs";
        if(!ui->playlistCheckBox->isChecked())
            arguments << "--no-playlist";
        writeOutput("Et c'est parti !");
        setWidgetsEnabled(false);
        ui->subsLineEdit->setEnabled(false);
        ui->downloadPushButton->hide();
        ui->stopPushButton->show();
        ui->logCheckBox->setChecked(true);
        ui->videoCodeSpinBox->setEnabled(false);
        ui->audioFormatLineEdit->setEnabled(false);
        mode = ShowingSubtitles;
#ifdef _WIN32
        process.start(programPath + "youtube-dl",arguments);
#else
        process.start("youtube-dl",arguments);
#endif
    }
    else
    {
        showMessageBox("Attention !","Le lien Youtube est vide.");
    }
}

void MainWindow::listVideoFormats()
{
    if(ui->lienLineEdit->text() != "")
    {
        arguments.clear();
        QDir::setCurrent(ui->destinationLineEdit->text());
        arguments << ui->lienLineEdit->text();
        arguments << "-F";
        if(!ui->playlistCheckBox->isChecked())
            arguments << "--no-playlist";
        writeOutput("Et c'est parti !");
        setWidgetsEnabled(false);
        ui->videoFormatCheckBox->setEnabled(false);
        ui->downloadPushButton->hide();
        ui->stopPushButton->show();
        ui->logCheckBox->setChecked(true);
        ui->subsLineEdit->setEnabled(false);
        ui->videoCodeSpinBox->setEnabled(false);
        ui->audioFormatLineEdit->setEnabled(false);
        mode = ShowingVideoFormats;
#ifdef _WIN32
        process.start(programPath + "youtube-dl",arguments);
#else
        process.start("youtube-dl",arguments);
#endif
    }
    else
        showMessageBox("Attention !","Le lien Youtube est vide.");
}

void MainWindow::subsChecked(int checked)
{
    switch(checked)
    {
    case Qt::Checked:
        ui->audioOnlyCheckBox->setChecked(false);
        ui->audioFormatLineEdit->clear();
        ui->subsLineEdit->setEnabled(true);
        ui->autosubCheckBox->setEnabled(true);
        if(ui->playlistCheckBox->isChecked())
            showMessageBox("Attention !","Il faut que toutes les vidéos de la playlist possèdent les mêmes types de sous-titres (langue, générés automatiquement ou non).");
        break;

    case Qt::Unchecked:
        ui->subsLineEdit->setEnabled(false);
        ui->autosubCheckBox->setChecked(false);
        ui->autosubCheckBox->setEnabled(false);
        ui->audioOnlyCheckBox->setEnabled(true);
        break;
    }
}

void MainWindow::videoFormatChecked(int checked)
{
    switch(checked)
    {
    case Qt::Checked:
        ui->videoCodeSpinBox->setEnabled(true);
        ui->audioOnlyCheckBox->setChecked(false);
        if(ui->playlistCheckBox->isChecked())
            showMessageBox("Attention !","Toutes les vidéos doivent posséder le même code pour le format souhaité.");
        //ui->playlistCheckBox->setChecked(false);
        break;

    case Qt::Unchecked:
        ui->videoCodeSpinBox->setEnabled(false);
        break;
    }
}

void MainWindow::audioOnlyChecked(int checked)
{
    switch(checked)
    {
    case Qt::Checked:
        ui->subsCheckBox->setChecked(false);
        ui->audioFormatLineEdit->setEnabled(true);
        ui->videoFormatCheckBox->setChecked(false);
        break;

    case Qt::Unchecked:
        ui->audioFormatLineEdit->setEnabled(false);
        break;
    }
}

void MainWindow::logChecked(int checked)
{
    switch(checked)
    {
    case Qt::Checked:
        ui->outputPlainTextEdit->show();
        break;

    case Qt::Unchecked:
        ui->outputPlainTextEdit->hide();
        break;
    }
}

void MainWindow::playlistChecked(int checked)
{
    switch(checked)
    {
    case Qt::Checked:
        if(ui->subsCheckBox->isChecked())
            showMessageBox("Attention !","Il faut que toutes les vidéos de la playlist possèdent les mêmes types de sous-titres (langue, générés automatiquement ou non).");
        if(ui->videoFormatCheckBox->isChecked())
            showMessageBox("Attention !","Toutes les vidéos doivent posséder le même code pour le format souhaité.");
        //ui->videoFormatCheckBox->setChecked(false);
        //ui->subsCheckBox->setChecked(false);
        //ui->autosubCheckBox->setChecked(false);
        break;

    case Qt::Unchecked:
        break;
    }
}

void MainWindow::getFileNames(QString& msg)
{
    QStringList split = msg.split('\n');
    for(int i = 0 ; i < split.length() ; i++)
    {
        if(split[i] != "")
        {
            if(!receivingIdOrTitle)
                id << split[i];
            else
            {
                nbVideosToDownload++;
                ui->globalProgressBar->setMaximum(nbVideosToDownload);
#ifdef _WIN32
                if(nbVideosToDownload == 2)
                {
                    disconnect(ui->progressBar, SIGNAL(valueChanged(int)), taskProgressBar, SLOT(setValue(int)));
                    connect(ui->globalProgressBar, SIGNAL(valueChanged(int)), taskProgressBar, SLOT(setValue(int)));
                }
                if(nbVideosToDownload > 1)
                {
                    taskProgressBar->setMaximum(nbVideosToDownload);
                }
#endif
                if(ui->audioOnlyCheckBox->isChecked())
                {
                    split[i].remove(split[i].right(split[i].length()-split[i].lastIndexOf('.')));
                    if(ui->audioFormatLineEdit->text() == "")
                        split[i].append(".mp3");
                    else
                        split[i].append("."+ui->audioFormatLineEdit->text());
                }
                title << split[i];
                if(ui->titleCorrectorCheckBox->isChecked())
                {
                    split[i].remove("-" + id.last());
                    split[i].replace('_',' ');
                    split[i].replace("  "," ");
                }
                correctedTitle << split[i];
            }
            receivingIdOrTitle = !receivingIdOrTitle;
        }
    }
}

void MainWindow::changeActualDownloadingVideo(QString& msg)
{
    msg.remove(msg.left(msg.indexOf("[download] Downloading video ")));
    msg.remove("[download] Downloading video ");
    msg.remove('\n');
    QStringList l = msg.split(' ');
    ui->globalProgressBar->setValue(l[0].toInt());
    ui->downloadingLabel->setText("Téléchargement de : " + correctedTitle[l[0].toInt()-1]);
    ui->progressBar->setValue(0);
}

void MainWindow::setProgressValue(QString& msg)
{
    msg = msg.mid(msg.lastIndexOf("[download] ")+QString("[download] ").length(),6);
    msg.remove(' ');
    msg.remove('%');
    bool ok;
    float value = msg.toFloat(&ok);
    if(ok)
        ui->progressBar->setValue(int(value));
}

void MainWindow::showMessageBox(QString title, QString text)
{
    if(ui->actionAlertes->isChecked())
    {
        QApplication::beep();
        QMessageBox msg;
        msg.setIcon(QMessageBox::Warning);
        msg.setWindowIcon(this->windowIcon());
        msg.setWindowTitle(title);
        msg.setText(text);
        msg.exec();
    }
}

void MainWindow::on_actionA_Propos_de_Youtube_dl_triggered()
{
    const std::string Compilator_Version = std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__);
    const QString CPP_Info = QString::fromStdString(std::to_string(__cplusplus));

    const QString QCompilator_Version = "Compiler Version :" + QString::fromStdString(Compilator_Version);
    //QString sDate = QDateTime::currentDateTime().toString("dddd dd MMMM yyyy hh:mm:ss.zzz");
    const QString sDate = QDateTime::currentDateTime().toString("dd/MM/yyyy");
    QMessageBox msgBox;

    msgBox.setText("<b>Build informations</b>");
    const QString Message = QString("Build by: ") + QCompilator_Version + "\n" + "C++ version: " + CPP_Info + "\n" + "Build date :" + sDate;
    msgBox.setInformativeText(Message);
    msgBox.exec();
}
