/* This file is part of the KDE libraries
   Copyright (C) 2007 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "projectfilequickopen.h"
#include <QIcon>
#include <QTextBrowser>
#include <QApplication>
#include <klocale.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/iproject.h>
#include <interfaces/icore.h>
#include <language/duchain/topducontext.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/indexedstring.h>
#include <language/duchain/parsingenvironment.h>
#include <project/projectmodel.h>

#include "../openwith/iopenwith.h"

using namespace KDevelop;

ProjectFileData::ProjectFileData( const ProjectFile& file )
: m_file(file)
{
}

QString ProjectFileData::text() const
{
    KUrl u(m_file.projectUrl.toUrl());
    u.adjustPath(KUrl::AddTrailingSlash);
    return KUrl::relativeUrl( u, m_file.url.toUrl() );
}

QString ProjectFileData::htmlDescription() const
{
  return "<small><small>" + i18nc("%1: project name", "Project %1", m_file.project.str())  + "</small></small>";
}

bool ProjectFileData::execute( QString& filterText )
{
    IOpenWith::openFiles(KUrl::List() << m_file.url.toUrl());
    QString path;
    uint lineNumber;
    if (extractLineNumber(filterText, path, lineNumber)) {
        IDocument* doc = ICore::self()->documentController()->documentForUrl(m_file.url.toUrl());
        if (doc) {
            doc->setCursorPosition(KTextEditor::Cursor(lineNumber - 1, 0));
        }
    }
    return true;
}

bool ProjectFileData::isExpandable() const
{
    return true;
}

QList<QVariant> ProjectFileData::highlighting() const
{
    QTextCharFormat boldFormat;
    boldFormat.setFontWeight(QFont::Bold);
    QTextCharFormat normalFormat;

    QString txt = text();

    QList<QVariant> ret;

    int fileNameLength = m_file.url.toUrl().fileName().length();

    ret << 0;
    ret << txt.length() - fileNameLength;
    ret << QVariant(normalFormat);
    ret << txt.length() - fileNameLength;
    ret << fileNameLength;
    ret << QVariant(boldFormat);

    return ret;
}

QWidget* ProjectFileData::expandingWidget() const
{
    DUChainReadLocker lock;

    ///Find a du-chain for the document
    QList<TopDUContext*> contexts = DUChain::self()->chainsForDocument(m_file.url);

    ///Pick a non-proxy context
    TopDUContext* chosen = 0;
    foreach( TopDUContext* ctx, contexts ) {
        if( !(ctx->parsingEnvironmentFile() && ctx->parsingEnvironmentFile()->isProxyContext()) ) {
            chosen = ctx;
        }
    }


    if( chosen ) {
        return chosen->createNavigationWidget(0, 0,
            "<small><small>"
            + i18nc("%1: project name", "Project %1", m_file.project.str())
            + "</small></small>");
    } else {
        QTextBrowser* ret = new QTextBrowser();
        ret->resize(400, 100);
        ret->setText(
                "<small><small>"
                + i18nc("%1: project name", "Project %1", m_file.project.str())
                + "<br>" + i18n("Not parsed yet") + "</small></small>");
        return ret;
    }

    return 0;
}

QIcon ProjectFileData::icon() const
{
    return m_file.icon;
}

BaseFileDataProvider::BaseFileDataProvider()
{
}

void BaseFileDataProvider::setFilterText( const QString& text )
{
    QString path(text);
    uint lineNumber;
    extractLineNumber(text, path, lineNumber);
    if ( path.startsWith(QLatin1String("./")) || path.startsWith(QLatin1String("../")) ) {
        // assume we want to filter relative to active document's url
        IDocument* doc = ICore::self()->documentController()->activeDocument();
        if (doc) {
            KUrl url = doc->url().upUrl();
            url.addPath( path);
            url.cleanPath();
            url.adjustPath(KUrl::RemoveTrailingSlash);
            path = url.pathOrUrl();
        }
    }
    Base::setFilter( path.split('/', QString::SkipEmptyParts), QChar('/') );
}

uint BaseFileDataProvider::itemCount() const
{
    return Base::filteredItems().count();
}

uint BaseFileDataProvider::unfilteredItemCount() const
{
    return Base::items().count();
}

QList<QuickOpenDataPointer> BaseFileDataProvider::data( uint start, uint end ) const
{
    if( end > (uint)Base::filteredItems().count() ) {
        end = Base::filteredItems().count();
    }

    QList<QuickOpenDataPointer> ret;
    ret.reserve(end - start);

    for( uint a = start; a < end; a++ ) {
        ProjectFile f( Base::filteredItems()[a] );
        ret << QuickOpenDataPointer( new ProjectFileData( Base::filteredItems()[a] ) );
    }

    return ret;
}

QString BaseFileDataProvider::itemText( const ProjectFile& data ) const
{
    return data.url.str();
}

ProjectFileDataProvider::ProjectFileDataProvider()
{
    connect(ICore::self()->projectController(), SIGNAL(projectClosing(KDevelop::IProject*)),
            this, SLOT(projectClosing(KDevelop::IProject*)));
    connect(ICore::self()->projectController(), SIGNAL(projectOpened(KDevelop::IProject*)),
            this, SLOT(projectOpened(KDevelop::IProject*)));
}

void ProjectFileDataProvider::projectClosing( IProject* project )
{
    foreach(const IndexedString& str, project->fileSet()) {
        fileRemovedFromSet(project, str);
    }
}

void ProjectFileDataProvider::projectOpened( IProject* project )
{
    const int processAfter = 1000;
    int processed = 0;
    foreach(const IndexedString& url, project->fileSet()) {
        fileAddedToSet(project, url);
        if (++processed == processAfter) {
            // prevent UI-lockup when a huge project was imported
            QApplication::processEvents();
            processed = 0;
        }
    }

    connect(project, SIGNAL(fileAddedToSet(KDevelop::IProject*, KDevelop::IndexedString)),
            this, SLOT(fileAddedToSet(KDevelop::IProject*, KDevelop::IndexedString)));
    connect(project, SIGNAL(fileRemovedFromSet(KDevelop::IProject*, KDevelop::IndexedString)),
            this, SLOT(fileRemovedFromSet(KDevelop::IProject*, KDevelop::IndexedString)));
}

void ProjectFileDataProvider::fileAddedToSet( IProject* project, const IndexedString& url )
{
    ProjectFile f;
    f.url = url;
    f.project = IndexedString(project->name());
    f.projectUrl = IndexedString(project->folder());
    m_projectFiles.insert(url.byteArray(), f);
}

void ProjectFileDataProvider::fileRemovedFromSet( IProject*, const IndexedString& url )
{
    m_projectFiles.remove(url.byteArray());
}

namespace
{
QSet<IndexedString> openFiles()
{
    QSet<IndexedString> openFiles;
    foreach( IDocument* doc, ICore::self()->documentController()->openDocuments() ) {
        openFiles << IndexedString(doc->url().pathOrUrl());
    }
    return openFiles;
}
}

bool sortProjectFiles(const ProjectFile& left, const ProjectFile& right)
{
    // arbitrarily sort projects (fast)
    if ( left.project != right.project ) {
        return left.project < right.project;
    }
    return left.url.byteArray() < right.url.byteArray();
}

void ProjectFileDataProvider::reset()
{
    Base::clearFilter();

    QSet<IndexedString> openFiles_ = openFiles();
    QList<ProjectFile> projectFiles;
    projectFiles.reserve(m_projectFiles.size());

    for(QMap<QByteArray, ProjectFile>::const_iterator it = m_projectFiles.constBegin();
        it != m_projectFiles.constEnd(); ++it)
    {
        if (!openFiles_.contains(it->url)) {
            projectFiles << *it;
        }
    }

    setItems(projectFiles);
}

QSet<IndexedString> ProjectFileDataProvider::files() const
{
    QSet<IndexedString> ret;

    foreach( IProject* project, ICore::self()->projectController()->projects() )
        ret += project->fileSet();

    return ret - openFiles();
}

void OpenFilesDataProvider::reset()
{
    Base::clearFilter();
    QList<ProjectFile> currentFiles;
    IProjectController* projCtrl = ICore::self()->projectController();
    IDocumentController* docCtrl = ICore::self()->documentController();

    const KIcon icon("tab-duplicate");
    foreach( IDocument* doc, docCtrl->openDocuments() ) {
        ProjectFile f;
        f.icon = icon;
        f.url = IndexedString(doc->url().pathOrUrl());
        IProject* project = projCtrl->findProjectForUrl(doc->url());
        if (project) {
            f.projectUrl = IndexedString(project->folder().pathOrUrl());
            f.project = IndexedString(project->name());
        } else {
            f.project = IndexedString(i18n("none"));
        }
        currentFiles << f;
    }

    qSort(currentFiles.begin(), currentFiles.end(), sortProjectFiles);

    setItems(currentFiles);
}

QSet<IndexedString> OpenFilesDataProvider::files() const
{
    return openFiles();
}

#include "projectfilequickopen.moc"
