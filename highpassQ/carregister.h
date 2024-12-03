#ifndef CARREGISTER_H
#define CARREGISTER_H

#include <QWidget>

namespace Ui {
class carRegister;
}

class carRegister : public QWidget
{
    Q_OBJECT

public:
    explicit carRegister(QWidget *parent = nullptr);
    ~carRegister();

private:
    Ui::carRegister *ui;
};

#endif // CARREGISTER_H
