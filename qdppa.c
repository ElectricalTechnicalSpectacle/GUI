#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/time.h>
#include "csv.h"

#define MEMORY_FILE_PATH    "/media/MICROSD/"
//#define MEMORY_FILE_PATH    "/home/abdulrahman/Dev/QDPPA/"

struct value {
    gint power;
    gdouble timestamp;
};

GArray *values;
gboolean is_capturing = FALSE;

gboolean update_label(gpointer label_numbers) {
    gchar *buff;
	int count;
    struct value val;
    struct timeval tv;
    
	count = atoi(gtk_label_get_text(label_numbers)) + 1;
	gettimeofday(&tv, NULL);
	
	val.power = count;
    val.timestamp = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
	
    buff = g_strdup_printf ("%d", count);
	gtk_label_set_text(label_numbers, buff);

    if(is_capturing)
        g_array_append_val(values, val);
    
    g_free(buff);

	if (count < 5000)
		return TRUE;
	else
		return FALSE;
}

void start_stop_listen_usb(GtkWidget *widget, gpointer data) {
	static gint func_ref = 0;
	GtkWidget *label_numbers;
	GtkWidget *toggle_button_usd;

	label_numbers = g_object_get_data(G_OBJECT(widget), "label_numbers");
	toggle_button_usd = g_object_get_data(G_OBJECT(widget), "toggle_button_usd");

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
	    gtk_widget_set_sensitive(toggle_button_usd, TRUE);
		func_ref = g_timeout_add(150, update_label, GTK_LABEL(label_numbers));
	} else {
	    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button_usd))) {
	        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button_usd), FALSE);
	    }
	    gtk_widget_set_sensitive(toggle_button_usd, FALSE);
		g_source_remove(func_ref);
	}
}

void create_csv_file() {
    CSV_BUFFER *my_buffer = csv_create_buffer();
    
    for(int i = 0; i < values->len; ++i) {
        struct value val;
        val = g_array_index(values, struct value, i);
        
        gchar *time_value = g_strdup_printf("%f", val.timestamp);
        gchar *power_value = g_strdup_printf("%d", val.power);
        
	    csv_set_field(my_buffer, i, 0, time_value);
	    csv_set_field(my_buffer, i, 1, power_value);
	    
	    g_free(time_value);
        g_free(power_value);
    }
    
    int count = 0;
    gchar *file_name = "data";
    gchar *extension = "csv";
    gchar *fname = "";
    fname = g_strdup_printf("%s%s-%d.%s", MEMORY_FILE_PATH, file_name, count++, extension);
    while(access(fname, F_OK) != -1)  {
        g_free(fname);
        fname = g_strdup_printf("%s%s-%d.%s", MEMORY_FILE_PATH, file_name, count++, extension);
    }
    csv_save(fname, my_buffer);
    g_free(fname);
    
    csv_destroy_buffer(my_buffer);
}

void start_stop_write_usd(GtkWidget *widget, gpointer data) {
    if(!g_file_test(MEMORY_FILE_PATH, G_FILE_TEST_IS_DIR)) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
        } else {
            return;
        }
        GtkWidget *window;
        GtkWidget *dialog;
        window = gtk_widget_get_toplevel(widget);
        dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_CLOSE,
                                        "Error opening SD card");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
        values = g_array_new(FALSE, FALSE, sizeof(struct value));
        is_capturing = TRUE;
	} else {
	    if(is_capturing) {
	        is_capturing = FALSE;
	        create_csv_file();
	        g_array_free(values, TRUE);
	    }
	}
}

void init_gui() {
    GtkWidget *window;
    GtkWidget *vbox_main;
    GtkWidget *hbox_labels;
    GtkWidget *hbox_buttons;
    GtkWidget *label_power;
    GtkWidget *label_numbers;
    GtkWidget *label_power_unit;
    GtkWidget *button_close;
    GtkWidget *toggle_button_capture;
    GtkWidget *toggle_button_usd;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
    gtk_window_set_title(GTK_WINDOW(window), "QDPPA");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 230, 250);
    gtk_container_set_border_width(GTK_CONTAINER(window), 5);
    gtk_window_maximize(GTK_WINDOW(window));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    vbox_main = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_container_add(GTK_CONTAINER(window), vbox_main);
    
    hbox_labels = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    hbox_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    gtk_widget_set_valign(hbox_buttons, GTK_ALIGN_END);
    gtk_widget_set_valign(hbox_labels, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox_main), hbox_labels, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_main), hbox_buttons, TRUE, TRUE, 0);
    
    label_power = gtk_label_new("Power");
    label_numbers = gtk_label_new("1");
    label_power_unit = gtk_label_new("W");
    gtk_box_pack_start(GTK_BOX(hbox_labels), label_power, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels), label_numbers, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels), label_power_unit, TRUE, FALSE, 0);
    
    toggle_button_usd = gtk_toggle_button_new_with_label("Write to SD Card");
    gtk_widget_set_sensitive(toggle_button_usd, FALSE);
    toggle_button_capture = gtk_toggle_button_new_with_label("Start Capture");
    button_close = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), toggle_button_usd, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), toggle_button_capture, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), button_close, TRUE, TRUE, 6);
    gtk_widget_set_halign(toggle_button_usd, GTK_ALIGN_START);
    gtk_widget_set_halign(toggle_button_capture, GTK_ALIGN_START);
    gtk_widget_set_halign(button_close, GTK_ALIGN_END);
    
    g_object_set_data(G_OBJECT(toggle_button_capture), "label_numbers", (gpointer) label_numbers);
    g_object_set_data(G_OBJECT(toggle_button_capture), "toggle_button_usd", (gpointer) toggle_button_usd);

    g_signal_connect(G_OBJECT(button_close), "clicked", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(toggle_button_capture), "toggled", G_CALLBACK(start_stop_listen_usb), NULL);
    g_signal_connect(G_OBJECT(toggle_button_usd), "toggled", G_CALLBACK(start_stop_write_usd), NULL);
    
    gtk_widget_show_all(window);
}

int main(int argc, char *argv[]) {
	gtk_init(&argc, &argv);
	
    init_gui();
	gtk_main();
	
	return 0;
}
