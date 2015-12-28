/*  This file is part of KDevelop
    Copyright 2010 Aleix Pol <aleixpol@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "vcschangesviewplugin.h"

#include <QAction>

#include <KActionCollection>
#include <KLocalizedString>
#include <KPluginFactory>

#include <interfaces/icore.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/iuicontroller.h>
#include <vcs/interfaces/ibasicversioncontrol.h>
#include <vcs/vcsstatusinfo.h>
#include <vcs/vcsjob.h>
#include <project/projectchangesmodel.h>
#include <project/projectmodel.h>
#include <interfaces/iplugincontroller.h>
#include "vcschangesview.h"

K_PLUGIN_FACTORY_WITH_JSON(VcsProjectIntegrationFactory, "kdevvcschangesview.json", registerPlugin<VcsProjectIntegrationPlugin>();)

using namespace KDevelop;

class VCSProjectToolViewFactory : public KDevelop::IToolViewFactory
{
public:
    VCSProjectToolViewFactory(VcsProjectIntegrationPlugin *plugin): m_plugin(plugin) {}

    QWidget* create(QWidget *parent = 0) override
    {
        VcsChangesView* modif = new VcsChangesView(m_plugin, parent);
        modif->setModel(m_plugin->model());
        QObject::connect(modif, static_cast<void(VcsChangesView::*)(const QList<KDevelop::IProject*>&)>(&VcsChangesView::reload), m_plugin->model(), static_cast<void(ProjectChangesModel::*)(const QList<KDevelop::IProject*>&)>(&ProjectChangesModel::reload));
        QObject::connect(modif, static_cast<void(VcsChangesView::*)(const QList<KDevelop::IProject*>&)>(&VcsChangesView::reload), m_plugin->model(), static_cast<void(ProjectChangesModel::*)(const QList<KDevelop::IProject*>&)>(&ProjectChangesModel::reload));
        QObject::connect(modif, &VcsChangesView::activated, m_plugin, &VcsProjectIntegrationPlugin::activated);
        return modif;
    }

    Qt::DockWidgetArea defaultPosition() override
    {
        return Qt::RightDockWidgetArea;
    }

    QString id() const override
    {
        return "org.kdevelop.VCSProject";
    }

private:
    VcsProjectIntegrationPlugin *m_plugin;
};

VcsProjectIntegrationPlugin::VcsProjectIntegrationPlugin(QObject* parent, const QVariantList&)
    : KDevelop::IPlugin("kdevvcsprojectintegration", parent)
    , m_model(0)
{
    ICore::self()->uiController()->addToolView(i18n("Project Changes"), new VCSProjectToolViewFactory(this));

    QAction* synaction = actionCollection()->addAction( "locate_document" );
    synaction->setText(i18n("Locate Current Document"));
    synaction->setIcon(QIcon::fromTheme("dirsync"));
    synaction->setToolTip(i18n("Locates the current document and selects it."));

    QAction* reloadaction = actionCollection()->addAction( "reload_view" );
    reloadaction->setText(i18n("Reload View"));
    reloadaction->setIcon(QIcon::fromTheme("view-refresh"));
    reloadaction->setToolTip(i18n("Refreshes the view for all projects, in case anything changed."));
}

void VcsProjectIntegrationPlugin::activated(const QModelIndex& /*idx*/)
{

}

ProjectChangesModel* VcsProjectIntegrationPlugin::model()
{
    if(!m_model) {
        m_model = ICore::self()->projectController()->changesModel();
        connect(actionCollection()->action("reload_view"), &QAction::triggered, m_model, &ProjectChangesModel::reloadAll);
    }

    return m_model;
}

#include "vcschangesviewplugin.moc"
