/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

#define YUILogComponent "gtk"

#include <YGUI.h>
#include "YGUtils.h"
#include "YComboBox.h"
#include "YGSelectionStore.h"
#include "YGWidget.h"

class YGComboBox : public YComboBox, public YGLabeledWidget, public YGSelectionStore
{
	public:
		YGComboBox (YWidget *parent, const std::string &label, bool editable)
		: YComboBox (NULL, label, editable),
		  YGLabeledWidget (this, parent, label, YD_HORIZ,
                                   GTK_TYPE_COMBO_BOX, "has-entry", editable ? TRUE : FALSE, NULL),
		  YGSelectionStore (false)
	{
		const GType types[2] = { GDK_TYPE_PIXBUF, G_TYPE_STRING };
		createStore (2, types);
		gtk_combo_box_set_model (getComboBox(), getModel());

		GtkCellRenderer* cell = gtk_cell_renderer_pixbuf_new();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (getWidget()), cell, FALSE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (getWidget()), cell,
			"pixbuf", 0, NULL);

		if (editable)
			gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (getWidget()), 1);
		else {
			cell = gtk_cell_renderer_text_new();
			gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (getWidget()), cell, TRUE);
			gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (getWidget()), cell,
				"text", 1, NULL);
		}

		connect (getWidget(), "changed", G_CALLBACK (selected_changed_cb), this);
	}

	inline GtkComboBox *getComboBox()
	{ return GTK_COMBO_BOX (getWidget()); }

	GtkEntry *getEntry()
	{ return GTK_ENTRY (gtk_bin_get_child (GTK_BIN (getWidget()))); }

	// YGSelectionModel

	void blockSelected() {}

	void doAddItem (YItem *item)
	{
		GtkTreeIter iter;
		addRow (item, &iter);
		setRowText (&iter, 0, item->iconName(), 1, item->label(), this);
		if (item->selected())
			doSelectItem (item, true);
	}

	void doSelectItem (YItem *item, bool select)
	{
		if (select) {
			BlockEvents block (this);
			GtkTreeIter iter;
			getTreeIter (item, &iter);
			gtk_combo_box_set_active_iter (getComboBox(), &iter);
		}
	}

	void doDeselectAllItems()
	{
		BlockEvents block (this);
		gtk_combo_box_set_active (getComboBox(), -1);
	}

    YItem *doSelectedItem()
    {
    	GtkTreeIter iter;
    	if (gtk_combo_box_get_active_iter (getComboBox(), &iter))
    		return getYItem (&iter);
    	return NULL;
    }

	// YComboBox

	virtual std::string text()
	{
		GtkTreeIter iter;
		gchar *str;
		if (editable()) {
			// HACK: this seems to be necessary
			GtkWidget *entry = gtk_bin_get_child (GTK_BIN (getWidget()));
			return gtk_entry_get_text (GTK_ENTRY (entry));
		}
		else
                if (gtk_combo_box_get_active_iter (getComboBox(), &iter))
                        gtk_tree_model_get (getModel(), &iter, 1, &str, -1);
                else
                        return "";
		std::string ret (str);
		g_free (str);
		return ret;
	}

	virtual void setText (const std::string &value)
	{
		BlockEvents block (this);
		GtkTreeIter iter;
		if (findLabel (1, value, &iter))
			gtk_combo_box_set_active_iter (getComboBox(), &iter);
        else
            gtk_entry_set_text (getEntry(), value.c_str());
	}

	virtual void setInputMaxLength (int length)
	{
		YComboBox::setInputMaxLength (length);
		gtk_entry_set_width_chars (getEntry(), length);
	}

	virtual void setValidChars (const std::string &validChars)
	{
		YComboBox::setValidChars (validChars);
		YGUtils::setFilter (getEntry(), validChars);
	}

	// callbacks
	static void selected_changed_cb (GtkComboBox *widget, YGComboBox *pThis)
	{ pThis->emitEvent (YEvent::ValueChanged); }

	YGLABEL_WIDGET_IMPL (YComboBox)
	YGSELECTION_WIDGET_IMPL (YComboBox)
};

YComboBox *YGWidgetFactory::createComboBox (YWidget *parent, const std::string &label, bool editable)
{
	return new YGComboBox (parent, label, editable);
}

