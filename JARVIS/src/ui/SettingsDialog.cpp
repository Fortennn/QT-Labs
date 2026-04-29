#include "SettingsDialog.h"

#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>

SettingsDialog::SettingsDialog(QWidget* parent,
                               float currentTemperature,
                               int currentContextSize,
                               const QString& currentModelPath)
    : QDialog(parent)
{
    setWindowTitle(tr("JARVIS — Settings"));
    setModal(true);
    setMinimumWidth(520);

    // ---------- Premium dark stylesheet (matches MainWindow) ----------
    setStyleSheet(R"(
        QDialog {
            background-color: #0d1117;
            color: #e6edf3;
            font-family: 'Segoe UI', 'Inter', 'SF Pro Display', sans-serif;
        }
        QLabel { color: #c9d1d9; font-size: 13px; }
        QLineEdit, QSpinBox, QDoubleSpinBox {
            background-color: #161b22;
            color: #e6edf3;
            border: 1px solid #30363d;
            border-radius: 8px;
            padding: 8px 10px;
            selection-background-color: #1f6feb;
            font-size: 13px;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 1px solid #58a6ff;
        }
        QPushButton {
            background-color: #21262d;
            color: #e6edf3;
            border: 1px solid #30363d;
            border-radius: 8px;
            padding: 8px 18px;
            font-weight: 600;
            font-size: 12px;
            letter-spacing: 1px;
        }
        QPushButton:hover {
            border-color: #58a6ff;
            background-color: #2d333b;
            color: #ffffff;
        }
        QPushButton:default {
            background-color: #1f6feb;
            border-color: #1f6feb;
            color: #ffffff;
        }
        QPushButton:default:hover { background-color: #388bfd; border-color: #388bfd; }
    )");

    // ---------- Widgets ----------
    auto* title = new QLabel(tr("MODEL & GENERATION"), this);
    title->setStyleSheet("color: #58a6ff; font-weight: 700; letter-spacing: 2px; font-size: 11px;");

    m_modelPath = new QLineEdit(this);
    m_modelPath->setPlaceholderText(tr("Select a .gguf model file…"));
    m_modelPath->setText(currentModelPath);

    m_browseBtn = new QPushButton(tr("Browse…"), this);
    connect(m_browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseModel);

    m_tempSpin = new QDoubleSpinBox(this);
    m_tempSpin->setRange(0.05, 2.0);
    m_tempSpin->setSingleStep(0.05);
    m_tempSpin->setDecimals(2);
    m_tempSpin->setValue(static_cast<double>(currentTemperature));

    m_ctxSpin = new QSpinBox(this);
    m_ctxSpin->setRange(512, 32768);
    m_ctxSpin->setSingleStep(256);
    m_ctxSpin->setValue(currentContextSize);

    // ---------- Layout ----------
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(10);

    int row = 0;
    grid->addWidget(new QLabel(tr("Model file:")), row, 0);
    auto* pathRow = new QHBoxLayout;
    pathRow->setSpacing(8);
    pathRow->addWidget(m_modelPath, 1);
    pathRow->addWidget(m_browseBtn);
    grid->addLayout(pathRow, row, 1);
    ++row;

    grid->addWidget(new QLabel(tr("Temperature:")), row, 0);
    grid->addWidget(m_tempSpin, row, 1);
    ++row;

    grid->addWidget(new QLabel(tr("Context size (n_ctx):")), row, 0);
    grid->addWidget(m_ctxSpin, row, 1);
    ++row;

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(16);
    root->addWidget(title);
    root->addLayout(grid);
    root->addStretch();
    root->addWidget(buttons);
}

void SettingsDialog::onBrowseModel() {
    QString startDir;
    if (!m_modelPath->text().isEmpty()) {
        startDir = QFileInfo(m_modelPath->text()).absolutePath();
    }
    const QString file = QFileDialog::getOpenFileName(
        this, tr("Select GGUF model"), startDir, tr("GGUF models (*.gguf);;All files (*)"));
    if (!file.isEmpty()) {
        m_modelPath->setText(file);
    }
}

float SettingsDialog::getTemperature() const {
    return static_cast<float>(m_tempSpin->value());
}

int SettingsDialog::getContextSize() const {
    return m_ctxSpin->value();
}

QString SettingsDialog::getSelectedModel() const {
    return m_modelPath->text().trimmed();
}
