/***************************************************************************
 *   Copyright (C) 2008 by Andreas Pakulat <apaku@gmx.de                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "openprojectdialog.h"
#include "openprojectpage.h"
#include "projectinfopage.h"

#include <QFileInfo>
#include <QFileDialog>

#include <KColorScheme>
#include <KIO/StatJob>
#include <KIO/ListJob>
#include <KJobWidgets>
#include <KLocalizedString>

#include "core.h"
#include "uicontroller.h"
#include "plugincontroller.h"
#include "mainwindow.h"
#include "shellextension.h"
#include "projectsourcepage.h"
#include <interfaces/iprojectcontroller.h>

namespace
{
struct URLInfo
{
    bool isValid;
    bool isDir;
    QString extension;
};

URLInfo getUrlInfo(const QUrl& url)
{
    URLInfo ret;
    ret.isValid = false;

    if (url.isLocalFile()) {
        QFileInfo info(url.toLocalFile());
        ret.isValid = info.exists();
        if (ret.isValid) {
            ret.isDir = info.isDir();
            ret.extension = info.suffix();
        }
    } else if (url.isValid()) {
        KIO::StatJob* statJob = KIO::stat(url, KIO::HideProgressInfo);
        KJobWidgets::setWindow(statJob, KDevelop::Core::self()->uiControllerInternal()->defaultMainWindow());
        ret.isValid = statJob->exec(); // TODO: do this asynchronously so that the user isn't blocked while typing every letter of the hostname in sftp://hostname
        if (ret.isValid) {
            KIO::UDSEntry entry = statJob->statResult();
            ret.isDir = entry.isDir();
            ret.extension = QFileInfo(entry.stringValue(KIO::UDSEntry::UDS_NAME)).suffix();
        }
    }
    return ret;
}
}

namespace KDevelop
{

OpenProjectDialog::OpenProjectDialog( bool fetch, const QUrl& startUrl, QWidget* parent )
    : KAssistantDialog( parent )
    , sourcePage(nullptr)
    , openPage(nullptr)
    , projectInfoPage(nullptr)
{
    resize(QSize(700, 500));

    const bool useKdeFileDialog = qEnvironmentVariableIsSet("KDE_FULL_SESSION");
    QStringList filters, allEntry;
    QString filterFormat = useKdeFileDialog
                         ? QStringLiteral("%1|%2 (%1)")
                         : QStringLiteral("%2 (%1)");
    allEntry << "*." + ShellExtension::getInstance()->projectFileExtension();
    filters << filterFormat.arg("*." + ShellExtension::getInstance()->projectFileExtension(), ShellExtension::getInstance()->projectFileDescription());
    QVector<KPluginMetaData> plugins = ICore::self()->pluginController()->queryExtensionPlugins(QStringLiteral("org.kdevelop.IProjectFileManager"));
    foreach(const KPluginMetaData& info, plugins)
    {
        QStringList filter = KPluginMetaData::readStringList(info.rawData(), QStringLiteral("X-KDevelop-ProjectFilesFilter"));
        QString desc = info.value(QStringLiteral("X-KDevelop-ProjectFilesFilterDescription"));

        if (!filter.isEmpty() && !desc.isEmpty()) {
            m_projectFilters.insert(info.name(), filter);
            allEntry += filter;
            filters << filterFormat.arg(filter.join(QStringLiteral(" ")), desc);
        }
    }

    if (useKdeFileDialog)
        filters.prepend(i18n("%1|All Project Files (%1)", allEntry.join(QStringLiteral(" "))));
    else
        filters.prepend(i18n("All Project Files (%1)", allEntry.join(QStringLiteral(" "))));

    QUrl start = startUrl.isValid() ? startUrl : Core::self()->projectController()->projectsBaseDirectory();
    start = start.adjusted(QUrl::NormalizePathSegments);
    KPageWidgetItem* currentPage = 0;

    if( fetch ) {
        sourcePageWidget = new ProjectSourcePage( start, this );
        connect( sourcePageWidget, &ProjectSourcePage::isCorrect, this, &OpenProjectDialog::validateSourcePage );
        sourcePage = addPage( sourcePageWidget, i18n("Select Source") );
        currentPage = sourcePage;
    }

    if (useKdeFileDialog) {
        openPageWidget = new OpenProjectPage( start, filters, this );
        connect( openPageWidget, &OpenProjectPage::urlSelected, this, &OpenProjectDialog::validateOpenUrl );
        connect( openPageWidget, &OpenProjectPage::accepted, this, &OpenProjectDialog::openPageAccepted );
        openPage = addPage( openPageWidget, i18n("Select a build system setup file, existing KDevelop project, "
                                                 "or any folder to open as a project") );

        if (!currentPage) {
            currentPage = openPage;
        }
    } else {
        nativeDialog = new QFileDialog(this, i18n("Open Project"));
        nativeDialog->setDirectoryUrl(start);
        nativeDialog->setFileMode(QFileDialog::ExistingFile);
        nativeDialog->setNameFilters(filters);
    }

    ProjectInfoPage* page = new ProjectInfoPage( this );
    connect( page, &ProjectInfoPage::projectNameChanged, this, &OpenProjectDialog::validateProjectName );
    connect( page, &ProjectInfoPage::projectManagerChanged, this, &OpenProjectDialog::validateProjectManager );
    projectInfoPage = addPage( page, i18n("Project Information") );

    if (!currentPage) {
        currentPage = projectInfoPage;
    }

    setValid( sourcePage, false );
    setValid( openPage, false );
    setValid( projectInfoPage, false);
    setAppropriate( projectInfoPage, false );

    setCurrentPage( currentPage );
    setWindowTitle(i18n("Open Project"));
}

bool OpenProjectDialog::execNativeDialog()
{
    while (true)
    {
        if (nativeDialog->exec()) {
            QUrl selectedUrl = nativeDialog->selectedUrls().at(0);
            if (getUrlInfo(selectedUrl).isValid) {
                // validate directory first to populate m_projectName and m_projectManager
                validateOpenUrl(selectedUrl.adjusted(QUrl::RemoveFilename));
                validateOpenUrl(selectedUrl);
                return true;
            }
        }
        else {
            return false;
        }
    }
}

int OpenProjectDialog::exec()
{
    if (nativeDialog && !execNativeDialog()) {
        reject();
        return QDialog::Rejected;
    }
    return KAssistantDialog::exec();
}

void OpenProjectDialog::validateSourcePage(bool valid)
{
    setValid(sourcePage, valid);
    openPageWidget->setUrl(sourcePageWidget->workingDir());
}

void OpenProjectDialog::validateOpenUrl( const QUrl& url_ )
{
    URLInfo urlInfo = getUrlInfo(url_);

    const QUrl url = url_.adjusted(QUrl::StripTrailingSlash);

    // openPage is used only in KDE
    if (openPage) {
        if ( urlInfo.isValid ) {
            // reset header
            openPage->setHeader(i18n("Open \"%1\" as project", url.fileName()));
        } else {
            // report error
            KColorScheme scheme(palette().currentColorGroup());
            const QString errorMsg = i18n("Selected URL is invalid");
            openPage->setHeader(QStringLiteral("<font color='%1'>%2</font>")
                .arg(scheme.foreground(KColorScheme::NegativeText).color().name(), errorMsg)
            );
            setAppropriate( projectInfoPage, false );
            setAppropriate( openPage, true );
            setValid( openPage, false );
            return;
        }
    }

    m_selected = url;

    if( urlInfo.isDir || urlInfo.extension != ShellExtension::getInstance()->projectFileExtension() )
    {
        setAppropriate( projectInfoPage, true );
        m_url = url;
        if( !urlInfo.isDir ) {
            m_url = m_url.adjusted(QUrl::StripTrailingSlash | QUrl::RemoveFilename);
        }
        ProjectInfoPage* page = qobject_cast<ProjectInfoPage*>( projectInfoPage->widget() );
        if( page )
        {
            page->setProjectName( m_url.fileName() );
            // Default manager
            page->setProjectManager( QStringLiteral("Generic Project Manager") );
            // clear the filelist
            m_fileList.clear();

            if( urlInfo.isDir ) {
                // If a dir was selected fetch all files in it
                KIO::ListJob* job = KIO::listDir( m_url );
                connect( job, &KIO::ListJob::entries,
                                this, &OpenProjectDialog::storeFileList);
                KJobWidgets::setWindow(job, Core::self()->uiController()->activeMainWindow());
                job->exec();
            } else {
                // Else we'lll just take the given file
                m_fileList << url.fileName();
            }
            // Now find a manager for the file(s) in our filelist.
            bool managerFound = false;
            foreach( const QString& manager, m_projectFilters.keys() )
            {
                foreach( const QString& filterexp, m_projectFilters.value(manager) )
                {
                    if( !m_fileList.filter( QRegExp( filterexp, Qt::CaseSensitive, QRegExp::Wildcard ) ).isEmpty() )
                    {
                        managerFound = true;
                        break;
                    }
                }
                if( managerFound ) {
                    page->setProjectManager( manager );
                    break;
                }
            }
        }
        m_url.setPath( m_url.path() + '/' + m_url.fileName() + '.' + ShellExtension::getInstance()->projectFileExtension() );
    } else {
        setAppropriate( projectInfoPage, false );
        m_url = url;
    }
    validateProjectInfo();
    setValid( openPage, true );
}

void OpenProjectDialog::openPageAccepted()
{
    if ( isValid( openPage ) ) {
        next();
    }
}

void OpenProjectDialog::validateProjectName( const QString& name )
{
    m_projectName = name;
    validateProjectInfo();
}

void OpenProjectDialog::validateProjectInfo()
{
    setValid( projectInfoPage, (!projectName().isEmpty() && !projectManager().isEmpty()) );
}

void OpenProjectDialog::validateProjectManager( const QString& manager )
{
    m_projectManager = manager;
    validateProjectInfo();
}

QUrl OpenProjectDialog::projectFileUrl() const
{
    return m_url;
}

QUrl OpenProjectDialog::selectedUrl() const
{
    return m_selected;
}

QString OpenProjectDialog::projectName() const
{
    return m_projectName;
}

QString OpenProjectDialog::projectManager() const
{
    return m_projectManager;
}

void OpenProjectDialog::storeFileList(KIO::Job*, const KIO::UDSEntryList& list)
{
    foreach( const KIO::UDSEntry& entry, list )
    {
        QString name = entry.stringValue( KIO::UDSEntry::UDS_NAME );
        if( name != QLatin1String(".") && name != QLatin1String("..") && !entry.isDir() )
        {
            m_fileList << name;
        }
    }
}

}


