/* $Id: rgsrcwindow.cc,v 1.15 2003/01/23 13:29:49 mvogt Exp $ */
#include <apt-pkg/error.h>
#include <glib.h>
#include <assert.h>
#include "rgrepositorywin.h"
#include "gsynaptic.h"
#include "config.h"
#include "i18n.h"

#if HAVE_RPM
enum  {ITEM_TYPE_RPM, 
       ITEM_TYPE_RPMSRC, 
       ITEM_TYPE_DEB , 
       ITEM_TYPE_DEBSRC};
#else
enum  {ITEM_TYPE_DEB, 
       ITEM_TYPE_DEBSRC, 
       ITEM_TYPE_RPM , 
       ITEM_TYPE_RPMSRC};
#endif

enum {   
    STATUS_COLUMN,
    TYPE_COLUMN,
    VENDOR_COLUMN,
    URI_COLUMN,
    DISTRIBUTION_COLUMN,
    SECTIONS_COLUMN,
    RECORD_COLUMN,
    DISABLED_COLOR_COLUMN,
    N_SOURCES_COLUMNS
};

RGRepositoryEditor::RGRepositoryEditor(RGWindow *parent)
    : RGGladeWindow(parent, "repositories")
{
    cout << "RGRepositoryEditor::RGRepositoryEditor(RGWindow *parent)"<<endl;
    assert(_win);
    _userDialog = new RGUserDialog(_win);
    _applied = false;
    _lastIter = NULL;

    setTitle(_("Setup Repositories"));
    gtk_window_set_modal(GTK_WINDOW(_win), TRUE);
    gtk_window_set_policy(GTK_WINDOW(_win), TRUE, TRUE, FALSE);

    // build gtktreeview
    _sourcesListStore = gtk_list_store_new (N_SOURCES_COLUMNS, 
					    G_TYPE_BOOLEAN, 
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    G_TYPE_POINTER,
					    GDK_TYPE_COLOR);
    
    _sourcesListView = glade_xml_get_widget(_gladeXML, 
					     "treeview_repositories");
    gtk_tree_view_set_model(GTK_TREE_VIEW(_sourcesListView),
			    GTK_TREE_MODEL(_sourcesListStore));    
    
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;

    // status
    renderer = gtk_cell_renderer_toggle_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Enabled"),
						       renderer,
						       "active", STATUS_COLUMN,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (_sourcesListView), column);
    
    // type
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Type"),
						       renderer,
						       "text", TYPE_COLUMN,
						       "foreground-gdk", DISABLED_COLOR_COLUMN,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW(_sourcesListView), column);

    // vendor
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Vendor"),
						       renderer,
						       "text", VENDOR_COLUMN,
						       "foreground-gdk", DISABLED_COLOR_COLUMN,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW(_sourcesListView), column);

    // uri
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("URI"),
						       renderer,
						       "text", URI_COLUMN,
						       "foreground-gdk", DISABLED_COLOR_COLUMN,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW(_sourcesListView), column);

    // distribution
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Distribution"),
						       renderer,
						       "text", DISTRIBUTION_COLUMN,
						       "foreground-gdk", DISABLED_COLOR_COLUMN,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW(_sourcesListView), column);

    // section(s)
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Section(s)"),
						       renderer,
						       "text", SECTIONS_COLUMN,
						       "foreground-gdk", DISABLED_COLOR_COLUMN,
						       NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW(_sourcesListView), column);

    GtkTreeSelection *select;
    select = gtk_tree_view_get_selection(GTK_TREE_VIEW (_sourcesListView));
    gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT (select), "changed",
		      G_CALLBACK(SelectionChanged),
		      this);

    _cbEnabled = glade_xml_get_widget(_gladeXML, "checkbutton_enabled");

    _optType = glade_xml_get_widget(_gladeXML, "optionmenu_type");
    _optTypeMenu = gtk_menu_new();

    GtkWidget *item;
#if HAVE_RPM
    item = gtk_menu_item_new_with_label("rpm");
    gtk_menu_append(GTK_MENU(_optTypeMenu), item);
    gtk_widget_show(item);
    gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_RPM);
    
    item = gtk_menu_item_new_with_label("rpm-src");
    gtk_menu_append(GTK_MENU(_optTypeMenu), item);
    gtk_widget_show(item);
    gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_RPMSRC);
    
    item = gtk_menu_item_new_with_label("deb");
    gtk_menu_append(GTK_MENU(_optTypeMenu), item);
    gtk_widget_show(item);
    gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_DEB);
    
    item = gtk_menu_item_new_with_label("deb-src");
    gtk_menu_append(GTK_MENU(_optTypeMenu), item);
    gtk_widget_show(item);
    gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_DEBSRC);
#else
    item = gtk_menu_item_new_with_label("deb");
    gtk_menu_append(GTK_MENU(_optTypeMenu), item);
    gtk_widget_show(item);
    gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_DEB);
    
    item = gtk_menu_item_new_with_label("deb-src");
    gtk_menu_append(GTK_MENU(_optTypeMenu), item);
    gtk_widget_show(item);
    gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_DEBSRC);

    item = gtk_menu_item_new_with_label("rpm");
    gtk_menu_append(GTK_MENU(_optTypeMenu), item);
    gtk_widget_show(item);
    gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_RPM);
    
    item = gtk_menu_item_new_with_label("rpm-src");
    gtk_menu_append(GTK_MENU(_optTypeMenu), item);
    gtk_widget_show(item);
    gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)ITEM_TYPE_RPMSRC);
#endif
    gtk_option_menu_set_menu(GTK_OPTION_MENU(_optType), _optTypeMenu);

    
    _optVendor = glade_xml_get_widget(_gladeXML, "optionmenu_vendor");
    _optVendorMenu = gtk_menu_new();
    item = gtk_menu_item_new_with_label(_("(no vendor)"));
    gtk_menu_append(GTK_MENU(_optVendorMenu), item);
    gtk_widget_show(item);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(_optVendor), _optVendorMenu);
    gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)"");

    _entryURI = glade_xml_get_widget(_gladeXML, "entry_uri");
    assert(_entryURI);
    _entryDist = glade_xml_get_widget(_gladeXML, "entry_distribution");
    assert(_entryDist);
    _entrySect = glade_xml_get_widget(_gladeXML, "entry_sections");
    assert(_entrySect);    

    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_ok_clicked",
				  G_CALLBACK(DoOK),
				  this);

    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_cancel_clicked",
				  G_CALLBACK(DoCancel),
				  this);

    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_edit_vendors_clicked",
				  G_CALLBACK(VendorsWindow),
				  this);

    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_clear_clicked",
				  G_CALLBACK(DoClear),
				  this);

    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_add_clicked",
				  G_CALLBACK(DoAdd),
				  this);

    glade_xml_signal_connect_data(_gladeXML,
				  "on_button_remove_clicked",
				  G_CALLBACK(DoRemove),
				  this);
}


RGRepositoryEditor::~RGRepositoryEditor()
{
    //gtk_widget_destroy(_win);
    delete _userDialog;
}


bool RGRepositoryEditor::Run()
{
  if(_lst.ReadSources() == false) {
    _error->Error(_("Cannot read sources.list file"));
    _userDialog->showErrors();
    return false;
  }
  if(_lst.ReadVendors() == false) {
    _error->Error(_("Cannot read vendors.list file"));
    _userDialog->showErrors();
    return false;
  }

  GdkColormap *cmap = gdk_colormap_get_system();
  _gray.red = _gray.green = _gray.blue = 0xAA00;
  gdk_color_alloc(cmap, &_gray);

  GtkTreeIter iter;
  for(SourcesListIter it =  _lst.SourceRecords.begin();
      it != _lst.SourceRecords.end(); it++)
      {
	  if ((*it)->Type & SourcesList::Comment) continue;
	  string Sections;
	  for (unsigned int J = 0; J < (*it)->NumSections; J++) {
	      Sections += (*it)->Sections[J];
	      Sections += " ";
	  }
	  
	  gtk_list_store_append(_sourcesListStore, &iter);  
	  gtk_list_store_set(_sourcesListStore, &iter,
			     STATUS_COLUMN, !((*it)->Type & 
					      SourcesList::Disabled),
			     TYPE_COLUMN,   utf8((*it)->GetType().c_str()),
			     VENDOR_COLUMN, utf8((*it)->VendorID.c_str()), 
			     URI_COLUMN,    utf8((*it)->URI.c_str()),
			     DISTRIBUTION_COLUMN, utf8((*it)->Dist.c_str()), 
			     SECTIONS_COLUMN, utf8(Sections.c_str()),
			     RECORD_COLUMN, (gpointer)(*it),
			     DISABLED_COLOR_COLUMN, (*it)->Type & SourcesList::Disabled ? &_gray : NULL,
			     -1);
      }

  
  UpdateVendorMenu();
  
  gtk_main();
  return _applied;
}

void RGRepositoryEditor::UpdateVendorMenu()
{
    gtk_option_menu_remove_menu(GTK_OPTION_MENU(_optVendor));
    _optVendorMenu = gtk_menu_new();
    GtkWidget *item = gtk_menu_item_new_with_label(_("(no vendor)"));
    gtk_menu_append(GTK_MENU(_optVendorMenu), item);
    gtk_object_set_data(GTK_OBJECT(item), "id", (gpointer)"");
    gtk_widget_show(item);
    for (VendorsListIter it = _lst.VendorRecords.begin();
	 it != _lst.VendorRecords.end(); it++) {
	item = gtk_menu_item_new_with_label((*it)->VendorID.c_str());
	gtk_menu_append(GTK_MENU(_optVendorMenu), item);
	gtk_widget_show(item);
	gtk_object_set_data(GTK_OBJECT(item), "id",
			    (gpointer)(*it)->VendorID.c_str());
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(_optVendor), _optVendorMenu);
}

int RGRepositoryEditor::VendorMenuIndex(string VendorID)
{
    int index = 0;
    for (VendorsListIter it = _lst.VendorRecords.begin();
	 it != _lst.VendorRecords.end(); it++) {
	index += 1;
	if ((*it)->VendorID == VendorID)
	    return index;
    }
    return 0;
}

void RGRepositoryEditor::DoClear(GtkWidget *, gpointer data)
{
    RGRepositoryEditor *me = (RGRepositoryEditor*)data;
    cout << "RGRepositoryEditor::DoClear(GtkWidget *, gpointer data)"<<endl;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(me->_cbEnabled), FALSE);
    gtk_option_menu_set_history(GTK_OPTION_MENU(me->_optType), 0);
    gtk_option_menu_set_history(GTK_OPTION_MENU(me->_optVendor), 0);
    gtk_entry_set_text(GTK_ENTRY(me->_entryURI), "");
    gtk_entry_set_text(GTK_ENTRY(me->_entryDist), "");
    gtk_entry_set_text(GTK_ENTRY(me->_entrySect), "");
}

void RGRepositoryEditor::DoAdd(GtkWidget *, gpointer data)
{
    RGRepositoryEditor *me = (RGRepositoryEditor*)data;
    cout << "RGRepositoryEditor::DoAdd(GtkWidget *, gpointer data)"<<endl;
 
   unsigned char Type = 0;
    const char *Section;
    string VendorID;
    string URI;
    string Dist;
    string Sections[5];
    unsigned short count = 0;
    // XXX user added entries go to sources.list for now..
    string SourceFile = "/etc/apt/sources.list";

    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(me->_cbEnabled)))
	Type |= SourcesList::Disabled;
    
    GtkWidget *menuitem = gtk_menu_get_active(GTK_MENU(me->_optTypeMenu));
    switch ((int)(gtk_object_get_data(GTK_OBJECT(menuitem), "id"))) {
    case ITEM_TYPE_DEB:
	Type |= SourcesList::Deb; break;
    case ITEM_TYPE_DEBSRC:
	Type |= SourcesList::DebSrc; break;
    case ITEM_TYPE_RPM:
	Type |= SourcesList::Rpm; break;
    case ITEM_TYPE_RPMSRC:
	Type |= SourcesList::RpmSrc; break;
    default:
	me->_userDialog->error(_("Unknown source type"));
	return;
    }

    menuitem = gtk_menu_get_active(GTK_MENU(me->_optVendorMenu));
    VendorID = (char*)gtk_object_get_data(GTK_OBJECT(menuitem), "id");
    
    URI = gtk_entry_get_text(GTK_ENTRY(me->_entryURI));
    Dist = gtk_entry_get_text(GTK_ENTRY(me->_entryDist));
    Section = gtk_entry_get_text(GTK_ENTRY(me->_entrySect));

    if (Section != 0 && Section[0] != 0)
	Sections[count++] = Section;
    
    SourcesList::SourceRecord *rec = me->_lst.AddSource((SourcesList::RecType)Type, VendorID, URI, Dist, Sections, count, SourceFile);

    if(rec == NULL) {
	me->_userDialog->error(_("Invalid URL"));
	return;
    }

    string Sects;
    for (unsigned short J = 0; J < rec->NumSections; J++) {
	Sects += rec->Sections[J];
	Sects += " ";
    }

    GtkTreeIter iter;
    gtk_list_store_append(me->_sourcesListStore, &iter);  
    gtk_list_store_set(me->_sourcesListStore, &iter,
		       STATUS_COLUMN, !(rec->Type & 
					SourcesList::Disabled),
		       TYPE_COLUMN,   utf8(rec->GetType().c_str()),
		       VENDOR_COLUMN, utf8(rec->VendorID.c_str()), 
		       URI_COLUMN,    utf8(rec->URI.c_str()),
		       DISTRIBUTION_COLUMN, utf8(rec->Dist.c_str()), 
		       SECTIONS_COLUMN, utf8(Sects.c_str()),
		       RECORD_COLUMN, (gpointer)(rec),
		       DISABLED_COLOR_COLUMN, (rec->Type & SourcesList::Disabled ? &me->_gray : NULL),
		       -1);
}

void RGRepositoryEditor::doEdit()
{
    cout << "RGRepositoryEditor::doEdit()"<<endl;


    //GtkTreeSelection *selection;
    //selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (_sourcesListView));
    if(_lastIter == NULL) {
	cout << "deadbeef"<<endl;
	return;
    }

    GtkTreeModel *model = GTK_TREE_MODEL(_sourcesListStore);
    //if (!gtk_tree_selection_get_selected (selection, &model, &iter)) 
    //return;
	
    SourcesList::SourceRecord *rec;
    gtk_tree_model_get(model, _lastIter, 
		       RECORD_COLUMN, &rec, 
		       -1);
    
    rec->Type = 0;
    if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_cbEnabled)))
	rec->Type |= SourcesList::Disabled;
    
    GtkWidget *menuitem = gtk_menu_get_active(GTK_MENU(_optTypeMenu));
    switch ((long)(gtk_object_get_data(GTK_OBJECT(menuitem), "id"))) {
    case ITEM_TYPE_DEB:
	rec->Type |= SourcesList::Deb; break;
    case ITEM_TYPE_DEBSRC:
	rec->Type |= SourcesList::DebSrc; break;
    case ITEM_TYPE_RPM:
	rec->Type |= SourcesList::Rpm; break;
    case ITEM_TYPE_RPMSRC:
	rec->Type |= SourcesList::RpmSrc; break;
    default:
	_userDialog->error(_("Unknown source type"));
	return;
    }

    menuitem = gtk_menu_get_active(GTK_MENU(_optVendorMenu));
    rec->VendorID = (char*)gtk_object_get_data(GTK_OBJECT(menuitem), "id");
    
    rec->URI = gtk_entry_get_text(GTK_ENTRY(_entryURI));
    rec->Dist = gtk_entry_get_text(GTK_ENTRY(_entryDist));
	
    delete [] rec->Sections;
    rec->NumSections = 0;

    const char *Section = gtk_entry_get_text(GTK_ENTRY(_entrySect));
    if (Section != 0 && Section[0] != 0)
	rec->NumSections++;
    
    rec->Sections = new string[rec->NumSections];
    rec->NumSections = 0;
    Section = gtk_entry_get_text(GTK_ENTRY(_entrySect));

    if (Section != 0 && Section[0] != 0)
	rec->Sections[rec->NumSections++] = Section;

    string Sect;
    for (unsigned int I = 0; I < rec->NumSections; I++) {
	Sect += rec->Sections[I];
	Sect += " ";
    }

    /* repaint screen */
    gtk_list_store_set(_sourcesListStore, _lastIter,
		       STATUS_COLUMN, !(rec->Type & 
					SourcesList::Disabled),
		       TYPE_COLUMN,   utf8(rec->GetType().c_str()),
		       VENDOR_COLUMN, utf8(rec->VendorID.c_str()), 
		       URI_COLUMN,    utf8(rec->URI.c_str()),
		       DISTRIBUTION_COLUMN, utf8(rec->Dist.c_str()), 
		       SECTIONS_COLUMN, utf8(Sect.c_str()),
		       RECORD_COLUMN, (gpointer)(rec),
		       DISABLED_COLOR_COLUMN, (rec->Type & SourcesList::Disabled ? &_gray : NULL),	
		       -1);

}

void RGRepositoryEditor::DoRemove(GtkWidget *, gpointer data)
{
    RGRepositoryEditor *me = (RGRepositoryEditor*)data;	
    cout << "RGRepositoryEditor::DoRemove(GtkWidget *, gpointer data)"<<endl;
    
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(me->_sourcesListView));
    if(gtk_tree_selection_get_selected(selection, 
				       (GtkTreeModel**)(&me->_sourcesListStore), 
				       &iter)) 
	{
	    SourcesList::SourceRecord *rec;
	    gtk_tree_model_get(GTK_TREE_MODEL(me->_sourcesListStore), &iter, 
					      RECORD_COLUMN, &rec, 
					      -1);
	    assert(rec);
	    
	    me->_lst.RemoveSource(rec);
	    gtk_list_store_remove(me->_sourcesListStore, &iter);
	    if(me->_lastIter != NULL)
		gtk_tree_iter_free(me->_lastIter);
	    me->_lastIter = NULL;
	}

}

void RGRepositoryEditor::DoOK(GtkWidget *, gpointer data)
{
    RGRepositoryEditor *me = (RGRepositoryEditor*)data;	

    me->doEdit();
    me->_lst.UpdateSources();
    gtk_main_quit();
    me->_applied = true;
}

void RGRepositoryEditor::DoCancel(GtkWidget *, gpointer data)
{
    //GtkUI::SrcEditor *This = (GtkUI::SrcEditor *)data;
    gtk_main_quit();
}



void RGRepositoryEditor::SelectionChanged(GtkTreeSelection *selection, 
					  gpointer data)
{
    RGRepositoryEditor *me = (RGRepositoryEditor*)data;	
    cout << "RGRepositoryEditor::SelectionChanged()"<<endl;

    GtkTreeIter iter;
    GtkTreeModel *model;

    if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
	me->doEdit(); // save the old row
	if(me->_lastIter != NULL)
	    gtk_tree_iter_free(me->_lastIter);
	me->_lastIter = gtk_tree_iter_copy(&iter);

	const SourcesList::SourceRecord *rec;
	gtk_tree_model_get(model, &iter, 
			   RECORD_COLUMN, &rec, 
			   -1);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(me->_cbEnabled),
				     !(rec->Type & SourcesList::Disabled));

	int id = ITEM_TYPE_DEB;
	if (rec->Type & SourcesList::DebSrc)
	    id = ITEM_TYPE_DEBSRC;
	else if (rec->Type & SourcesList::Rpm)
	    id = ITEM_TYPE_RPM;
	else if (rec->Type & SourcesList::RpmSrc)
	    id = ITEM_TYPE_RPMSRC;
	gtk_option_menu_set_history(GTK_OPTION_MENU(me->_optType), id);

	gtk_option_menu_set_history(GTK_OPTION_MENU(me->_optVendor),
				    me->VendorMenuIndex(rec->VendorID));

	gtk_entry_set_text(GTK_ENTRY(me->_entryURI), utf8(rec->URI.c_str()));
	gtk_entry_set_text(GTK_ENTRY(me->_entryDist),utf8(rec->Dist.c_str()));
	gtk_entry_set_text(GTK_ENTRY(me->_entrySect), "");

	for (unsigned int I = 0; I < rec->NumSections; I++) {
		gtk_entry_append_text(GTK_ENTRY(me->_entrySect), 
				      utf8(rec->Sections[I].c_str()));
		gtk_entry_append_text(GTK_ENTRY(me->_entrySect), " ");
	}
    }
}

void RGRepositoryEditor::VendorsWindow(GtkWidget *, void *data)
{
    RGRepositoryEditor *me = (RGRepositoryEditor*)data;
    RGVendorsEditor w(me, me->_lst);
    w.Run();
    GtkWidget *menuitem = gtk_menu_get_active(GTK_MENU(me->_optVendorMenu));
    string VendorID = (char*)gtk_object_get_data(GTK_OBJECT(menuitem), "id");
    me->UpdateVendorMenu();
    gtk_option_menu_set_history(GTK_OPTION_MENU(me->_optVendor),
				me->VendorMenuIndex(VendorID));
}



