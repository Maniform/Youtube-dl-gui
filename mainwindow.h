#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QDir>
#include <QFileDialog>
#include <QProcess>
#include <QList>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#ifdef _WIN32
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    enum Mode{
        Nothing,
        Downloading,
        ShowingSubtitles,
        ShowingVideoFormats
    };

public slots:
    void openDestinationFolder();
    void outputRead();
    void errorRead();
    void startProcess();
    void processFinished(int exitCode);
    void errorOccured();
    void stopProcess();
    void optionChecked(int checked);
    void optionUnchecked(bool checked);
    void listSubtitles();
    void listVideoFormats();
    void subsChecked(int checked);
    void audioOnlyChecked(int checked);
    void videoFormatChecked(int checked);
    void playlistChecked(int checked);
    void logChecked(int checked);

private slots:
    void on_actionA_Propos_de_Youtube_dl_triggered();

private:
    Ui::MainWindow *ui;
    int nbVideosToDownload; //Nombre de vidéos à télécharger
    QString outputText; //Texte de sortie du programme
    QString programPath; //Localisation du fichier youtube-dl (qui n'est autre que youtube-dl.exe mais renommé pour ne pas le confondre avec l'exécutable de ce programme)
    bool receivingIdOrTitle; //Utile pour la correction des titres, car youtube-dl ajoute un ID à la fin du nom des fichiers
    Mode mode;
    bool gettingFileName;
    QList<QWidget*> widgetsToDisableOnRun;

    QStringList id;
    QStringList title;
    QStringList correctedTitle;

    QProcess process;
    QStringList arguments;

    //Instances nécessaires pour l'affichage de la barre de progression dans l'icône de la barre des tâches
#ifdef _WIN32
    QWinTaskbarButton *taskButton;
    QWinTaskbarProgress *taskProgressBar;
#endif

    void writeOutput(QString text);
    void showMessageBox(QString, QString);
    void setWidgetsEnabled(bool enabled);
    void getFileNames(QString& msg);
    void changeActualDownloadingVideo(QString& msg);
    void setProgressValue(QString& msg);
};

#endif // MAINWINDOW_H
