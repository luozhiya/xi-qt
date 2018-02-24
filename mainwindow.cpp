#include <QFileDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editor.h"
#include "xibridge.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    editor_(nullptr),
    bridge_(nullptr)
{
    ui->setupUi(this);
    setCentralWidget(&stack_);
    init();
}

MainWindow::~MainWindow()
{
    delete bridge_;
    delete editor_;
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName;
    fileName = QFileDialog::getOpenFileName(this,
        tr("Open Text"), "", "");
    editor_->new_view(fileName);
}

void MainWindow::on_actionClose_triggered()
{
    close();
}

void MainWindow::init()
{
    xi::EditorOption option = xi::Editor::getDefaultOption();
    bridge_ = new xi::XiBridge();
    bridge_->spawn();
    editor_ = new xi::Editor(bridge_, option);
    bridge_->init(editor_);

    stack_.insertWidget(1, editor_);
    stack_.setCurrentIndex(1);
}
