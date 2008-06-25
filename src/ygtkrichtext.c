/********************************************************************
 *           YaST2-GTK - http://en.opensuse.org/YaST2-GTK           *
 ********************************************************************/

/* YGtkRichText widget */
// check the header file for information about this widget

#include <config.h>
#include "ygtkrichtext.h"
#include <gtk/gtkversion.h>
#include <string.h>

#define IDENT_MARGIN      20
#define PARAGRAPH_SPACING 12

//#define PRINT_WARNINGS

// Sucky - but we mix C & C++ so ...
/* Convert html to xhtml (or at least try) */
gchar *ygutils_convert_to_xhmlt_and_subst (const char *instr);

G_DEFINE_TYPE (YGtkRichText, ygtk_rich_text, GTK_TYPE_TEXT_VIEW)

static guint link_clicked_signal;
static GdkColor link_color = { 0, 0, 0, 0xeeee };

// utilities
// Looks at all tags covering the position of iter in the text view,
// and returns the link the text points to, in case that text is a link.
static const char *get_link (GtkTextView *text_view, gint x, gint y)
{
	GtkTextIter iter;
	gtk_text_view_get_iter_at_location (text_view, &iter, x, y);

	char *link = NULL;
	GSList *tags = gtk_text_iter_get_tags (&iter), *tagp;
	for (tagp = tags; tagp != NULL; tagp = tagp->next) {
		GtkTextTag *tag = (GtkTextTag*) tagp->data;
		link = (char*) g_object_get_data (G_OBJECT (tag), "link");
		if (link)
			break;
	}

	if (tags)
		g_slist_free (tags);
	return link;
}

// callbacks
// Links can also be activated by clicking.
static gboolean event_after (GtkWidget *text_view, GdkEvent *ev)
{
	GtkTextIter start, end;
	GtkTextBuffer *buffer;
	GdkEventButton *event;
	gint x, y;

	if (ev->type != GDK_BUTTON_RELEASE)
		return FALSE;

	event = (GdkEventButton *)ev;
	if (event->button != 1)
		return FALSE;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

	// We shouldn't follow a link if the user is selecting something.
	gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
	if (gtk_text_iter_get_offset (&start) != gtk_text_iter_get_offset (&end))
		return FALSE;

	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view),
		GTK_TEXT_WINDOW_WIDGET, (gint) event->x, (gint) event->y, &x, &y);

	const char *link = get_link (GTK_TEXT_VIEW (text_view), x, y);
	if (link)  // report link
		g_signal_emit (YGTK_RICH_TEXT (text_view), link_clicked_signal, 0, link);
	return FALSE;
}

#include <stdlib.h>
static int mystrcmp(void *a, void *b)
{ return g_ascii_strcasecmp (*(char **)a, *(char **)b); }

static gboolean isBlockTag (const char *tag)
{
	static const char *Tags[] =
	{ "blockquote", "h1", "h2", "h3", "h4", "h5", "li", "p", "pre" };
	void *ret;
	ret = bsearch (&tag, Tags, sizeof (Tags)/sizeof(char*), sizeof(char *), (void*)mystrcmp);
	return ret != 0;
}
static gboolean isIdentTag (const char *tag)
{
	static const char *Tags[] =
	{ "blockquote", "ol", "ul" };
	void *ret;
	ret = bsearch (&tag, Tags, sizeof (Tags)/sizeof(char*), sizeof(char *), (void*)mystrcmp);
	return ret != 0;
}

void ygtk_rich_text_init (YGtkRichText *rtext)
{
	GtkTextView *tview = GTK_TEXT_VIEW (rtext);
	gtk_text_view_set_wrap_mode (tview, GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_editable (tview, FALSE);
	gtk_text_view_set_cursor_visible (tview, FALSE);
	gtk_text_view_set_pixels_below_lines (tview, 4);
	gtk_text_view_set_left_margin (tview, 4);

	// Init link support
	GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (rtext));
	rtext->hand_cursor = gdk_cursor_new_for_display (display, GDK_HAND1);
	gdk_cursor_ref (rtext->hand_cursor);

#if GTK_CHECK_VERSION(2,10,0)
	gtk_widget_style_get (GTK_WIDGET (rtext), "link_color", &link_color, NULL);
#endif

	g_signal_connect (tview, "event-after",
	                  G_CALLBACK (event_after), NULL);

	// Create a few tags like 'h3', 'b', 'i'. others need to be created as we parse
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (tview);
	PangoFontDescription *font_desc = GTK_WIDGET (rtext)->style->font_desc;
	int size = pango_font_description_get_size (font_desc);
	if (pango_font_description_get_size_is_absolute (font_desc))
		size /= PANGO_SCALE;

	gtk_text_buffer_create_tag (buffer, "h1", "weight", PANGO_WEIGHT_HEAVY,
		"size", (int)(size * PANGO_SCALE_XX_LARGE), "pixels-below-lines", 16,
		"foreground", "#5c5c5c", NULL);
	gtk_text_buffer_create_tag (buffer, "h2", "weight", PANGO_WEIGHT_ULTRABOLD,
		"size", (int)(size * PANGO_SCALE_X_LARGE), "pixels-below-lines", 15, NULL);
	gtk_text_buffer_create_tag (buffer, "h3", "weight", PANGO_WEIGHT_BOLD,
		"size", (int)(size * PANGO_SCALE_LARGE), "pixels-below-lines", 14, NULL);
	gtk_text_buffer_create_tag (buffer, "h4", "weight", PANGO_WEIGHT_SEMIBOLD,
		"size", (int)(size * PANGO_SCALE_LARGE), "pixels-below-lines", 13, NULL);
	gtk_text_buffer_create_tag (buffer, "h5",
		"size", (int)(size * PANGO_SCALE_LARGE), NULL);
	gtk_text_buffer_create_tag (buffer, "p", "pixels-below-lines", 12, NULL);
	gtk_text_buffer_create_tag (buffer, "big",
	                            "size", (int)(size * PANGO_SCALE_LARGE), NULL);
	gtk_text_buffer_create_tag (buffer, "small",
	                            "size", (int)(size * PANGO_SCALE_SMALL), NULL);
	gtk_text_buffer_create_tag (buffer, "tt", "family", "monospace", NULL);
	gtk_text_buffer_create_tag (buffer, "pre", "family", "monospace",
		"paragraph-background", "#f0f0f0", NULL);
	gtk_text_buffer_create_tag (buffer, "b", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag (buffer, "i", "style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_buffer_create_tag (buffer, "u", "underline", PANGO_UNDERLINE_SINGLE, NULL);
	gtk_text_buffer_create_tag (buffer, "center", "justification", GTK_JUSTIFY_CENTER, NULL);
	// helpers
	gtk_text_buffer_create_tag (buffer, "keyword", "background", "yellow", NULL);
}

static void ygtk_rich_text_destroy (GtkObject *object)
{
	// destroy can be called multiple times, and we must ref only once
	YGtkRichText *rtext = YGTK_RICH_TEXT (object);
	gdk_cursor_unref (rtext->hand_cursor);
	GTK_OBJECT_CLASS (ygtk_rich_text_parent_class)->destroy (object);
}

static void ygtk_rich_text_realize (GtkWidget *widget)
{
	GTK_WIDGET_CLASS (ygtk_rich_text_parent_class)->realize (widget);
	GtkTextView *view = GTK_TEXT_VIEW (widget);
	GdkWindow *window = gtk_text_view_get_window (view, GTK_TEXT_WINDOW_TEXT);
	gdk_window_set_cursor (window, NULL);
}

// Change the cursor to the "hands" cursor typically used by web browsers,
// if there is a link in the given position.
static void set_cursor_if_appropriate (GtkTextView *view, gint wx, gint wy)
{
	if (wx == -1) {
		GtkWidget *widget = GTK_WIDGET (view);
		gdk_window_get_pointer (widget->window, &wx, &wy, NULL);
		if (wx < 0 || wy < 0 || wx >= widget->allocation.width ||
		    wy >= widget->allocation.height)
			return;
	}
	gint bx, by;
	gtk_text_view_window_to_buffer_coords (view, GTK_TEXT_WINDOW_WIDGET,
	                                       wx, wy, &bx, &by);

	static gboolean hovering_over_link = FALSE;
	gboolean hovering = get_link (view, bx, by) != NULL;

	if (hovering != hovering_over_link) {
		hovering_over_link = hovering;
		YGtkRichText *rtext = YGTK_RICH_TEXT (view);
		GdkWindow *window = gtk_text_view_get_window (view, GTK_TEXT_WINDOW_TEXT);
		GdkCursor *cursor = hovering ? rtext->hand_cursor : NULL;
		gdk_window_set_cursor (window, cursor);
	}
}

// Update the cursor image if the pointer moved.
static gboolean ygtk_rich_text_motion_notify_event (GtkWidget *widget,
                                                    GdkEventMotion *event)
{
	set_cursor_if_appropriate (GTK_TEXT_VIEW (widget), event->x, event->y);
	return TRUE;
}

static gboolean ygtk_rich_text_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	gboolean ret;
	ret = GTK_WIDGET_CLASS (ygtk_rich_text_parent_class)->expose_event (widget, event);
	set_cursor_if_appropriate (GTK_TEXT_VIEW (widget), -1, -1);
	return ret;
}

/* Rich Text parsing methods. */

typedef struct _HTMLList
{
	gboolean ordered;
	char enumeration;
} HTMLList;

static void HTMLList_init (HTMLList *list, gboolean ordered)
{
	list->ordered = ordered;
	list->enumeration = 1;
}

typedef struct GRTPTag {
	GtkTextMark *mark;
	GtkTextTag  *tag;
} GRTPTag;
typedef struct GRTParseState {
	GtkTextBuffer *buffer;
	GtkTextTagTable *tags;
	GList *htags;  // of GRTPTag

	// Attributes for tags that affect their children
	gboolean pre_mode;
	gboolean default_color;
	int left_margin;
	GList *html_list;  // of HTMLList
	gboolean closed_block_tag;
} GRTParseState;

static void GRTParseState_init (GRTParseState *state, GtkTextBuffer *buffer)
{
	state->buffer = buffer;
	state->pre_mode = FALSE;
	state->default_color = TRUE;
	state->left_margin = 0;
	state->tags = gtk_text_buffer_get_tag_table (buffer);
	state->html_list = NULL;
	state->htags = NULL;
	state->closed_block_tag = FALSE;
}

static void free_list (GList *list)
{
	GList *i;
	for (i = g_list_first (list); i; i = i->next)
		g_free (i->data);
	g_list_free (list);
}

static void GRTParseState_free (GRTParseState *state)
{
	// NOTE: some elements might not have been freed because of bad html
	free_list (state->html_list);
	free_list (state->htags);
}

static gchar *elide_whitespace (const gchar *instr, gint len);

// Tags to support: <p> and not </p>:
// either 'elide' \ns (turn off with <pre> I guess
static void
rt_start_element (GMarkupParseContext *context,
                  const gchar         *element_name,
                  const gchar        **attribute_names,
                  const gchar        **attribute_values,
                  gpointer             user_data,
                  GError             **error)
{	// Called for open tags <foo bar="baz">
	if (!g_ascii_strcasecmp (element_name, "body"))
		return;

	GRTParseState *state = (GRTParseState*) user_data;
	GRTPTag *tag = g_malloc (sizeof (GRTPTag));
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter (state->buffer, &iter);
	tag->mark = gtk_text_buffer_create_mark (state->buffer, NULL, &iter, TRUE);

	if (!g_ascii_strcasecmp (element_name, "pre"))
		state->pre_mode = TRUE;

	// Check if this is a block tag
	if (isBlockTag (element_name)) {
		// make sure this opens a new paragraph
		if (!gtk_text_iter_starts_line (&iter)) {
			gtk_text_buffer_insert (state->buffer, &iter, "\n", -1);
			gtk_text_buffer_get_end_iter (state->buffer, &iter);
		}
	}
	state->closed_block_tag = FALSE;

	char *lower = g_ascii_strdown (element_name, -1);
	tag->tag = gtk_text_tag_table_lookup (state->tags, lower);

	// Special tags that must be inserted manually
	if (!tag->tag) {
		if (!g_ascii_strcasecmp (element_name, "font")) {
			if (attribute_names[0] &&
			    !g_ascii_strcasecmp (attribute_names[0], "color")) {
				tag->tag = gtk_text_buffer_create_tag (state->buffer, NULL,
				                                       "foreground", attribute_values[0],
				                                       NULL);
				state->default_color = FALSE;
			}
#ifdef PRINT_WARNINGS
			else
				g_warning ("Unknown font attribute: '%s'", attribute_names[0]);
#endif
		}
		else if (!g_ascii_strcasecmp (element_name, "a")) {
			if (attribute_names[0] &&
			    !g_ascii_strcasecmp (attribute_names[0], "href")) {
				tag->tag = gtk_text_buffer_create_tag (state->buffer, NULL,
				                                       "underline", PANGO_UNDERLINE_SINGLE,
				                                       NULL);
				if (state->default_color)
					g_object_set (tag->tag, "foreground-gdk", &link_color, NULL);
				g_object_set_data (G_OBJECT (tag->tag), "link", g_strdup (attribute_values[0]));
			}
#ifdef PRINT_WARNINGS
			else
				g_warning ("Unknown a attribute: '%s'", attribute_names[0]);
#endif
		}
		else if (!g_ascii_strcasecmp (element_name, "li")) {
			HTMLList *front_list;

			if (state->html_list &&
			    (front_list = g_list_first (state->html_list)->data) &&
			    (front_list->ordered)) {;
				gchar *str = g_strdup_printf ("%d. ", front_list->enumeration++);
				gtk_text_buffer_insert (state->buffer, &iter, str, -1);
				g_free (str);
			}
			else {                           // \\u25cf for bigger bullets
				gtk_text_buffer_insert (state->buffer, &iter, "\u2022 ", -1);
			}
		}
		// Tags that affect the margin
		else if (!g_ascii_strcasecmp (element_name, "ul") ||
		         !g_ascii_strcasecmp (element_name, "ol")) {
			HTMLList *list = g_malloc (sizeof (HTMLList));
			HTMLList_init (list, !g_ascii_strcasecmp (element_name, "ol"));
			state->html_list = g_list_append (state->html_list, list);
		}

		else if (!g_ascii_strcasecmp (element_name, "img")) {
			if (attribute_names[0] &&
			    !g_ascii_strcasecmp (attribute_names[0], "src")) {
				GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (attribute_values[0], NULL);
				if (pixbuf) {
					gtk_text_buffer_insert_pixbuf (state->buffer, &iter, pixbuf);
					g_object_unref (G_OBJECT (pixbuf));
				}
			}
#ifdef PRINT_WARNINGS
			else
				g_warning ("Unknown img attribute: '%s'", attribute_names[0]);
#endif
		}

		// for tags like <br/>, GMarkup will pass them through the end
		// tag callback too, so we'll deal with them there
		else if (!g_ascii_strcasecmp (element_name, "br"))
			;

		else
		{
#ifdef PRINT_WARNINGS
			if (isBlockTag (element_name))
				;
			else
				g_warning ("Unknown tag '%s'", element_name);
#endif
		}
	}

	if (!tag->tag && isIdentTag (element_name)) {
		state->left_margin += IDENT_MARGIN;
		tag->tag = gtk_text_buffer_create_tag (state->buffer, NULL,
		                   "left-margin", state->left_margin, NULL);
	}

	g_free (lower);
	state->htags = g_list_append (state->htags, tag);
}

static void
rt_end_element (GMarkupParseContext *context,
                const gchar         *element_name,
                gpointer             user_data,
                GError             **error)
{	// Called for close tags </foo>
	if (!g_ascii_strcasecmp (element_name, "body"))
		return;

	GRTParseState *state = (GRTParseState*) user_data;

	if (g_list_length (state->htags) == 0) {
		g_warning ("Urgh - empty tag queue closing '%s'", element_name);
		return;
	}

	g_return_if_fail (state->htags != NULL);
	GRTPTag *tag = g_list_last (state->htags)->data;
	state->htags = g_list_remove (state->htags, tag);

	GtkTextIter start, end;
	gtk_text_buffer_get_iter_at_mark (state->buffer, &start, tag->mark);
	gtk_text_buffer_get_end_iter (state->buffer, &end);

	gint appendLines = 0;

	if (isIdentTag (element_name))
		state->left_margin -= IDENT_MARGIN;

	if (!g_ascii_strcasecmp (element_name, "ul") ||
	   !g_ascii_strcasecmp (element_name, "ol")) {
		HTMLList *last_list = g_list_last (state->html_list)->data;
		state->html_list = g_list_remove (state->html_list, last_list);
		g_free (last_list);
	}
	else if (!g_ascii_strcasecmp (element_name, "font"))
		state->default_color = TRUE;

	else if (!g_ascii_strcasecmp (element_name, "pre"))
		state->pre_mode = FALSE;

	if (isBlockTag (element_name) || !g_ascii_strcasecmp (element_name, "br")) {
		appendLines = 1;
		state->closed_block_tag = TRUE;
	}
	else
		state->closed_block_tag = FALSE;

	if (appendLines) {
		gtk_text_buffer_insert (state->buffer, &end,
		                        appendLines == 1 ? "\n" : "\n\n", -1);
		gtk_text_buffer_get_iter_at_mark (state->buffer, &start, tag->mark);
		gtk_text_buffer_get_end_iter (state->buffer, &end);
	}

	if (tag->tag)
		gtk_text_buffer_apply_tag (state->buffer, tag->tag, &start, &end);

	gtk_text_buffer_delete_mark (state->buffer, tag->mark);
	g_free (tag);
}

static void
rt_text (GMarkupParseContext *context,
         const gchar         *text,
         gsize                text_len,
         gpointer             user_data,
         GError             **error)
{  // Called for character data, NB. text NOT nul-terminated
	GRTParseState *state = (GRTParseState*) user_data;
	GtkTextIter start, end;
	gtk_text_buffer_get_end_iter (state->buffer, &start);
	if (state->pre_mode)
		gtk_text_buffer_insert_with_tags (state->buffer, &start,
		                                  text, text_len, NULL, NULL);
	else {
		if (!state->closed_block_tag) {
			if (g_ascii_isspace (text[0]))
				gtk_text_buffer_insert (state->buffer, &start, " ", 1);
		}
		state->closed_block_tag = FALSE;

		gchar *real = elide_whitespace (text, text_len);
		gtk_text_buffer_insert (state->buffer, &start, real, -1);
		g_free (real);
	}
	gtk_text_buffer_get_end_iter (state->buffer, &end);
}

static void
rt_passthrough (GMarkupParseContext *context,
                const gchar         *passthrough_text,
                gsize                text_len,
                gpointer             user_data,
                GError             **error)
{
	// ignore comments etc.
}

static void
rt_error (GMarkupParseContext *context,
          GError              *error,
          gpointer             user_data)
{
}

static GMarkupParser rt_parser = {
	rt_start_element,
	rt_end_element,
	rt_text,
	rt_passthrough,
	rt_error
};

GtkWidget *ygtk_rich_text_new (void)
{
	return g_object_new (YGTK_TYPE_RICH_TEXT, NULL);
}

/* String preparation methods. */
static gchar *elide_whitespace (const gchar *instr, gint len)
{
	GString *dest = g_string_new ("");
	if (len < 0)
		len = strlen (instr);
	// collapse more than one space in one
	gboolean start_text = TRUE, is_white = FALSE;
	gint i;
	for (i = 0; i < len; i++) {
		gchar ch = instr[i];
		if (g_ascii_isspace (ch)) {
			if (!is_white && !start_text)
				g_string_append_c (dest, ' ');
			is_white = TRUE;
		}
		else {
			g_string_append_c (dest, ch);
			is_white = FALSE;
			start_text = FALSE;
		}
	}
	return g_string_free (dest, FALSE);
}

void ygtk_rich_text_set_text (YGtkRichText* rtext, const gchar* text, gboolean plain_mode)
{
	GtkTextBuffer *buffer;
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (rtext));

	if (plain_mode) {
		gtk_text_buffer_set_text (buffer, text, -1);
		return;
	}

	// remove any possible existing text
	gtk_text_buffer_set_text (buffer, "", 0);

	GRTParseState state;
	GRTParseState_init (&state, buffer);

	GMarkupParseContext *ctx;
	ctx = g_markup_parse_context_new (&rt_parser, (GMarkupParseFlags)0, &state, NULL);

	char *xml = ygutils_convert_to_xhmlt_and_subst (text);
	GError *error = NULL;
	if (!g_markup_parse_context_parse (ctx, xml, -1, &error)) {
#ifdef PRINT_WARNINGS
		g_warning ("Markup parse error '%s'", error ? error->message : "Unknown");
#endif
	}
	g_free (xml);

	g_markup_parse_context_free (ctx);
	GRTParseState_free (&state);

	// remove last empty line, if any
	GtkTextIter end_it, before_end_it;
	gtk_text_buffer_get_end_iter (buffer, &end_it);
	before_end_it = end_it;
	if (gtk_text_iter_backward_char (&before_end_it) &&
	    gtk_text_iter_get_char (&before_end_it) == '\n')
		gtk_text_buffer_delete (buffer, &before_end_it, &end_it);
}

/* gtk_text_iter_forward_search() is case-sensitive so we roll our own.
   The idea is to keep use get_text and strstr there, but to be more
   efficient we check per line. */
static gboolean ygtk_rich_text_forward_search (const GtkTextIter *begin,
	const GtkTextIter *end, const gchar *_key, GtkTextIter *match_start,
	GtkTextIter *match_end)
{
	if (*_key == 0)
		return FALSE;

	/* gtk_text_iter_get_char() returns a gunichar (ucs4 coding), so we
	   convert the given string (which is utf-8, like anyhting in gtk+) */
	gunichar *key = g_utf8_to_ucs4 (_key, -1, NULL, NULL, NULL);
	if (!key)  // conversion error -- should not happen
		return FALSE;

	// convert key to lower case, to avoid work later
	gunichar *k;
	for (k = key; *k; k++)
		*k = g_unichar_tolower (*k);

	GtkTextIter iter = *begin, iiter;
	while (!gtk_text_iter_is_end (&iter) && gtk_text_iter_compare (&iter, end) <= 0) {
		iiter = iter;
		for (k = key; *k == g_unichar_tolower (gtk_text_iter_get_char (&iiter)) && (*k);
		     k++, gtk_text_iter_forward_char (&iiter))
			;
		if (!*k) {
			*match_start = iter;
			*match_end = iiter;
			return TRUE;
		}
		gtk_text_iter_forward_char (&iter);
	}
	return FALSE;
}

gboolean ygtk_rich_text_mark_text (YGtkRichText *rtext, const gchar *text)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (rtext));
	GtkTextIter iter, end, match_start, match_end;

	gtk_text_buffer_get_bounds (buffer, &iter, &end);
	gtk_text_buffer_remove_tag_by_name (buffer, "keyword", &iter, &end);

	gtk_text_buffer_select_range (buffer, &iter, &iter);  // unselect text
	if (!text || *text == '\0')
		return TRUE;

	gboolean found = FALSE;
	while (ygtk_rich_text_forward_search (&iter, &end, text,
	                                     &match_start, &match_end)) {
		found = TRUE;
		gtk_text_buffer_apply_tag_by_name (buffer, "keyword", &match_start, &match_end);
		iter = match_end;
		gtk_text_iter_forward_char (&iter);
	}
	return found;
}

gboolean ygtk_rich_text_forward_mark (YGtkRichText *rtext, const gchar *text)
{
	GtkTextIter start_iter, end_iter;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (rtext));
	gtk_text_buffer_get_iter_at_mark (buffer, &start_iter,
	                                  gtk_text_buffer_get_selection_bound (buffer));
	gtk_text_buffer_get_end_iter (buffer, &end_iter);

	gboolean found;
	found = ygtk_rich_text_forward_search (&start_iter, &end_iter, text,
	                                      &start_iter, &end_iter);
	if (!found) {
		gtk_text_buffer_get_start_iter (buffer, &start_iter);
		found = ygtk_rich_text_forward_search (&start_iter, &end_iter, text,
		                                      &start_iter, &end_iter);
	}

	if (found) {
		gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (rtext), &start_iter, 0.10,
		                              FALSE, 0, 0);
		gtk_text_buffer_select_range (buffer, &start_iter, &end_iter);
		return TRUE;
	}
	return FALSE;
}

void ygtk_rich_text_class_init (YGtkRichTextClass *klass)
{
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	gtkwidget_class->realize = ygtk_rich_text_realize;
	gtkwidget_class->motion_notify_event = ygtk_rich_text_motion_notify_event;
	gtkwidget_class->expose_event = ygtk_rich_text_expose_event;

	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	gtkobject_class->destroy = ygtk_rich_text_destroy;

	link_clicked_signal = g_signal_new ("link-clicked",
		G_TYPE_FROM_CLASS (G_OBJECT_CLASS (klass)), G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (YGtkRichTextClass, link_clicked), NULL, NULL,
		g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

