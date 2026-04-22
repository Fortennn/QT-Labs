#include "addentrydialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDateTime>
#include <QGraphicsDropShadowEffect>

AddEntryDialog::AddEntryDialog(QWidget *parent)
    : QDialog(parent)
{
    m_leakChecker = new PasswordLeakChecker(this);
    connect(m_leakChecker, &PasswordLeakChecker::checkCompleted, this, &AddEntryDialog::onLeakCheckCompleted);
    connect(m_leakChecker, &PasswordLeakChecker::checkError, this, &AddEntryDialog::onLeakCheckError);

    setupUI();
}

void AddEntryDialog::setupUI()
{
    setWindowTitle("Add New Password");
    setFixedSize(460, 420);
    setModal(true);

    // --- Main layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(28, 24, 28, 24);

    // --- Header ---
    QLabel *header = new QLabel("🔐  New Password Entry");
    header->setObjectName("dialogHeader");
    mainLayout->addWidget(header);

    // --- Separator ---
    QFrame *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("dialogSeparator");
    mainLayout->addWidget(sep);

    // --- Form ---
    QFormLayout *form = new QFormLayout;
    form->setSpacing(12);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_titleEdit = new QLineEdit;
    m_titleEdit->setPlaceholderText("e.g. My Gmail Account");
    m_titleEdit->setMinimumHeight(34);
    form->addRow("Title", m_titleEdit);

    m_usernameEdit = new QLineEdit;
    m_usernameEdit->setPlaceholderText("e.g. user@gmail.com");
    m_usernameEdit->setMinimumHeight(34);
    form->addRow("Username", m_usernameEdit);

    // Password field with toggle
    QHBoxLayout *passLayout = new QHBoxLayout;
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setPlaceholderText("Enter password");
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setMinimumHeight(34);

    m_togglePasswordButton = new QPushButton("👁");
    m_togglePasswordButton->setObjectName("togglePassBtn");
    m_togglePasswordButton->setFixedSize(60, 34);
    m_togglePasswordButton->setCheckable(true);
    m_togglePasswordButton->setCursor(Qt::PointingHandCursor);
    m_togglePasswordButton->setToolTip("Show / Hide password");
    connect(m_togglePasswordButton, &QPushButton::toggled, this, [this](bool checked) {
        m_passwordEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });

    passLayout->addWidget(m_passwordEdit);
    passLayout->addWidget(m_togglePasswordButton);
    
    m_checkLeakButton = new QPushButton("Check Security");
    m_checkLeakButton->setObjectName("checkLeakBtn");
    m_checkLeakButton->setMinimumHeight(34);
    connect(m_checkLeakButton, &QPushButton::clicked, this, &AddEntryDialog::onCheckLeakClicked);
    passLayout->addWidget(m_checkLeakButton);
    
    form->addRow("Password", passLayout);

    m_leakStatusLabel = new QLabel("");
    m_leakStatusLabel->setObjectName("leakStatusLabel");
    m_leakStatusLabel->setWordWrap(true);
    form->addRow("", m_leakStatusLabel);

    m_websiteEdit = new QLineEdit;
    m_websiteEdit->setPlaceholderText("e.g. https://gmail.com");
    m_websiteEdit->setMinimumHeight(34);
    form->addRow("Website", m_websiteEdit);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItems({"Email", "Social", "Banking", "Work", "Other"});
    m_categoryCombo->setMinimumHeight(34);
    form->addRow("Category", m_categoryCombo);

    mainLayout->addLayout(form);
    mainLayout->addStretch();

    // --- Buttons ---
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(12);

    m_cancelButton = new QPushButton("Cancel");
    m_cancelButton->setObjectName("cancelBtn");
    m_cancelButton->setMinimumHeight(36);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    m_saveButton = new QPushButton("✓  Save Entry");
    m_saveButton->setObjectName("saveBtn");
    m_saveButton->setMinimumHeight(36);
    m_saveButton->setDefault(true);
    connect(m_saveButton, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelButton);
    btnLayout->addWidget(m_saveButton);
    mainLayout->addLayout(btnLayout);

    // --- Dialog-specific styles ---
    setStyleSheet(R"(
        QDialog {
            background-color: #1E1E2E;
        }

        QLabel#dialogHeader {
            color: #CDD6F4;
            font-size: 20px;
            font-weight: bold;
            padding-bottom: 2px;
        }

        QFrame#dialogSeparator {
            color: #313244;
            max-height: 1px;
        }

        QLabel {
            color: #A6ADC8;
            font-size: 13px;
            font-weight: bold;
        }

        QLineEdit {
            background-color: #11111B;
            border: 1px solid #313244;
            border-radius: 6px;
            padding: 6px 10px;
            color: #CDD6F4;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 1px solid #89B4FA;
        }

        QComboBox {
            background-color: #11111B;
            border: 1px solid #313244;
            border-radius: 6px;
            padding: 6px 10px;
            color: #CDD6F4;
            font-size: 14px;
        }
        QComboBox:focus {
            border: 1px solid #89B4FA;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox QAbstractItemView {
            background-color: #181825;
            color: #CDD6F4;
            border: 1px solid #313244;
            selection-background-color: #45475A;
            selection-color: #CDD6F4;
        }

        QPushButton#saveBtn {
            background-color: #89B4FA;
            color: #11111B;
            border: none;
            border-radius: 6px;
            padding: 8px 24px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton#saveBtn:hover {
            background-color: #74C7EC;
        }
        QPushButton#saveBtn:pressed {
            background-color: #B4BEFE;
        }

        QPushButton#cancelBtn {
            background-color: #313244;
            color: #A6ADC8;
            border: none;
            border-radius: 6px;
            padding: 8px 20px;
            font-size: 14px;
        }
        QPushButton#cancelBtn:hover {
            background-color: #45475A;
            color: #CDD6F4;
        }

        QPushButton#togglePassBtn {
            background-color: #313244;
            border: 1px solid #313244;
            border-radius: 6px;
            font-size: 13px;
            color: #CDD6F4;
        }
        QPushButton#togglePassBtn:hover {
            background-color: #45475A;
            color: #CDD6F4;
        }
        QPushButton#togglePassBtn:checked {
            background-color: #45475A;
            border: 1px solid #89B4FA;
            color: #CDD6F4;
        }

        QPushButton#checkLeakBtn {
            background-color: #313244;
            color: #A6ADC8;
            border: none;
            border-radius: 6px;
            padding: 8px 16px;
            font-size: 13px;
        }
        QPushButton#checkLeakBtn:hover {
            background-color: #45475A;
            color: #CDD6F4;
        }
        QPushButton#checkLeakBtn:disabled {
            background-color: #181825;
            color: #585B70;
        }
    )");
}

PasswordEntry AddEntryDialog::getEntry() const
{
    PasswordEntry entry;
    entry.id       = m_entryId;
    entry.title    = m_titleEdit->text().trimmed();
    entry.username = m_usernameEdit->text().trimmed();
    entry.password = m_passwordEdit->text();
    entry.website  = m_websiteEdit->text().trimmed();
    entry.category = m_categoryCombo->currentText();
    entry.updatedAt = QDateTime::currentDateTime();
    return entry;
}

void AddEntryDialog::setEditingEntry(const PasswordEntry &entry)
{
    m_entryId = entry.id;
    m_titleEdit->setText(entry.title);
    m_usernameEdit->setText(entry.username);
    m_passwordEdit->setText(entry.password);
    m_websiteEdit->setText(entry.website);
    m_categoryCombo->setCurrentText(entry.category);

    setWindowTitle("Edit Password Entry");
    
    // Update header label text correctly
    QLabel *header = findChild<QLabel*>("dialogHeader");
    if (header) {
        header->setText("🔐  Edit Password Entry");
    }
}

void AddEntryDialog::onCheckLeakClicked()
{
    QString pwd = m_passwordEdit->text();
    if (pwd.isEmpty()) {
        m_leakStatusLabel->setText("Please enter a password first.");
        m_leakStatusLabel->setStyleSheet("color: #F38BA8;"); // Red
        return;
    }
    
    m_checkLeakButton->setEnabled(false);
    m_leakStatusLabel->setText("Checking password security...");
    m_leakStatusLabel->setStyleSheet("color: #F9E2AF;"); // Yellow
    m_leakChecker->checkPassword(pwd);
}

void AddEntryDialog::onLeakCheckCompleted(bool isLeaked, int count)
{
    m_checkLeakButton->setEnabled(true);
    if (isLeaked) {
        m_leakStatusLabel->setText(QString("⚠️ Password compromised! Found %1 times in data breaches.").arg(count));
        m_leakStatusLabel->setStyleSheet("color: #F38BA8;"); // Red
    } else {
        m_leakStatusLabel->setText("✅ Password is safe! Not found in known data breaches.");
        m_leakStatusLabel->setStyleSheet("color: #A6E3A1;"); // Green
    }
}

void AddEntryDialog::onLeakCheckError(const QString &errorMessage)
{
    m_checkLeakButton->setEnabled(true);
    m_leakStatusLabel->setText("❌ Error checking password: " + errorMessage);
    m_leakStatusLabel->setStyleSheet("color: #F38BA8;"); // Red
}
