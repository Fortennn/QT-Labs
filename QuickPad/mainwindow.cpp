#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QInputDialog>
#include <QLineEdit>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->actionSelect_All, &QAction::triggered, ui->editor, &QPlainTextEdit::selectAll);
    connect(ui->actionCopy, &QAction::triggered, ui->editor, &QPlainTextEdit::copy);
    connect(ui->actionPaste, &QAction::triggered, ui->editor, &QPlainTextEdit::paste);
    connect(ui->actionCopy, &QAction::triggered, ui->editor, &QPlainTextEdit::copy);
    connect(ui->actionCut, &QAction::triggered, ui->editor, &QPlainTextEdit::cut);
    connect(ui->actionUndo, &QAction::triggered, ui->editor, &QPlainTextEdit::undo);
    connect(ui->actionRedo, &QAction::triggered, ui->editor, &QPlainTextEdit::redo);
    connect(ui->actionSearch, &QAction::triggered, this, &MainWindow::OnSearch);
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::SaveFile);
    connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::SaveAsFile);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::OpenFile);
    connect(ui->actionNew, &QAction::triggered, [this]() {
        ui->editor->clear();
        currentFilePath.clear();
        setWindowTitle("Editor");
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::OpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open File",
        "",
        "Text Files (*.txt);;All Files (*)"
        );

    if (fileName.isEmpty())
        return;

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Помилка", "Не вдалось відкрити файл.");
        return;
    }

    QTextStream in(&file);
    ui->editor->setPlainText(in.readAll());
    file.close();

    currentFilePath = fileName;
    setWindowTitle("Editor - " + fileName);
}

void MainWindow::SaveFile()
{
    if (currentFilePath.isEmpty()) {
        SaveAsFile();
        return;
    }

    QFile file(currentFilePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Помилка", "Неможливо зберегти файл.");
        return;
    }

    QTextStream out(&file);
    out << ui->editor->toPlainText();
    file.close();
}

void MainWindow::SaveAsFile()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Save File As",
        "",
        "Text Files (*.txt);;All Files (*)"
        );

    if (fileName.isEmpty())
        return;

    currentFilePath = fileName;
    SaveFile();

    setWindowTitle("Golden Editor - " + fileName);
}


void MainWindow::OnSearch()
{
    bool ok;
    QString text = QInputDialog::getText(
        this,
        "Search",
        "Enter text:",
        QLineEdit::Normal,
        lastSearchText,
        &ok
        );

    if (!ok || text.isEmpty())
        return;

    lastSearchText = text;

    if (!ui->editor->find(text)) {
        ui->editor->moveCursor(QTextCursor::Start);

        if (!ui->editor->find(text)) {
            QMessageBox::information(this, "Знайти", "Текст не знайдено.");
        }
    }
}
