#ifndef OSTIME_H
#define OSTIME_H

#include <QDialog>
#include <QWidget>
#include <optional>

QT_BEGIN_NAMESPACE
namespace Ui {
class time;
}
QT_END_NAMESPACE

class ostime : public QDialog
{
    Q_OBJECT

public slots:
    void enableAccordingToType(int);
    void cancel();
    void approved();
    void keyPressEvent(QKeyEvent *evt);

public:
    explicit ostime(openspace::Profile* imported, QWidget *parent = nullptr);
    ~ostime();
private:
    void enableFormatForAbsolute(bool enableAbs);
    Ui::time *ui;
    QWidget* _parent;
    openspace::Profile* _imported;
    openspace::Profile::Time _data;
    bool _initializedAsAbsolute = true;
};

#endif // OSTIME_H
