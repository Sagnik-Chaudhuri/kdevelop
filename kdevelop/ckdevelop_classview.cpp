/***************************************************************************
                          cclassview.cpp  -  implementation
                             -------------------
    begin                : Fri Mar 19 1999
    copyright            : (C) 1999 by Jonas Nordin
    email                : jonas.nordin@cenacle.se
    based on             : ckdevelop_classview.cpp by Sandy Meier
   
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/

#include "ckdevelop.h"
#include "cclassview.h"
#include "caddclassmethoddlg.h"
#include "caddclassattributedlg.h"
#include <assert.h>


/*********************************************************************
 *                                                                   *
 *                              SLOTS                                *
 *                                                                   *
 ********************************************************************/

/*--------------------------------- CKDevelop::slotClassbrowserViewTree()
 * slotClassbrowserViewTree()
 *   Event when the user clicks the graphical view toolbar button.
 *
 * Parameters:
 *   -
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::slotClassbrowserViewTree()
{
  class_tree->viewGraphicalTree();
}

/*--------------------------------- CKDevelop::slotClassChoiceCombo()
 * slotClassChoiceCombo()
 *   Event when the user selects an item in the classcombo.
 *
 * Parameters:
 *   index           Index of the selected item.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::slotClassChoiceCombo(int index)
{
  CParsedClass *aClass;
  KCombo* classCombo = toolBar(ID_BROWSER_TOOLBAR)->getCombo(ID_CV_TOOLBAR_CLASS_CHOICE);
  QString classname = classCombo->text( index );

  if ( !classname.isEmpty() )
  {
    aClass = class_tree->store->getClassByName( classname );
    refreshMethodCombo( aClass );
  }
}

/*-------------------------------- CKDevelop::slotMethodChoiceCombo()
 * slotMethodChoiceCombo()
 *   Event when the user selects an item in the methodcombo.
 *
 * Parameters:
 *   index           Index of the selected item.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::slotMethodChoiceCombo(int index)
{
  CParsedClass *aClass;
  CParsedMethod *aMethod;
  QString toFile;
  int toLine;
  KCombo* classCombo = toolBar(ID_BROWSER_TOOLBAR)->getCombo(ID_CV_TOOLBAR_CLASS_CHOICE);
  KCombo* methodCombo = toolBar(ID_BROWSER_TOOLBAR)->getCombo(ID_CV_TOOLBAR_METHOD_CHOICE);
  QString classname = classCombo->currentText();
  QString methodname = methodCombo->text( index );

  if (methodname == "") return;
  aClass = class_tree->store->getClassByName( classname );
  if( aClass )
  {
    aMethod = aClass->getMethodByNameAndArg( methodname );

    if( aMethod )
    {
      toFile = ( aMethod->isInHFile ? aClass->definedInFile : aClass->declaredInFile );
      toLine = aMethod->declaredOnLine;
      switchToFile( toFile, toLine );
    }
  }
}

/*-------------------------------- CKDevelop::slotCVViewDeclaration()
 * slotCVViewDeclaration()
 *   Event when the user wants a declaration for an item in the tree.
 *
 * Parameters:
 *   index           Index of the item.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::slotCVViewDeclaration( const char *className, 
                                       const char *declName, 
                                       THType type )
{
  CVGotoDeclaration( className, declName, type );
}

/*-------------------------------- CKDevelop::slotCVViewDefinition()
 * slotCVViewDefinition()
 *   Event when the user wants a definition for an item in the tree.
 *
 * Parameters:
 *   index           Index of the item.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::slotCVViewDefinition(  const char *className, 
                                       const char *declName, 
                                       THType type )
{
  CVGotoDefinition( className, declName, type );
}

/*-------------------------------------- CKDevelop::slotCVAddMethod()
 * slotCVAddMethod()
 *   Event when the user adds a method to a class.
 *
 * Parameters:
 *   aMethod         The method to add.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::slotCVAddMethod( const char *aClassName )
{
  CParsedClass *aClass;
  QString toAdd;
  int atLine = -1;
  CParsedMethod *meth = NULL;
  CParsedMethod *aMethod;
  CAddClassMethodDlg dlg(this, "methodDlg" );
  
  // Show the dialog and let the user fill it out.
  if( dlg.exec() )
  {
    aMethod = dlg.asSystemObj();
    aMethod->setDeclaredInClass( aClassName );
  }
  else
    return;

  // Fetch the current class.
  aClass = class_tree->store->getClassByName( aClassName );

  // Search for a method with the same export as the one being added.
  for( aClass->methodIterator.toFirst();
       aClass->methodIterator.current() && meth == NULL;
       ++aClass->methodIterator )
  {
    if( aClass->methodIterator.current()->export == aMethod->export )
      meth = aClass->methodIterator.current();
  }

  // Switch to the .h file.
  CVGotoDefinition( aClass->name, NULL, THCLASS );  

  aMethod->asHeaderCode( toAdd );

  if( meth )
    atLine = meth->definedOnLine + 1;
  else
  {
    switch( aMethod->export )
    {
      case PIE_PUBLIC:
        toAdd = "public: // Public methods\n" + toAdd;
        break;
      case PIE_PROTECTED:
        toAdd = "protected: // Protected methods\n" + toAdd;
        break;
      case PIE_PRIVATE:
        toAdd = "private: // Private methods\n" + toAdd;
        break;
      default:
        break;
    }

    atLine = aClass->definedOnLine + 2;
  }

  // Add the declaration.
  edit_widget->insertAtLine( toAdd, atLine );
  edit_widget->setCursorPosition( atLine, 0 );
  edit_widget->toggleModified( true );

  // Switch to the .cpp file.
  switchToFile( aClass->declaredInFile );

  // Add the code to the file.
  aMethod->asCppCode( toAdd );
  edit_widget->append( toAdd );
  edit_widget->setCursorPosition( edit_widget->lines() - 1, 0 );
  edit_widget->toggleModified( true );
}

/*------------------------------------- CKDevelop::slotCVAddAttribute()
 * slotCVAddAttribute()
 *   Event when the user adds an attribute to a class.
 *
 * Parameters:
 *   aAttr           The attribute to add.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::slotCVAddAttribute( const char *aClassName )
{
  CParsedClass *aClass;
  CParsedAttribute *attr = NULL;
  QString toAdd;
  int atLine = -1;
  CAddClassAttributeDlg dlg(this, "attrDlg" );
  CParsedAttribute *aAttr;

  if( dlg.exec() )
  {
    aAttr = dlg.asSystemObj();

    aAttr->setDeclaredInClass( aClassName );
  }
  else
    return;

  // Fetch the current class.
  aClass = class_tree->store->getClassByName( aClassName );
  
  for( aClass->attributeIterator.toFirst();
       aClass->attributeIterator.current() && attr == NULL;
       ++aClass->attributeIterator )
  {
    if( aClass->attributeIterator.current()->export == aAttr->export )
      attr = aClass->attributeIterator.current();
  }

  // Switch to the .h file.
  CVGotoDefinition( aClass->name, NULL, THCLASS );  

  // If we find an attribute with the same export we don't need to output
  // the label as well.
  aAttr->asHeaderCode( toAdd );
  if( attr )
    atLine = attr->definedOnLine + 1;
  else
  {
    switch( aAttr->export )
    {
      case PIE_PUBLIC:
        toAdd = "public: // Public attributes\n" + toAdd;
        break;
      case PIE_PROTECTED:
        toAdd = "protected: // Protected attributes\n" + toAdd;
        break;
      case PIE_PRIVATE:
        toAdd = "private: // Private attributes\n" + toAdd;
        break;
      default:
        break;
    }

    atLine = aClass->definedOnLine + 2;
  }

  // Add the code to the file.
  edit_widget->insertAtLine( toAdd, atLine );
  edit_widget->setCursorPosition( atLine, 0 );
  edit_widget->toggleModified( true );
}


/*********************************************************************
 *                                                                   *
 *                          PUBLIC METHODS                           *
 *                                                                   *
 ********************************************************************/

/*-------------------------------------- CKDevelop::CVClassSelected()
 * CVClassSelected()
 *   The class has been selected, make sure the classcombo updates.
 *
 * Parameters:
 *   aName          Name of the class.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::CVClassSelected( const char *aName )
{
  KCombo* classCombo = toolBar(ID_BROWSER_TOOLBAR)->getCombo(ID_CV_TOOLBAR_CLASS_CHOICE);
  bool found = false;
  int i;

  for( i=0; i< classCombo->count() && !found; i++ )
    found = ( strcmp( classCombo->text( i ), aName ) == 0 );

  if( found )
  {
    i--;
    classCombo->setCurrentItem( i );
    slotClassChoiceCombo( i );
  }
}

/*-------------------------------------- CKDevelop::CVMethodSelected()
 * CVMethodSelected()
 *   A method has been selected, make sure the methodcombo updates.
 *
 * Parameters:
 *   aName          Name of the method.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::CVMethodSelected( const char *aName )
{
  KCombo* methodCombo = toolBar(ID_BROWSER_TOOLBAR)->getCombo(ID_CV_TOOLBAR_METHOD_CHOICE);
  bool found = false;
  int i;

  for( i=0; i< methodCombo->count() && !found; i++ )
    found = ( strcmp( methodCombo->text( i ), aName ) == 0 );

  if( found )
  {
    i--;
    methodCombo->setCurrentItem( i );
  }
}

/*-------------------------------------- CKDevelop::CVGotoDefinition()
 * CVGotoDefinition()
 *   Goto the definition of the item with the index in the tree.
 *
 * Parameters:
 *   index          Index in the tree.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::CVGotoDefinition( const char *className, 
                                  const char *declName, 
                                  THType type )
{
  CParsedClass *aClass = NULL;
  CParsedAttribute *aAttr = NULL;
  CParsedStruct *aStruct = NULL;
  QString toFile;
  int toLine = -1;

  // Fetch a class if one is passed.
  if( className != NULL && strlen( className ) > 0 )
  {
    aClass = class_tree->store->getClassByName( className );

    if( aClass != NULL )
      CVClassSelected( className );
  }

  // Get the type of declaration at the index.
  switch( type )
  {
    case THCLASS:
      toFile = aClass->definedInFile;
      toLine = aClass->definedOnLine;
      break;
    case THSTRUCT:
      if( aClass != NULL )
        aClass->getStructByName( declName );
      else
        aStruct = class_tree->store->globalContainer.getStructByName( declName );

      toFile = aStruct->definedInFile;
      toLine = aStruct->definedOnLine;
      break;
    case THPUBLIC_ATTR:
    case THPROTECTED_ATTR:
    case THPRIVATE_ATTR:
      if( aClass != NULL )
        aAttr = aClass->getAttributeByName( declName );
      else
      {
        aStruct = class_tree->store->globalContainer.getStructByName( className );
        if( aStruct != NULL )
          aAttr = aStruct->getAttributeByName( declName );
      }
      break;
    case THPUBLIC_METHOD:
    case THPROTECTED_METHOD:
    case THPRIVATE_METHOD:
      aAttr = aClass->getMethodByNameAndArg( declName );
      CVMethodSelected( declName );
      break;
    case THPUBLIC_SLOT:
    case THPROTECTED_SLOT:
    case THPRIVATE_SLOT:
      aAttr = aClass->getSlotByNameAndArg( declName );
      break;
    case THPUBLIC_SIGNAL:
    case THPROTECTED_SIGNAL:
    case THPRIVATE_SIGNAL:
      aAttr = aClass->getSignalByNameAndArg( declName );
      break;
    case THGLOBAL_FUNCTION:
      aAttr = class_tree->store->globalContainer.getMethodByNameAndArg( declName );
      break;
    case THGLOBAL_VARIABLE:
      aAttr = class_tree->store->globalContainer.getAttributeByName( declName );
      break;
    default:
      debug( "Clicked on unknown type." );
  }

  // Fetch the line and file from the attribute if the value is set.
  if( aAttr != NULL )
  {
    toFile = aAttr->definedInFile;
    toLine = aAttr->definedOnLine;
  }

  if( toLine != -1 )
  {
    debug( "  Switching to file %s @ line %d", toFile.data(), toLine );
    switchToFile( toFile, toLine );
  }
}

/*-------------------------------------- CKDevelop::CVGotoDeclaration()
 * CVGotoDeclaration()
 *   Goto the declaration of the item with the index in the tree.
 *
 * Parameters:
 *   index          Index in the tree.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::CVGotoDeclaration( const char *className, 
                                   const char *declName, 
                                   THType type )
{
  CParsedClass *aClass = NULL;
  CParsedMethod *aMethod = NULL;

  if( className != NULL && strlen( className ) > 0 )
  {
    aClass = class_tree->store->getClassByName( className );
    CVClassSelected( className );
  }

  switch( type )
  {
    case THPUBLIC_SLOT:
    case THPROTECTED_SLOT:
    case THPRIVATE_SLOT:
      if( aClass )
        aMethod = aClass->getSlotByNameAndArg( declName );      
      break;
    case THPUBLIC_METHOD:
    case THPROTECTED_METHOD:
    case THPRIVATE_METHOD:
      if( aClass )
      {
        aMethod = aClass->getMethodByNameAndArg( declName );
        CVMethodSelected( declName );
      }
      break;
    case THGLOBAL_FUNCTION:
      aMethod = class_tree->store->globalContainer.getMethodByNameAndArg( declName );
      break;
    default:
      break;
  }
  
  if( aMethod )
    switchToFile( aMethod->declaredInFile, aMethod->declaredOnLine );
}

/*----------------------------------- CKDevelop::refreshClassCombo()
 * refreshClassCombo()
 *   Update the class combo with all classes.
 *
 * Parameters:
 *   -
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::refreshClassCombo()
{
  CParsedClass *aClass;
  QList<CParsedClass> *classList;
  KCombo* classCombo = toolBar(ID_BROWSER_TOOLBAR)->getCombo(ID_CV_TOOLBAR_CLASS_CHOICE);
  KCombo* methodCombo = toolBar(ID_BROWSER_TOOLBAR)->getCombo(ID_CV_TOOLBAR_METHOD_CHOICE);
  QString savedClass;
  int savedIdx = -1;
  int i;

  savedClass = classCombo->currentText();

  // Clear the combos.
  classCombo->clear();

  classList = class_tree->store->getSortedClasslist();
  // Add all classes.
  for( aClass = classList->first(),i=0;
       aClass != NULL;
       aClass = classList->next(), i++ )
  {
    classCombo->insertItem( aClass->name );
    if( aClass->name == savedClass )
      savedIdx = i;
  }

  delete classList;

  // Update the method combo with the class from the classcombo.
  aClass = class_tree->store->getClassByName( savedClass );
  if( aClass && savedIdx != -1 )
  {
    classCombo->setCurrentItem( savedIdx );
    refreshMethodCombo( aClass );
  }
  else
    methodCombo->clear();
}

/*----------------------------------- CKDevelop::refreshMethodCombo()
 * refreshMethodCombo()
 *   Update the method combo with the methods from the selected
 *   class.
 *
 * Parameters:
 *   aClass           The selected class.
 *
 * Returns:
 *   -
 *-----------------------------------------------------------------*/
void CKDevelop::refreshMethodCombo( CParsedClass *aClass )
{
  QListBox *lb;
  KCombo* methodCombo = toolBar(ID_BROWSER_TOOLBAR)->getCombo(ID_CV_TOOLBAR_METHOD_CHOICE);
  QString str;
  QString savedMethod;

  // Save the current value.
  savedMethod = methodCombo->currentText();

  methodCombo->clear();
  lb = methodCombo->listBox();
  lb->setAutoUpdate( false );

  // Add all methods, slots and signals of this class.
  for( aClass->methodIterator.toFirst(); 
       aClass->methodIterator.current();
       ++aClass->methodIterator )
  {
    aClass->methodIterator.current()->asString( str );
    lb->inSort( str );
  }
  
  for( aClass->slotIterator.toFirst(); 
       aClass->slotIterator.current();
       ++aClass->slotIterator )
  {
    aClass->slotIterator.current()->asString( str );
    lb->inSort( str );
  }

  lb->setAutoUpdate( true );

  // Try to restore the saved value.
  for(int i=0; i<methodCombo->count(); i++ )
  {
    if( savedMethod == methodCombo->text( i ) )
      methodCombo->setCurrentItem( i );
  }
}





