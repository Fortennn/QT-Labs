#include "SettingsDialog.h"

#include <QCoreApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("JARVIS Settings");
    setModal(true);
    resize(520, 260);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    auto* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignTop);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(10);

    auto* modelRow = new QWidget(this);
    auto* modelRowLayout = new QHBoxLayout(modelRow);
    modelRowLayout->setContentsMargins(0, 0, 0, 0);
    modelRowLayout->setSpacing(8);

    m_modelCombo = new QComboBox(this);
    m_modelCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* browseButton = new QPushButton("Browse...", this);
    browseButton->setCursor(Qt::PointingHandCursor);

    modelRowLayout->addWidget(m_modelCombo, 1);
    modelRowLayout->addWidget(browseButton);

    form->addRow("Model", modelRow);

    m_temperatureSpin = new QDoubleSpinBox(this);
    m_temperatureSpin->setRange(0.0, 2.0);
    m_temperatureSpin->setSingleStep(0.05);
    m_temperatureSpin->setDecimals(2);
    m_temperatureSpin->setValue(0.80);
    form->addRow("Temperature", m_temperatureSpin);

    m_contextSpin = new QSpinBox(this);
    m_contextSpin->setRange(512, 16384);
    m_contextSpin->setSingleStep(256);
    m_contextSpin->setValue(2048);
    form->addRow("Context Size", m_contextSpin);

    root->addLayout(form);

    auto* hint = new QLabel("Choose a GGUF model and runtime parameters. The model will be reloaded after applying settings.", this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color: #9aa4b2;");
    root->addWidget(hint);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(browseButton, &QPushButton::clicked, this, [this]() {
        const QString file = QFileDialog::getOpenFileName(this, "Select GGUF model", QString(), "GGUF Models (*.gguf)");
        if (file.isEmpty()) {
            return;
        }

        const int existing = m_modelCombo->findData(file);
        if (existing >= 0) {
            m_modelCombo->setCurrentIndex(existing);
            return;
        }

        m_modelCombo->addItem(QFileInfo(file).fileName(), file);
        m_modelCombo->setCurrentIndex(m_modelCombo->count() - 1);
    });

    setStyleSheet(
        "QDialog { background-color: #0b0f14; color: #e6edf3; }"
        "QLabel { color: #d1d7df; }"
        "QComboBox, QDoubleSpinBox, QSpinBox {"
        "  background-color: #111720;"
        "  color: #e6edf3;"
        "  border: 1px solid #2a3442;"
        "  border-radius: 8px;"
        "  padding: 6px 8px;"
        "}"
        "QPushButton {"
        "  background-color: #1a2230;"
        "  color: #e6edf3;"
        "  border: 1px solid #2f3d52;"
        "  border-radius: 8px;"
        "  padding: 7px 12px;"
        "}"
        "QPushButton:hover { background-color: #233046; }"
    );

    populateModelList();
    selectBestDefaultModel();
}

float SettingsDialog::getTemperature() const {
    return static_cast<float>(m_temperatureSpin->value());
}

int SettingsDialog::getContextSize() const {
    return m_contextSpin->value();
}

QString SettingsDialog::getSelectedModel() const {
    return m_modelCombo->currentData().toString();
}

void SettingsDialog::populateModelList() {
    QStringList roots;
    roots << QDir::current().absoluteFilePath("models")
          << QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("models");

    for (const QString& rootPath : roots) {
        QDir modelDir(rootPath);
        if (!modelDir.exists()) {
            continue;
        }

        const QStringList files = modelDir.entryList({"*.gguf"}, QDir::Files, QDir::Name);
        for (const QString& file : files) {
            const QString absolutePath = modelDir.absoluteFilePath(file);
            if (m_modelCombo->findData(absolutePath) < 0) {
                m_modelCombo->addItem(file, absolutePath);
            }
        }
    }

    if (m_modelCombo->count() == 0) {
        m_modelCombo->addItem("dolphin.gguf", QDir::current().absoluteFilePath("models/dolphin.gguf"));
    }
}

void SettingsDialog::selectBestDefaultModel() {
    int dolphinIndex = -1;

    for (int i = 0; i < m_modelCombo->count(); ++i) {
        const QString modelName = m_modelCombo->itemText(i).toLower();
        if (modelName.contains("dolphin")) {
            dolphinIndex = i;
            break;
        }
    }

    if (dolphinIndex >= 0) {
        m_modelCombo->setCurrentIndex(dolphinIndex);
    } else {
        m_modelCombo->setCurrentIndex(0);
    }
}
