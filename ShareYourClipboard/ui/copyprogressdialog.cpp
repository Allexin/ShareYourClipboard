#include "copyprogressdialog.h"
#include "ui_copyprogressdialog.h"
#include <QMessageBox>

CopyProgressDialog::CopyProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CopyProgressDialog)
{
    ui->setupUi(this);
}

CopyProgressDialog::~CopyProgressDialog()
{
    delete ui;
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
    QMessageBox::information(this,"",message);
}

void CopyProgressDialog::stop()
{
    hide();
}
