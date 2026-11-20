#include "reports_widget.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QAbstractItemView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

ReportsWidget::ReportsWidget(Database &database, const AuthSession &session,
                            QWidget *parent)
    : QDialog(parent)
    , m_database(database)
    , m_session(session)
    , m_service(database, session)
{
    setWindowTitle(tr("Reportes"));
    resize(900, 520);
    buildUi();
    refresh();
}

void ReportsWidget::buildUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel(tr("Reporte:"), this));
    m_reportCombo = new QComboBox(this);
    m_reportCombo->addItem(tr("Residuos registrados"), 0);
    m_reportCombo->addItem(tr("Traslados registrados"), 1);
    topRow->addWidget(m_reportCombo);

    topRow->addWidget(new QLabel(tr("Estado:"), this));
    m_stateCombo = new QComboBox(this);
    m_stateCombo->addItem(tr("Todos"), QStringLiteral(""));
    m_stateCombo->addItem(tr("Activo (A)"), QStringLiteral("A"));
    m_stateCombo->addItem(tr("Inactivo (I)"), QStringLiteral("I"));
    m_stateCombo->addItem(tr("Eliminado (*)"), QStringLiteral("*"));
    topRow->addWidget(m_stateCombo);

    auto *refreshBtn = new QPushButton(tr("Actualizar"), this);
    topRow->addWidget(refreshBtn);
    m_exportButton = new QPushButton(tr("Exportar CSV"), this);
    topRow->addWidget(m_exportButton);
    topRow->addStretch();
    layout->addLayout(topRow);

    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_table, 1);

    auto *bottomRow = new QHBoxLayout();
    m_countLabel = new QLabel(this);
    m_totalLabel = new QLabel(this);
    bottomRow->addWidget(m_countLabel);
    bottomRow->addStretch();
    bottomRow->addWidget(m_totalLabel);
    layout->addLayout(bottomRow);

    connect(m_reportCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportsWidget::refresh);
    connect(m_stateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReportsWidget::refresh);
    connect(refreshBtn, &QPushButton::clicked, this, &ReportsWidget::refresh);
    connect(m_exportButton, &QPushButton::clicked, this, &ReportsWidget::exportCsv);
}

void ReportsWidget::refresh()
{
    const QString state = m_stateCombo->currentData().toString();
    if (m_reportCombo->currentIndex() == 0)
        m_current = m_service.wasteReport(state);
    else
        m_current = m_service.transferReport(state);

    if (m_current.permissionDenied) {
        QMessageBox::warning(this, tr("Sin permiso"), m_current.errorMessage);
        m_table->clear();
        m_table->setRowCount(0);
        m_table->setColumnCount(0);
        m_countLabel->setText(tr("Sin acceso"));
        m_totalLabel->clear();
        m_exportButton->setEnabled(false);
        return;
    }
    if (!m_current.ok) {
        QMessageBox::warning(this, tr("Error"), m_current.errorMessage);
        m_exportButton->setEnabled(false);
        return;
    }

    populateTable(m_current);
    m_countLabel->setText(tr("Registros: %1").arg(m_current.count));
    if (m_current.hasTotalAmount)
        m_totalLabel->setText(tr("Cantidad total: %1").arg(m_current.totalAmount, 0, 'f', 2));
    else
        m_totalLabel->clear();
    m_exportButton->setEnabled(m_current.count > 0);
}

void ReportsWidget::populateTable(const ReportResult &result)
{
    m_table->clear();
    m_table->setColumnCount(result.headers.size());
    m_table->setHorizontalHeaderLabels(result.headers);
    m_table->setRowCount(result.rows.size());
    for (int r = 0; r < result.rows.size(); ++r) {
        for (int c = 0; c < result.rows[r].size(); ++c) {
            m_table->setItem(r, c, new QTableWidgetItem(result.rows[r][c]));
        }
    }
    m_table->resizeColumnsToContents();
}

void ReportsWidget::exportCsv()
{
    if (!m_current.ok) {
        return;
    }
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Guardar CSV"), QString(),
        tr("Archivos CSV (*.csv)"));
    if (path.isEmpty())
        return; // cancelled: no file created

    QString error;
    if (!ReportService::writeCsv(path, m_current, &error)) {
        QMessageBox::warning(this, tr("Error de exportacion"),
                            error.isEmpty() ? tr("No se pudo escribir el archivo.") : error);
        return;
    }
    QMessageBox::information(this, tr("Exportacion completada"),
                             tr("Archivo guardado correctamente."));
}