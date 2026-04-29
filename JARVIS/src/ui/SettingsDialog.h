#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QString>

class QDoubleSpinBox;
class QSpinBox;
class QLineEdit;
class QPushButton;

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr,
                            float currentTemperature = 0.8f,
                            int currentContextSize = 2048,
                            const QString& currentModelPath = QString());
    ~SettingsDialog() override = default;

    float   getTemperature()  const;
    int     getContextSize()  const;
    QString getSelectedModel() const;

private slots:
    void onBrowseModel();

private:
    QDoubleSpinBox* m_tempSpin   = nullptr;
    QSpinBox*       m_ctxSpin    = nullptr;
    QLineEdit*      m_modelPath  = nullptr;
    QPushButton*    m_browseBtn  = nullptr;
};

#endif // SETTINGS_DIALOG_H
