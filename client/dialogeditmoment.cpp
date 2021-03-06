#include "dialogeditmoment.h"
#include "ui_dialogeditmoment.h"

#include <string>

DialogEditMoment::DialogEditMoment(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogEditMoment)
{
    ui->setupUi(this);
    connect(ui->btn_send, &QPushButton::clicked, this, &DialogEditMoment::slotSendMoment);
}

CppContent Input2Content(std::string in) {
    return CppContent(in.c_str(), in.c_str() + in.length());
}

void DialogEditMoment::slotSendMoment() {
    emit SendMoment(Input2Content(ui->content->toPlainText().toStdString()));
    ui->content->clear();
}

DialogEditMoment::~DialogEditMoment()
{
    delete ui;
}
