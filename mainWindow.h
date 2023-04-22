#pragma once
#include <QThread>
#include <QDebug>
#include <QLabel>
#include <QObject>
#include <QVBoxLayout>
#include <QMainWindow>
#include <QMovie>
#include "ui_mainwindow.h"

#include "Racer.h"

namespace Ui { class mainWindow; }



namespace Values {
    inline const char* TrackNames[25]{ // Lookup Table
      "BTC","BEC",
      "BWR","HG","AMR","APC",
      "AQC","SC","BB",
      "SR","DR","ABY",
      "BC","GVG","FMR","INF",
      "MGS","SMR","ZC",
      "VEN","EXE","TGT",
      "M100","DD","SL"
    };

    inline const char* RacerNames[23]{ // Lookup Table
        "Anakin", "Teempto", "Sebulba", "Ratts", "Aldar", "Mawhonic", "Ark",
        "Wan", "Mars", "Ebe", "Dud", "Gasgano", "Clegg", "Elan", "Neva", "Bozzie",
        "Boles", "Ody", "Fud", "Ben", "Slide", "Toy", "Bullseye"
    };
}


class Worker : public QThread, public Racer  {
    Q_OBJECT
public:
    Worker(QObject *parent = nullptr) : QThread(parent) {}

signals:
    void workCompleted(int count);
    void newRace(bool f, bool r, bool p);
    void newFrame(const char* track, const char* pod, float timer, int nodes, int nodeCount = -1);
    void updateConnectionStatus(bool isConnected);

protected:
    void run() override {
        init();
        while (!isInterruptionRequested()) {
            update();
        }
    }

private:
    void updateUI(const char* flag) override {

        if (!strcmp(flag, "nextFrame")) {
            emit newFrame(Values::TrackNames[rec.track],Values::RacerNames[rec.racer],rec.getLastNode()->timer,rec.getSize());
        } else if (!strcmp(flag, "connection")) {
            emit updateConnectionStatus(isConnected());
        }
        else if (!strcmp(flag, "mode")) {
            emit newRace(getraceFlag(), getrecMode(), getplayMode());
        }
        
    }


};


class mainWindow : public QMainWindow {
    Q_OBJECT

public:
    mainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        connect(&gameThread, &Worker::newRace, this, &mainWindow::handleNewRace);
        connect(&gameThread, &Worker::newFrame, this, &mainWindow::handleNewFrame);
        connect(&gameThread, &Worker::updateConnectionStatus, this, &mainWindow::handleConnectionStatus);
        
        ui = new Ui::mainWindow;
        ui->setupUi(this);

        connect(ui->pushButton_6, &QPushButton::clicked, this, &mainWindow::toggleConsole);
        
        


        connect(ui->checkBox, &QCheckBox::clicked, this, [=]() { 
            if (ui->checkBox_2->isChecked()) {
                ui->checkBox_2->setChecked(false);
            }
            gameThread.setMode(ui->checkBox->isChecked(),ui->checkBox_2->isChecked());
        });
        connect(ui->checkBox_2, &QCheckBox::clicked, this, [=]() {
            if (ui->checkBox->isChecked()) {
                ui->checkBox->setChecked(false);
            }
            gameThread.setMode(ui->checkBox->isChecked(),ui->checkBox_2->isChecked());
        });
    }

    void start() {
        show();
        gameThread.start();
    }

    void stop() {
        gameThread.requestInterruption();
        gameThread.wait();
    }

public slots:

    void handleNewRace(bool f, bool r, bool p) {
        ui->checkBox->setEnabled(!f);
        ui->checkBox_2->setEnabled(!f);

        ui->label_7->setText(
            !f ? "Standby" : r ? "Recording" : (p? "Playback" : "Standby")
        );
    }

    void handleNewFrame(const char* track, const char* pod, float timer, int nodes, int nodeCount = -1) {
        ui->label->setText(QString("Track\n") + QString(track));
        ui->label_2->setText(QString("Pod\n") + QString(pod));
        ui->label_3->setText("Timer\n" + formatTimer(timer));
        nodeCount = (nodeCount==-1)? nodes : nodeCount;
        ui->label_4->setText("Nodes\n" + QString::number(nodes) + "/" + QString::number(nodeCount));
    }

    void handleConnectionStatus(bool isConnected) {
        QPalette palette = ui->label_5->palette();
        
        if(isConnected) { 
            ui->label_5->setText("connected"); 
            palette.setColor(QPalette::WindowText, Qt::blue);

            QPixmap pixmap(":/res/connected.png");
            ui->label_6->setMovie(nullptr);
            ui->label_6->setPixmap(pixmap);
        }
        else { 
            ui->label_5->setText("disconnected"); 
            palette.setColor(QPalette::WindowText, Qt::red);
            
            QMovie *movie = new QMovie(":/res/connectionlost.gif");
            ui->label_6->setPixmap(QPixmap());
            ui->label_6->setMovie(movie);
            movie->start();
        }
        ui->label_5->setPalette(palette); ui->label_5->update();
    }

    

private:
    Ui::mainWindow *ui;
    Worker gameThread;
    bool isConsoleVisible = true;
    
    QString formatTimer(float timer) {
        int minutes = static_cast<int>(timer / 60);
        int seconds = static_cast<int>(timer) % 60;
        int milliseconds = (timer - static_cast<int>(timer)) * 1000;

        return QString("%1:%2.%3")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(milliseconds, 3, 10, QChar('0'));
    }

    void toggleConsole() {
        HWND consoleWindow = GetConsoleWindow();
        if (consoleWindow != nullptr) {
            if (isConsoleVisible) { ShowWindow(consoleWindow, SW_HIDE); } 
            else { ShowWindow(consoleWindow, SW_SHOW); }
            isConsoleVisible = !isConsoleVisible;
        }
    }
};



