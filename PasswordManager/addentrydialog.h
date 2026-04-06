#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "passwordentry.h"

class AddEntryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEntryDialog(QWidget *parent = nullptr);

    void setEditingEntry(const PasswordEntry &entry);
    PasswordEntry getEntry() const;

private:
    void setupUI();

    QLineEdit   *m_titleEdit;
    QLineEdit   *m_usernameEdit;
    QLineEdit   *m_passwordEdit;
    QLineEdit   *m_websiteEdit;
    QComboBox   *m_categoryCombo;
    QPushButton *m_saveButton;
    QPushButton *m_cancelButton;
    QPushButton *m_togglePasswordButton;

    int m_entryId = 0;
};
