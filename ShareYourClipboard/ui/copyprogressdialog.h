#ifndef COPYPROGRESSDIALOG_H
#define COPYPROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
class CopyProgressDialog;
}

class CopyProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CopyProgressDialog(QWidget *parent = 0);
    ~CopyProgressDialog();

private:
    Ui::CopyProgressDialog *ui;
protected slots:
    void onCancelPress();
signals:
    void cancel();
public slots:
    void start(QString mainOperationName);
    void stop();
    void showMessage(QString message);
    void setProgressMain(QString text, int value, int max);
    void setProgressSecond(QString text, int value, int max);
};

#endif // COPYPROGRESSDIALOG_H
