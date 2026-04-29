#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QComboBox;
class QDoubleSpinBox;
class QSpinBox;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    float getTemperature() const;
    int getContextSize() const;
    QString getSelectedModel() const;

private:
    void populateModelList();
    void selectBestDefaultModel();

    QComboBox* m_modelCombo = nullptr;
    QDoubleSpinBox* m_temperatureSpin = nullptr;
    QSpinBox* m_contextSpin = nullptr;
};

#endif // SETTINGSDIALOG_H
