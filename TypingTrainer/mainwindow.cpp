#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QAction>
#include <QFont>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_keyboard = new KeyboardWidget(this);
    QVBoxLayout *kbLayout = new QVBoxLayout(ui->keyboardPlaceholder);
    kbLayout->setContentsMargins(0, 0, 0, 0);
    kbLayout->addWidget(m_keyboard);

    m_textDisplay = new TextDisplayWidget(this);
    QVBoxLayout *tdLayout = new QVBoxLayout(ui->textDisplayPlaceholder);
    tdLayout->setContentsMargins(0, 0, 0, 0);
    tdLayout->addWidget(m_textDisplay);

    QFont lineFont("Courier New", 16, QFont::Bold);
    lineFont.setStyleHint(QFont::Monospace);

    m_lblPrevLine = new QLabel(this);
    m_lblPrevLine->setFont(lineFont);
    m_lblPrevLine->setStyleSheet("QLabel { color: #aaaaaa; padding: 2px 8px; }");
    m_lblPrevLine->setTextFormat(Qt::PlainText);
    m_lblPrevLine->setWordWrap(false);

    m_lblCurrLine = new QLabel(this);
    m_lblCurrLine->setFont(lineFont);
    m_lblCurrLine->setStyleSheet("QLabel { color: #222222; background: #fffde7;"
                                 " padding: 2px 8px; border-radius: 3px; }");
    m_lblCurrLine->setTextFormat(Qt::PlainText);
    m_lblCurrLine->setWordWrap(false);

    tdLayout->insertWidget(0, m_lblPrevLine);
    tdLayout->insertWidget(1, m_lblCurrLine);

    m_lessons = TextModel::builtinLessons();
    populateLessonCombo();

    setupConnections();

    m_model.loadText(m_lessons.first().text);
    updateTrainingUI();

    goToPage(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ---------------------------------------------------------------
void MainWindow::populateLessonCombo()
{
    if (m_lessons.size() == ui->comboLesson->count()) {
        ui->lblLessonDesc->setText(m_lessons[0].description);
    }
}

// ---------------------------------------------------------------
void MainWindow::setupConnections()
{
    connect(ui->btnStartTraining, &QPushButton::clicked,
            this, &MainWindow::onStartTraining);
    connect(ui->btnRestart, &QPushButton::clicked,
            this, &MainWindow::onRestartTraining);
    connect(ui->btnReturnToMain, &QPushButton::clicked,
            this, &MainWindow::onReturnToMain);

    connect(ui->actionExit, &QAction::triggered,
            this, &MainWindow::onExit);
    connect(ui->actionAbout, &QAction::triggered,
            this, &MainWindow::onAbout);

    connect(ui->comboLesson, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onLessonChanged);

    QAction *actStep = new QAction("Step Forward (Test)", this);
    actStep->setShortcut(QKeySequence("Ctrl+Right"));
    ui->menuSettings->addAction(actStep);
    connect(actStep, &QAction::triggered, this, &MainWindow::onStepForward);
}

void MainWindow::goToPage(int index)
{
    ui->stackScreens->setCurrentIndex(index);
}

void MainWindow::updateTrainingUI()
{
    m_lblPrevLine->setText(m_model.previousLine());
    m_lblCurrLine->setText(m_model.currentLine());

    int globalOffset = 0;
    const QStringList &lines = m_model.lines();
    for (int i = 0; i < m_model.lineIndex() && i < lines.size(); ++i) {
        globalOffset += lines[i].size() + 1;
    }
    globalOffset += m_model.charIndex();

    m_textDisplay->setText(m_model.fullText());
    m_textDisplay->setTypedCount(globalOffset);

    statusBar()->showMessage(
        QString("Line %1/%2  Char %3/%4  |  Typed: %5  Remaining: %6")
            .arg(m_model.lineIndex() + 1)
            .arg(m_model.lineCount())
            .arg(m_model.charIndex())
            .arg(m_model.currentLine().size())
            .arg(m_model.typedPart())
            .arg(m_model.remainingPart())
    );
}

// ---------------------------------------------------------------
void MainWindow::onLessonChanged(int index)
{
    if (index < 0 || index >= m_lessons.size())
        return;

    ui->lblLessonDesc->setText(m_lessons[index].description);

    m_model.loadText(m_lessons[index].text);
    updateTrainingUI();
}

// ---------------------------------------------------------------
void MainWindow::onStepForward()
{
    if (ui->stackScreens->currentIndex() != 1)
        return;

    m_model.advance();
    updateTrainingUI();

    if (m_model.isFinished()) {
        statusBar()->showMessage("✓ Text finished! Press Ctrl+Right had no more effect.");
    }
}

void MainWindow::onStartTraining()
{
    goToPage(1); // Training page
    updateTrainingUI();
}

void MainWindow::onRestartTraining()
{
    m_model.reset();
    goToPage(1);
    updateTrainingUI();
}

void MainWindow::onReturnToMain()
{
    goToPage(0);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "About TypingTrainer",
                       "TypingTrainer v2.0");
}

void MainWindow::onExit()
{
    close();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (ui->stackScreens->currentIndex() == 1) {
        QString key = event->text();

        if (event->key() == Qt::Key_Backspace)
            m_keyboard->pressKey("Backspace");
        else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            m_keyboard->pressKey("Enter");
        else if (event->key() == Qt::Key_Shift)
            m_keyboard->pressKey("Shift");
        else if (event->key() == Qt::Key_Space)
            m_keyboard->pressKey("Space");
        else if (!key.isEmpty())
            m_keyboard->pressKey(key);
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (ui->stackScreens->currentIndex() == 1) {
        QString key = event->text();

        if (event->key() == Qt::Key_Backspace)
            m_keyboard->releaseKey("Backspace");
        else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            m_keyboard->releaseKey("Enter");
        else if (event->key() == Qt::Key_Shift)
            m_keyboard->releaseKey("Shift");
        else if (event->key() == Qt::Key_Space)
            m_keyboard->releaseKey("Space");
        else if (!key.isEmpty())
            m_keyboard->releaseKey(key);
    }
    QMainWindow::keyReleaseEvent(event);
}
