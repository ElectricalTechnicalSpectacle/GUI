#define _BSD_SOURCE
#define _POSIX_SOURCE

#define PORT           59481
#define BUFF_SIZE       1048576

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/time.h>
#include "csv.h"

#define MEMORY_FILE_PATH    "/media/MICROSD/"
//#define MEMORY_FILE_PATH    "/home/abdulrahman/Dev/QDPPA-CheckOff/"

struct value {
    gchar *power;
    gchar *voltage;
    gchar *current;
    gchar *state;
};

struct reading {
    gchar *numbers_power;
    gchar *numbers_accuracy_power;
    gchar *power_unit;
    
    gchar *numbers_voltage;
    gchar *numbers_accuracy_voltage;
    gchar *voltage_unit;
    
    gchar *numbers_current;
    gchar *numbers_accuracy_current;
    gchar *current_unit;
};

struct reading last_reading;

int socket_fd;

GArray *values;
gboolean is_capturing = FALSE;
gboolean is_listening = FALSE;

gboolean init_connection() {
    struct sockaddr_in servaddr;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        //perror("Problem creating the socket");
        return FALSE;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    //printf("Establishing connection to the server...\n");
    if (connect(socket_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        //perror("Problem connecting to the server");
        close(socket_fd);
        return FALSE;
    }
    
    listen(socket_fd , 3);
    return TRUE;
}

void send_test(gchar *command) {
    char sendline[BUFF_SIZE] = "";
    sprintf(sendline, "%s", command);
    if (send(socket_fd, sendline, strlen(sendline), 0) == 0) {
        //perror("Could not send data to server.");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
}

void recv_test(char *buff) {
    if (recv(socket_fd, buff, BUFF_SIZE, 0) == 0) {
        //perror("The server terminated prematurely.");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
}

gboolean update_labels(gpointer labels) {
    GtkLabel *label_numbers;
    GtkLabel *label_numbers_voltage;
    GtkLabel *label_numbers_current;
    GtkLabel *label_numbers_accuracy;
    GtkLabel *label_numbers_accuracy_voltage;
    GtkLabel *label_numbers_accuracy_current;
    GtkLabel *label_power_unit;
    GtkLabel *label_voltage_unit;
    GtkLabel *label_current_unit;
	
    label_numbers = g_object_get_data(G_OBJECT(labels), "label_numbers");
    label_numbers_voltage = g_object_get_data(G_OBJECT(labels), "label_numbers_voltage");
    label_numbers_current = g_object_get_data(G_OBJECT(labels), "label_numbers_current");
    label_numbers_accuracy = g_object_get_data(G_OBJECT(labels), "label_numbers_accuracy");
    label_numbers_accuracy_voltage = g_object_get_data(G_OBJECT(labels), "label_numbers_accuracy_voltage");
    label_numbers_accuracy_current = g_object_get_data(G_OBJECT(labels), "label_numbers_accuracy_current");
    label_power_unit = g_object_get_data(G_OBJECT(labels), "label_power_unit");
    label_voltage_unit = g_object_get_data(G_OBJECT(labels), "label_voltage_unit");
    label_current_unit = g_object_get_data(G_OBJECT(labels), "label_current_unit");
    
    gtk_label_set_text(label_numbers, last_reading.numbers_power);
    gtk_label_set_text(label_numbers_voltage, last_reading.numbers_voltage);
    gtk_label_set_text(label_numbers_current, last_reading.numbers_current);    
    gtk_label_set_text(label_numbers_accuracy, last_reading.numbers_accuracy_power);
    gtk_label_set_text(label_numbers_accuracy_voltage, last_reading.numbers_accuracy_voltage);
    gtk_label_set_text(label_numbers_accuracy_current, last_reading.numbers_accuracy_current);
    gtk_label_set_text(label_power_unit, last_reading.power_unit);
    gtk_label_set_text(label_voltage_unit, last_reading.voltage_unit);
    gtk_label_set_text(label_current_unit, last_reading.current_unit);
        
    g_free(last_reading.numbers_power);
    g_free(last_reading.numbers_voltage);
    g_free(last_reading.numbers_current);
    g_free(last_reading.numbers_accuracy_power);
    g_free(last_reading.numbers_accuracy_voltage);
    g_free(last_reading.numbers_accuracy_current);
    g_free(last_reading.power_unit);
    g_free(last_reading.voltage_unit);
    g_free(last_reading.current_unit);

    return TRUE;
}

gboolean listen_for_data() {
    gchar buff[BUFF_SIZE] = "";
    gchar **arr_string;
    gchar **full_readings;
    gchar **individual_reading;
    gchar **voltage_reading;
    gchar **current_reading;
    gchar **power_reading;
    gchar *command = "";
        
    struct value val;
    
    command = "get";
    send_test(command);
    recv_test(buff);
    
    if (g_strcmp0(buff, "") != 0) {
        arr_string = g_strsplit(buff, "|", 0);
        full_readings = g_strsplit(arr_string[1], "~", 0);
        for(int i = 0; i < atoi(arr_string[0]); i++) {
            individual_reading = g_strsplit(full_readings[i], "&", 0);
            
            voltage_reading = g_strsplit(individual_reading[0], ",", 0);
            current_reading = g_strsplit(individual_reading[1], ",", 0);
            power_reading = g_strsplit(individual_reading[2], ",", 0);
	        
	        if (is_capturing) {
	            val.power = g_strconcat(power_reading[0], ".", power_reading[1], " ", power_reading[2], NULL);
	            val.voltage = g_strconcat(voltage_reading[0], ".", voltage_reading[1], " ", voltage_reading[2], NULL);
	            val.current = g_strconcat(current_reading[0], ".", current_reading[1], " ", current_reading[2], NULL);
	            val.state = g_strdup(individual_reading[3]);
                
                g_array_append_val(values, val);
            }
        }
        last_reading.numbers_power = g_strdup(power_reading[0]);
        last_reading.numbers_voltage = g_strdup(voltage_reading[0]);
        last_reading.numbers_current = g_strdup(current_reading[0]);
        
        last_reading.numbers_accuracy_power = g_strdup(power_reading[1]);
        last_reading.numbers_accuracy_voltage = g_strdup(voltage_reading[1]);
        last_reading.numbers_accuracy_current = g_strdup(current_reading[1]);
        
        last_reading.power_unit = g_strdup(power_reading[2]);
        last_reading.voltage_unit = g_strdup(voltage_reading[2]);
        last_reading.current_unit = g_strdup(current_reading[2]);
        
        g_strfreev(arr_string);
        g_strfreev(full_readings);
        g_strfreev(individual_reading);
        g_strfreev(voltage_reading);
        g_strfreev(current_reading);
        g_strfreev(power_reading);
    } else {
        last_reading.numbers_power = g_strdup("0");
        last_reading.numbers_voltage = g_strdup("0");
        last_reading.numbers_current = g_strdup("0");
        
        last_reading.numbers_accuracy_power = g_strdup("0");
        last_reading.numbers_accuracy_voltage = g_strdup("0");
        last_reading.numbers_accuracy_current = g_strdup("0");
        
        last_reading.power_unit = g_strdup("W");
        last_reading.voltage_unit = g_strdup("V");
        last_reading.current_unit = g_strdup("A");
    }
    return TRUE;
}

void start_stop_listen_usb(GtkWidget *widget, gpointer data) {
	static gint func_update_ref = 0;
	static gint func_listen_ref = 0;
	gboolean connection_status = TRUE;
	GtkWidget *toggle_button_usd;
    GtkWidget *window;
    GtkWidget *dialog;
    
	toggle_button_usd = g_object_get_data(G_OBJECT(widget), "toggle_button_usd");

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
	    connection_status = init_connection();
	    if(connection_status){
	        is_listening = TRUE;
	        gtk_widget_set_sensitive(toggle_button_usd, TRUE);
	        func_listen_ref = g_timeout_add(50, listen_for_data, G_OBJECT(widget));
		    func_update_ref = g_timeout_add(150, update_labels, G_OBJECT(widget));
	    } else {
	        is_listening = FALSE;
            window = gtk_widget_get_toplevel(widget);
            dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                            GTK_MESSAGE_ERROR,
                                            GTK_BUTTONS_CLOSE,
                                            "Error establishing socket communication");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
	    	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
	            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
	        }
	    }
	} else {
	    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle_button_usd))) {
	        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle_button_usd), FALSE);
	    }
	    gtk_widget_set_sensitive(toggle_button_usd, FALSE);
	    if (is_listening)
	        close(socket_fd);
	    
	    if (func_listen_ref)
	        g_source_remove(func_listen_ref);
	    if (func_update_ref)
	        g_source_remove(func_update_ref);
	}
}

void create_csv_file() {
    CSV_BUFFER *my_buffer = csv_create_buffer();
    
    for(int i = 0; i < values->len; ++i) {
        struct value val;
        val = g_array_index(values, struct value, i);
        
        gchar *power_value = g_strdup_printf("%s", val.power);
        gchar *voltage_value = g_strdup_printf("%s", val.voltage);
        gchar *current_value = g_strdup_printf("%s", val.current);
        gchar *state_value = g_strdup_printf("%s", val.state);
        
	    csv_set_field(my_buffer, i, 0, power_value);
	    csv_set_field(my_buffer, i, 1, voltage_value);
	    csv_set_field(my_buffer, i, 2, current_value);
	    csv_set_field(my_buffer, i, 3, state_value);
	    
	    g_free(power_value);
        g_free(voltage_value);
        g_free(current_value);
        g_free(state_value);
        
        g_free(val.power);
        g_free(val.voltage);
        g_free(val.current);
        g_free(val.state);
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
    GtkWidget *hbox_labels_voltage;
    GtkWidget *hbox_labels_current;
    GtkWidget *label_power;
    GtkWidget *label_voltage;
    GtkWidget *label_current;
    
    GtkWidget *label_numbers;
    GtkWidget *label_numbers_separator;
    GtkWidget *label_numbers_accuracy;
    GtkWidget *label_power_unit;
    
    GtkWidget *label_numbers_voltage;
    GtkWidget *label_numbers_separator_voltage;
    GtkWidget *label_numbers_accuracy_voltage;
    GtkWidget *label_voltage_unit;
    
    GtkWidget *label_numbers_current;
    GtkWidget *label_numbers_separator_current;
    GtkWidget *label_numbers_accuracy_current;
    GtkWidget *label_current_unit;
    
    
    GtkWidget *button_close;
    GtkWidget *toggle_button_capture;
    GtkWidget *toggle_button_usd;
    
    PangoAttrList *attrs_power = pango_attr_list_new();
    PangoAttrList *attrs_voltage_current = pango_attr_list_new();
    
    PangoAttribute *attr_weight = pango_attr_weight_new(PANGO_WEIGHT_BOLD);
    PangoAttribute *attr_size = pango_attr_size_new(14*PANGO_SCALE);
    PangoAttribute *attr_size_voltage_current = pango_attr_size_new(12*PANGO_SCALE);
    
    pango_attr_list_insert (attrs_power, attr_weight);
    pango_attr_list_insert (attrs_power, attr_size);
    
    pango_attr_list_insert (attrs_voltage_current, attr_size_voltage_current);

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
    hbox_labels_voltage = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    hbox_labels_current = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    hbox_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    gtk_widget_set_valign(hbox_buttons, GTK_ALIGN_END);
    gtk_widget_set_valign(hbox_labels, GTK_ALIGN_START);
    gtk_widget_set_valign(hbox_labels_voltage, GTK_ALIGN_START);
    gtk_widget_set_valign(hbox_labels_current, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(vbox_main), hbox_labels, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_main), hbox_labels_voltage, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_main), hbox_labels_current, TRUE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_main), hbox_buttons, TRUE, TRUE, 0);
    
    label_power = gtk_label_new("Power: ");
    gtk_label_set_attributes (GTK_LABEL(label_power), attrs_power);
    label_numbers = gtk_label_new("00000");
    gtk_label_set_attributes (GTK_LABEL(label_numbers), attrs_power);
    label_numbers_separator = gtk_label_new(".");
    gtk_label_set_attributes (GTK_LABEL(label_numbers_separator), attrs_power);
    label_numbers_accuracy = gtk_label_new("00");
    gtk_label_set_attributes (GTK_LABEL(label_numbers_accuracy), attrs_power);
    label_power_unit = gtk_label_new("W");
    gtk_label_set_attributes (GTK_LABEL(label_power_unit), attrs_power);
    gtk_box_pack_start(GTK_BOX(hbox_labels), label_power, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels), label_numbers, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels), label_numbers_separator, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels), label_numbers_accuracy, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels), label_power_unit, FALSE, FALSE, 30);
    
    label_voltage = gtk_label_new("Voltage: ");
    gtk_label_set_attributes (GTK_LABEL(label_voltage), attrs_voltage_current);
    label_numbers_voltage = gtk_label_new("00000");
    gtk_label_set_attributes (GTK_LABEL(label_numbers_voltage), attrs_voltage_current);
    label_numbers_separator_voltage = gtk_label_new(".");
    gtk_label_set_attributes (GTK_LABEL(label_numbers_separator_voltage), attrs_voltage_current);
    label_numbers_accuracy_voltage = gtk_label_new("00");
    gtk_label_set_attributes (GTK_LABEL(label_numbers_accuracy_voltage), attrs_voltage_current);
    label_voltage_unit = gtk_label_new("V");
    gtk_label_set_attributes (GTK_LABEL(label_voltage_unit), attrs_voltage_current);
    gtk_box_pack_start(GTK_BOX(hbox_labels_voltage), label_voltage, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels_voltage), label_numbers_voltage, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels_voltage), label_numbers_separator_voltage, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels_voltage), label_numbers_accuracy_voltage, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels_voltage), label_voltage_unit, FALSE, FALSE, 30);
    
    label_current = gtk_label_new("Current: ");
    gtk_label_set_attributes (GTK_LABEL(label_current), attrs_voltage_current);
    label_numbers_current = gtk_label_new("00000");
    gtk_label_set_attributes (GTK_LABEL(label_numbers_current), attrs_voltage_current);
    label_numbers_separator_current = gtk_label_new(".");
    gtk_label_set_attributes (GTK_LABEL(label_numbers_separator_current), attrs_voltage_current);
    label_numbers_accuracy_current = gtk_label_new("00");
    gtk_label_set_attributes (GTK_LABEL(label_numbers_accuracy_current), attrs_voltage_current);
    label_current_unit = gtk_label_new("A");
    gtk_label_set_attributes (GTK_LABEL(label_current_unit), attrs_voltage_current);
    gtk_box_pack_start(GTK_BOX(hbox_labels_current), label_current, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels_current), label_numbers_current, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels_current), label_numbers_separator_current, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels_current), label_numbers_accuracy_current, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_labels_current), label_current_unit, FALSE, FALSE, 30);
    
    
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
    g_object_set_data(G_OBJECT(toggle_button_capture), "label_numbers_accuracy", (gpointer) label_numbers_accuracy);
    g_object_set_data(G_OBJECT(toggle_button_capture), "label_power_unit", (gpointer) label_power_unit);
    
    g_object_set_data(G_OBJECT(toggle_button_capture), "label_numbers_current", (gpointer) label_numbers_current);
    g_object_set_data(G_OBJECT(toggle_button_capture), "label_numbers_accuracy_current", (gpointer) label_numbers_accuracy_current);
    g_object_set_data(G_OBJECT(toggle_button_capture), "label_current_unit", (gpointer) label_current_unit);
    
    
    g_object_set_data(G_OBJECT(toggle_button_capture), "label_numbers_voltage", (gpointer) label_numbers_voltage);
    g_object_set_data(G_OBJECT(toggle_button_capture), "label_numbers_accuracy_voltage", (gpointer) label_numbers_accuracy_voltage);
    g_object_set_data(G_OBJECT(toggle_button_capture), "label_voltage_unit", (gpointer) label_voltage_unit);
    
    
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
