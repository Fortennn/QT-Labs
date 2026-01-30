#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    converter = new Converter();
    this->setWindowTitle("Convertor");


    auto validator = new QDoubleValidator(this);
    validator->setLocale(QLocale::c());
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setDecimals(10);

    ui->length1->setValidator(validator);
    ui->length2->setValidator(validator);

    ui->mass1->setValidator(validator);
    ui->mass2->setValidator(validator);

    ui->temp1->setValidator(validator);
    ui->temp2->setValidator(validator);

    connect(ui->length1, &QLineEdit::textEdited, this, &MainWindow::updateConversion);
    connect(ui->length2, &QLineEdit::textEdited, this, &MainWindow::updateConversion);

    connect(ui->combo_length1, &QComboBox::currentIndexChanged, this, &MainWindow::updateConversion);
    connect(ui->combo_length2, &QComboBox::currentIndexChanged, this, &MainWindow::updateConversion);

    connect(ui->mass1, &QLineEdit::textEdited, this, &MainWindow::updateConversion);
    connect(ui->mass2, &QLineEdit::textEdited, this, &MainWindow::updateConversion);

    connect(ui->combo_mass1, &QComboBox::currentIndexChanged, this, &MainWindow::updateConversion);
    connect(ui->combo_mass2, &QComboBox::currentIndexChanged, this, &MainWindow::updateConversion);

    connect(ui->temp1, &QLineEdit::textEdited, this, &MainWindow::updateConversion);
    connect(ui->temp2, &QLineEdit::textEdited, this, &MainWindow::updateConversion);

    connect(ui->combo_temp1, &QComboBox::currentIndexChanged, this, &MainWindow::updateConversion);
    connect(ui->combo_temp2, &QComboBox::currentIndexChanged, this, &MainWindow::updateConversion);

    connect(ui->tabWidget,    &QTabWidget::currentChanged,     this, &MainWindow::updateConversion);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateConversion(){
    QSignalBlocker block1(ui->length1);
    QSignalBlocker block2(ui->length2);
    QSignalBlocker block3(ui->mass1);
    QSignalBlocker block4(ui->mass2);
    QSignalBlocker block5(ui->temp1);
    QSignalBlocker block6(ui->temp2);

    int tabIndex = ui->tabWidget->currentIndex();
    Converter::Mode currentMode;
    QString sourceText;
    QLineEdit* sourceEdit = nullptr;
    QLineEdit* targetEdit = nullptr;
    QComboBox* sourceCombo = nullptr;
    QComboBox* targetCombo = nullptr;

    if (tabIndex == 0) {
        currentMode = Converter::Mode::length;
        sourceEdit = ui->length1;
        targetEdit = ui->length2;
        sourceCombo = ui->combo_length1;
        targetCombo = ui->combo_length2;
    }
    else if (tabIndex == 1) {
        currentMode = Converter::Mode::mass;
        sourceEdit = ui->mass1;
        targetEdit = ui->mass2;
        sourceCombo = ui->combo_mass1;
        targetCombo = ui->combo_mass2;
    }
    else if (tabIndex == 2) {
        currentMode = Converter::Mode::temperature;
        sourceEdit = ui->temp1;
        targetEdit = ui->temp2;
        sourceCombo = ui->combo_temp1;
        targetCombo = ui->combo_temp2;
    }
    else {
        return;
    }

    sourceText = sourceEdit->text().trimmed();
    if (sourceText.isEmpty()) {
        targetEdit->clear();
        return;
    }
    QString normalized = sourceText;
    normalized.replace(',', '.');
    bool ok;
    double value = sourceText.toDouble(&ok);
    if (!ok) {
        targetEdit->clear();
        return;
    }
    int fromIndex = sourceCombo->currentIndex();
    int toIndex   = targetCombo->currentIndex();

    double result = converter->convert(value, currentMode, fromIndex, toIndex);
    targetEdit->setText(QString::number(result, 'g', 8));
}
