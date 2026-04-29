#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QColor>
#include <QDialog>
#include <QString>

#include "../ai/LlamaWorkerThread.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QSlider;
QT_END_NAMESPACE

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    // ---- LLM-side params ----
    LlamaWorkerThread::GenParams getGenParams() const;
    QString                       getSystemPromptOverride() const;

    float    getTemperature()   const;
    int      getContextSize()   const;
    QString  getSelectedModel() const;

    // ---- UI-only params ----
    QColor   getAccentColor()   const;
    double   getWindowOpacity() const;
    bool     showTimestamps()   const;
    QString  getUserName()      const;

    // Persist current values via QSettings. Called automatically on accept().
    void save() const;

    // Load from QSettings into the controls. If a key is missing the field
    // keeps its declared default. Called from the constructor.
    void load();

protected:
    // Paints the aurora background (radial gradient + accent blob + vignette)
    // matching the main window's atmosphere.
    void paintEvent(QPaintEvent* e) override;

private:
    void populateModelList();
    void selectBestDefaultModel();
    void resetToDefaults();
    void buildModelTab(QWidget* tab);
    void buildSamplingTab(QWidget* tab);
    void buildPersonaTab(QWidget* tab);
    void buildInterfaceTab(QWidget* tab);

    // Sampling
    QComboBox*       m_modelCombo            = nullptr;
    QDoubleSpinBox*  m_temperatureSpin       = nullptr;
    QSpinBox*        m_contextSpin           = nullptr;
    QDoubleSpinBox*  m_topPSpin              = nullptr;
    QSpinBox*        m_topKSpin              = nullptr;
    QDoubleSpinBox*  m_minPSpin              = nullptr;
    QDoubleSpinBox*  m_repeatPenaltySpin     = nullptr;
    QSpinBox*        m_maxTokensSpin         = nullptr;

    // Persona
    QPlainTextEdit*  m_systemPromptEdit      = nullptr;
    QLineEdit*       m_userNameEdit          = nullptr;

    // Interface
    QComboBox*       m_accentCombo           = nullptr;
    QSlider*         m_opacitySlider         = nullptr;
    QCheckBox*       m_timestampsCheck       = nullptr;
};

#endif // SETTINGSDIALOG_H
