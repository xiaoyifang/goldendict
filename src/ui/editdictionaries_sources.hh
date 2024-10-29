/* This file is (c) 2008-2012 Konstantin Isakov <ikm@goldendict.org>
 * Part of GoldenDict. Licensed under GPLv3 or later, see the LICENSE file */

#ifndef __EDITDICTIONARIES_HH_INCLUDED__
#define __EDITDICTIONARIES_HH_INCLUDED__

#include "config.hh"
#include "dict/dictionary.hh"
#include "dict/sources.hh"
#include "groups.hh"
#include "instances.hh"
#include "orderandprops.hh"
#include "ui_editdictionaries_sources.h"
#include <QAction>
#include <QNetworkAccessManager>
#include <QPointer>

class EditDictionariesSources: public QDialog
{
  Q_OBJECT

public:

  EditDictionariesSources( QWidget * parent,
                           Config::Class & cfg,
                           std::vector< sptr< Dictionary::Class > > & dictionaries,
                           Instances::Groups & groupInstances, // We only clear those on rescan
                           QNetworkAccessManager & dictNetMgr );

  ~EditDictionariesSources();

  /// Returns true if any changes to the 'dictionaries' vector passed were done.
  bool areDictionariesChanged() const
  {
    return dictionariesChanged;
  }

  /// Returns true if groups were changed.
  bool areGroupsChanged() const
  {
    return groupsChanged;
  }

protected:

  virtual void accept();

private slots:
  void buttonBoxClicked( QAbstractButton * button );

  void rescanSources();

signals:

  void showDictionaryInfo( QString const & dictId );

  void showDictionaryHeadwords( Dictionary::Class * dict );

private:

  bool isSourcesChanged() const;

  void acceptChangedSources( bool rebuildGroups );

  //the rebuildGroups was an initative,means to build the group if possible.
  void save( bool rebuildGroups = false );

private:

  Config::Class & cfg;
  std::vector< sptr< Dictionary::Class > > & dictionaries;
  Instances::Groups & groupInstances;
  QNetworkAccessManager & dictNetMgr;

  // Backed up to decide later if something was changed or not
  Config::Class origCfg;

  Ui::EditDictionariesSources ui;
  Sources sources;
  QPointer< OrderAndProps > orderAndProps;
  QPointer< Groups > groups;

  bool dictionariesChanged;
  bool groupsChanged;

  QString lastTabName;

  QAction helpAction;
};

#endif
