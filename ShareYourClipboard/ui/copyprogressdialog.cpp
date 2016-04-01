#include "copyprogressdialog.h"
#include "ui_copyprogressdialog.h"
#include <QMessageBox>

CopyProgressDialog::CopyProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CopyProgressDialog)
{
    ui->setupUi(this);
    connect(ui->pushButtonCancel,SIGNAL(released()),this, SLOT(onCancelPress()));
}

CopyProgressDialog::~CopyProgressDialog()
{
    delete ui;
}

void CopyProgressDialog::onCancelPress()
{
    emit cancel();
}

void CopyProgressDialog::start(QString mainOperationName)
{
    ui->progressBarMain->setValue(0);
    ui->progressBarMain->setMaximum(1);
    ui->labelMain->setText(mainOperationName);

    ui->progressBarSecond->setValue(0);
    ui->progressBarSecond->setMaximum(1);
    ui->labelSecond->setText("");

    showNormal();
    raise();
    activateWindow();
}

void CopyProgressDialog::showMessage(QString message)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("");
    msgBox.setText(message);
    msgBox.showNormal();
    msgBox.raise();
    msgBox.activateWindow();
    msgBox.exec();
}

void CopyProgressDialog::setProgressMain(QString text, int value, int max)
{
    ui->progressBarMain->setMaximum(max);
    ui->progressBarMain->setValue(value);
    ui->labelMain->setText(text);
}

void CopyProgressDialog::setProgressSecond(QString text, int value, int max)
{
    ui->progressBarSecond->setMaximum(max);
    ui->progressBarSecond->setValue(value);
    ui->labelSecond->setText(text);
}

void CopyProgressDialog::stop()
{
    hide();
}
