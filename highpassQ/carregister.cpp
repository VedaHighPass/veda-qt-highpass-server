#include "carregister.h"
#include "ui_carregister.h"

carRegister::carRegister(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::carRegister)
{
    ui->setupUi(this);
}

carRegister::~carRegister()
{
    delete ui;
}
