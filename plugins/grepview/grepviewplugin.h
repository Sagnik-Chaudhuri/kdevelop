/***************************************************************************
 *   Copyright 1999-2001 by Bernd Gehrmann                                 *
 *   bernd@kdevelop.org                                                    *
 *   Copyright 2010 Julien Desgats <julien.desgats@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KDEVPLATFORM_PLUGIN_GREPVIEWPLUGIN_H
#define KDEVPLATFORM_PLUGIN_GREPVIEWPLUGIN_H

#include <interfaces/iplugin.h>
#include <interfaces/contextmenuextension.h>

#include <QList>
#include <QPointer>
#include <QVariant>

class KJob;
class GrepDialog;
class GrepJob;
class GrepOutputViewFactory;

class GrepViewPlugin : public KDevelop::IPlugin
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.kdevelop.GrepViewPlugin" )

public:
    explicit GrepViewPlugin( QObject *parent, const QVariantList & = QVariantList() );
    ~GrepViewPlugin() override;

    void unload() override;

    void rememberSearchDirectory(QString const & directory);
    KDevelop::ContextMenuExtension contextMenuExtension(KDevelop::Context* context) override;
    void showDialog(bool setLastUsed = false, QString pattern = QString(), bool show = true);

    /**
     * Returns a new instance of GrepJob. Since the plugin supports only one job at the same time,
     * previous job, if any, is killed before creating a new job.
     */
    GrepJob *newGrepJob();
    GrepJob *grepJob();
    GrepOutputViewFactory* toolViewFactory() const;
public Q_SLOTS:
    ///@param pattern the pattern to search
    ///@param directory the directory, or a semicolon-separated list of files
    ///@param show whether the search dialog should be shown. if false,
    ///            the parameters of the last search will be used.
    Q_SCRIPTABLE void startSearch(QString pattern, QString directory, bool show);

Q_SIGNALS:
    void grepJobFinished(bool success);

private Q_SLOTS:
    void showDialogFromMenu();
    void showDialogFromProject();
    void jobFinished(KJob *job);

private:
    GrepJob *m_currentJob;
    QList<QPointer<GrepDialog> > m_currentDialogs;
    QString m_directory;
    QString m_contextMenuDirectory;
    GrepOutputViewFactory* m_factory;
};

#endif
