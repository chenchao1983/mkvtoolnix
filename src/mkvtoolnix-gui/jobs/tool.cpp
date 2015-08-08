#include "common/common_pch.h"

#include "common/qt.h"
#include "mkvtoolnix-gui/forms/jobs/tool.h"
#include "mkvtoolnix-gui/forms/main_window/main_window.h"
#include "mkvtoolnix-gui/jobs/mux_job.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/merge/mux_config.h"
#include "mkvtoolnix-gui/util/message_box.h"
#include "mkvtoolnix-gui/util/settings.h"
#include "mkvtoolnix-gui/util/util.h"
#include "mkvtoolnix-gui/watch_jobs/tab.h"
#include "mkvtoolnix-gui/watch_jobs/tool.h"

#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QTreeView>

namespace mtx { namespace gui { namespace Jobs {

Tool::Tool(QWidget *parent,
           QMenu *jobQueueMenu)
  : ToolBase{parent}
  , ui{new Ui::Tool}
  , m_model{new Model{this}}
  , m_startAutomaticallyAction{new QAction{this}}
  , m_startManuallyAction{new QAction{this}}
  , m_viewOutputAction{new QAction{this}}
  , m_removeAction{new QAction{this}}
  , m_acknowledgeSelectedWarningsAction{new QAction{this}}
  , m_acknowledgeSelectedErrorsAction{new QAction{this}}
  , m_acknowledgeSelectedWarningsErrorsAction{new QAction{this}}
  , m_openFolderAction{new QAction{this}}
  , m_jobQueueMenu{jobQueueMenu}
  , m_jobsMenu{new QMenu{this}}
{
  // Setup UI controls.
  ui->setupUi(this);

  setupUiControls();
}

Tool::~Tool() {
}

void
Tool::loadAndStart() {
  auto reg = Util::Settings::registry();
  m_model->loadJobs(*reg);

  resizeColumnsToContents();

  m_model->start();
}

Model *
Tool::model()
  const {
  return m_model;
}

void
Tool::setupUiControls() {
  auto mwUi = MainWindow::getUi();

  ui->jobs->setModel(m_model);

  m_jobsMenu->addAction(m_viewOutputAction);
  m_jobsMenu->addAction(m_openFolderAction);
  m_jobsMenu->addSeparator();
  m_jobsMenu->addAction(m_startAutomaticallyAction);
  m_jobsMenu->addAction(m_startManuallyAction);
  m_jobsMenu->addSeparator();
  m_jobsMenu->addAction(m_removeAction);
  m_jobsMenu->addSeparator();
  m_jobsMenu->addAction(m_acknowledgeSelectedWarningsAction);
  m_jobsMenu->addAction(m_acknowledgeSelectedErrorsAction);
  m_jobsMenu->addAction(m_acknowledgeSelectedWarningsErrorsAction);

  connect(m_jobQueueMenu,                                   &QMenu::aboutToShow,       this,    &Tool::onJobQueueMenu);

  connect(mwUi->actionJobQueueStartAllPending,              &QAction::triggered,       this,    &Tool::onStartAllPending);

  connect(mwUi->actionJobQueueStopAfterCurrentJob,          &QAction::triggered,       this,    &Tool::onStopQueueAfterCurrentJob);
  connect(mwUi->actionJobQueueStopImmediately,              &QAction::triggered,       this,    &Tool::onStopQueueImmediately);

  connect(mwUi->actionJobQueueRemoveDone,                   &QAction::triggered,       this,    &Tool::onRemoveDone);
  connect(mwUi->actionJobQueueRemoveDoneOk,                 &QAction::triggered,       this,    &Tool::onRemoveDoneOk);
  connect(mwUi->actionJobQueueRemoveAll,                    &QAction::triggered,       this,    &Tool::onRemoveAll);

  connect(mwUi->actionJobQueueAcknowledgeAllWarnings,       &QAction::triggered,       m_model, &Model::acknowledgeAllWarnings);
  connect(mwUi->actionJobQueueAcknowledgeAllErrors,         &QAction::triggered,       m_model, &Model::acknowledgeAllErrors);
  connect(mwUi->actionJobQueueAcknowledgeAllWarningsErrors, &QAction::triggered,       m_model, &Model::acknowledgeAllWarnings);
  connect(mwUi->actionJobQueueAcknowledgeAllWarningsErrors, &QAction::triggered,       m_model, &Model::acknowledgeAllErrors);

  connect(m_startAutomaticallyAction,                       &QAction::triggered,       this,    &Tool::onStartAutomatically);
  connect(m_startManuallyAction,                            &QAction::triggered,       this,    &Tool::onStartManually);
  connect(m_viewOutputAction,                               &QAction::triggered,       this,    &Tool::onViewOutput);
  connect(m_removeAction,                                   &QAction::triggered,       this,    &Tool::onRemove);
  connect(m_acknowledgeSelectedWarningsAction,              &QAction::triggered,       this,    &Tool::acknowledgeSelectedWarnings);
  connect(m_acknowledgeSelectedErrorsAction,                &QAction::triggered,       this,    &Tool::acknowledgeSelectedErrors);
  connect(m_acknowledgeSelectedWarningsErrorsAction,        &QAction::triggered,       this,    &Tool::acknowledgeSelectedWarnings);
  connect(m_acknowledgeSelectedWarningsErrorsAction,        &QAction::triggered,       this,    &Tool::acknowledgeSelectedErrors);
  connect(m_openFolderAction,                               &QAction::triggered,       this,    &Tool::onOpenFolder);

  connect(ui->jobs,                                         &QTreeView::doubleClicked, this,    &Tool::onViewOutput);
}

void
Tool::onJobQueueMenu() {
  auto mwUi             = MainWindow::getUi();
  auto hasJobs          = m_model->hasJobs();
  auto hasPendingManual = false;

  m_model->withAllJobs([&hasPendingManual](Job &job) {
    if (Job::PendingManual == job.m_status)
      hasPendingManual = true;
  });

  mwUi->actionJobQueueStartAllPending->setEnabled(hasPendingManual);

  mwUi->actionJobQueueRemoveDone->setEnabled(hasJobs);
  mwUi->actionJobQueueRemoveDoneOk->setEnabled(hasJobs);
  mwUi->actionJobQueueRemoveAll->setEnabled(hasJobs);

  mwUi->actionJobQueueAcknowledgeAllWarnings->setEnabled(hasJobs);
  mwUi->actionJobQueueAcknowledgeAllErrors->setEnabled(hasJobs);
  mwUi->actionJobQueueAcknowledgeAllWarningsErrors->setEnabled(hasJobs);
}

void
Tool::onContextMenu(QPoint pos) {
  bool hasSelection = false;

  m_model->withSelectedJobs(ui->jobs, [&hasSelection](Job &) { hasSelection = true; });

  m_startAutomaticallyAction->setEnabled(hasSelection);
  m_startManuallyAction->setEnabled(hasSelection);
  m_viewOutputAction->setEnabled(hasSelection);
  m_removeAction->setEnabled(hasSelection);

  m_acknowledgeSelectedWarningsAction->setEnabled(hasSelection);
  m_acknowledgeSelectedErrorsAction->setEnabled(hasSelection);
  m_acknowledgeSelectedWarningsErrorsAction->setEnabled(hasSelection);

  m_jobsMenu->exec(ui->jobs->viewport()->mapToGlobal(pos));
}

void
Tool::onStartAutomatically() {
  m_model->withSelectedJobs(ui->jobs, [](Job &job) { job.setPendingAuto(); });

  m_model->startNextAutoJob();
}

void
Tool::onStartManually() {
  m_model->withSelectedJobs(ui->jobs, [](Job &job) { job.setPendingManual(); });
}

void
Tool::onStartAllPending() {
  m_model->withAllJobs([](Job &job) {
    if (Job::PendingManual == job.m_status)
      job.setPendingAuto();
  });

  m_model->startNextAutoJob();
}

void
Tool::onRemove() {
  auto idsToRemove        = QMap<uint64_t, bool>{};
  auto emitRunningWarning = false;

  m_model->withSelectedJobs(ui->jobs, [&idsToRemove](Job &job) { idsToRemove[job.m_id] = true; });

  m_model->removeJobsIf([&idsToRemove, &emitRunningWarning](Job const &job) -> bool {
    if (!idsToRemove[job.m_id])
      return false;
    if (Job::Running != job.m_status)
      return true;

    emitRunningWarning = true;
    return false;
  });

  if (emitRunningWarning)
    MainWindow::get()->setStatusBarMessage(QY("Running jobs cannot be removed."));
}

void
Tool::onRemoveDone() {
  m_model->removeJobsIf([this](Job const &job) {
      return (Job::DoneOk       == job.m_status)
          || (Job::DoneWarnings == job.m_status)
          || (Job::Failed       == job.m_status)
          || (Job::Aborted      == job.m_status);
    });
}

void
Tool::onRemoveDoneOk() {
  m_model->removeJobsIf([this](Job const &job) { return Job::DoneOk == job.m_status; });
}

void
Tool::onRemoveAll() {
  auto emitRunningWarning = false;

  m_model->removeJobsIf([&emitRunningWarning](Job const &job) -> bool {
    if (Job::Running != job.m_status)
      return true;

    emitRunningWarning = true;
    return false;
  });

  if (emitRunningWarning)
    MainWindow::get()->setStatusBarMessage(QY("Running jobs cannot be removed."));
}

void
Tool::onStopQueueAfterCurrentJob() {
  stopQueue(false);
}

void
Tool::onStopQueueImmediately() {
  stopQueue(true);
}

void
Tool::resizeColumnsToContents()
  const {
  Util::resizeViewColumnsToContents(ui->jobs);
}

void
Tool::addJob(JobPtr const &job) {
  m_model->add(job);
  resizeColumnsToContents();
}

void
Tool::retranslateUi() {
  ui->retranslateUi(this);
  m_model->retranslateUi();

  m_viewOutputAction->setText(QY("&View output"));
  m_startAutomaticallyAction->setText(QY("&Start jobs automatically"));
  m_startManuallyAction->setText(QY("Start jobs &manually"));
  m_removeAction->setText(QY("&Remove jobs"));
  m_openFolderAction->setText(QY("&Open folder"));

  m_acknowledgeSelectedWarningsAction->setText(QY("Acknowledge warnings"));
  m_acknowledgeSelectedErrorsAction->setText(QY("Acknowledge errors"));
  m_acknowledgeSelectedWarningsErrorsAction->setText(QY("Acknowledge warnings and errors"));

  setupToolTips();
}

void
Tool::setupToolTips() {
  Util::setToolTip(ui->jobs, QY("Right-click for actions for jobs"));
}

void
Tool::toolShown() {
  MainWindow::get()->showTheseMenusOnly({ m_jobQueueMenu });
}

void
Tool::acknowledgeSelectedWarnings() {
  m_model->acknowledgeSelectedWarnings(ui->jobs);
}

void
Tool::acknowledgeSelectedErrors() {
  m_model->acknowledgeSelectedErrors(ui->jobs);
}

void
Tool::onViewOutput() {
  auto tool = mtx::gui::MainWindow::watchJobTool();
  m_model->withSelectedJobs(ui->jobs, [tool](Job &job) { tool->viewOutput(job); });

  mtx::gui::MainWindow::get()->switchToTool(tool);
}

void
Tool::onOpenFolder() {
  m_model->withSelectedJobs(ui->jobs, [](Job const &job) { job.openOutputFolder(); });
}

void
Tool::acknowledgeWarningsAndErrors(uint64_t id) {
  m_model->withJob(id, [](Job &job) {
    job.acknowledgeWarnings();
    job.acknowledgeErrors();
  });
}

void
Tool::stopQueue(bool immediately) {
  auto askBeforeAborting   = Util::Settings::get().m_warnBeforeAbortingJobs;
  auto askedBeforeAborting = false;

  m_model->withAllJobs([](Job &job) {
    job.action([&job]() {
      if (job.m_status == Job::PendingAuto)
        job.setPendingManual();
    });
  });

  if (!immediately)
    return;

  m_model->withAllJobs([this, askBeforeAborting, &askedBeforeAborting](Job &job) {
    job.action([this, &job, askBeforeAborting, &askedBeforeAborting]() {
      if (job.m_status != Job::Running)
        return;

      if (   !askBeforeAborting
          ||  askedBeforeAborting
          || (Util::MessageBox::question(this, QY("Abort running jobs"), QY("Do you really want to abort the currently running job?")) == QMessageBox::Yes))
        job.abort();

      askedBeforeAborting = true;
    });
  });
}

}}}
