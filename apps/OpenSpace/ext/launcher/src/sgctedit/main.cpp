#include <QApplication>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QScreen>
#include <QTextBrowser>
#include <QWidget>
#include <string>

#include "include/display.h"
#include "include/filesupport.h"
#include "include/monitorbox.h"
#include "include/windowcontrol.h"
#include "include/orientation.h"


int main(int argc, char *argv[ ])
{
    QApplication app(argc, argv);
    QMainWindow win(nullptr);

    //Temporary code for monitor detection
    QList<QScreen*> screenList = app.screens();
    std::vector<QSize> monitorSizeList;
    for (size_t s = 0; s < std::min(screenList.length(), 2); ++s) {
        monitorSizeList.push_back({//QRect(
            screenList[s]->size().width(),
            screenList[s]->size().height(),
        });
    }
for (QScreen* s : screenList) {
    std::cout << "Monitor ";
    std::cout << s->size().width() << "x" << s->size().height();
    std::cout << ", " << s->availableGeometry().width() << "x";
    std::cout << s->availableGeometry().height() << " offset [";
    std::cout << s->availableGeometry().x() << ",";
    std::cout << s->availableGeometry().y() << "]";
    std::cout << std::endl;
}
    //std::cout << "Primary";
    //QScreen* screen = app.primaryScreen();
    //std::cout << screen->size().width() << "x" << screen->size().height() << std::endl;
    //QRect screenGeometry = screen->geometry();
    //End code for monitor detection

    Display* displayWidget = nullptr;
    Display* displayWidget2 = nullptr;
    QFrame* monitorBorderFrame = nullptr;
    Orientation* orientationWidget = nullptr;

    if (screenList.length() == 0) {
        std::cerr << "Error: Qt reports no screens available." << std::endl;
        return -1;
    }
    QVBoxLayout* layoutMainV = new QVBoxLayout();
    QHBoxLayout* layoutMainH = new QHBoxLayout();

    orientationWidget = new Orientation(layoutMainV);
    QWidget* mainWindow = new QWidget();
    mainWindow->setLayout(layoutMainV);
    win.setCentralWidget(mainWindow);

    displayWidget = new Display(&monitorSizeList[0]);
    layoutMainH->addWidget(displayWidget);

screenList.push_back(screenList[0]);
QSize* m2 = new QSize(1080, 1920);
monitorSizeList.push_back({m2->width(), m2->height()});

    if (screenList.size() > 1) {
        displayWidget2 = new Display(&monitorSizeList[1]);
        monitorBorderFrame = new QFrame();
        monitorBorderFrame->setFrameShape(QFrame::VLine);
        layoutMainH->addWidget(monitorBorderFrame);
        layoutMainH->addWidget(displayWidget2);
    }

    layoutMainV->addLayout(layoutMainH);
    FileSupport* fileSupportWidget = new FileSupport(layoutMainV);

    win.setWindowTitle("Window Details");
    win.show();
    app.exec();
    delete orientationWidget;
    delete displayWidget;
    if (displayWidget2) {
        delete displayWidget2;
    }
    if (monitorBorderFrame) {
        delete monitorBorderFrame;
    }
    delete layoutMainH;
    delete layoutMainV;
    delete mainWindow;
}
