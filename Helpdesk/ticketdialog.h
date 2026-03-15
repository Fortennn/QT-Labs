#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class TicketDialog; }
QT_END_NAMESPACE

class TicketDialog : public QDialog
{
    Q_OBJECT

public:
    enum Mode { ModeView, ModeEdit, ModeNew };

    explicit TicketDialog(QWidget *parent = nullptr);
    ~TicketDialog();

    void setMode(Mode mode);

private slots:
    void onEditClicked();
    void onSaveClicked();
    void onCancelClicked();

private:
    Ui::TicketDialog *ui;
    Mode m_mode;
    Mode m_previousMode;

    void applyMode();
    void setFieldsReadOnly(bool readOnly);
};
