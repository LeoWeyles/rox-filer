/* vi: set cindent:
 * $Id$
 *
 * ROX-Filer, filer for the ROX desktop project
 * Copyright (C) 1999, Thomas Leonard, <tal197@ecs.soton.ac.uk>.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <gtk/gtk.h>
#include "collection.h"

#include "main.h"
#include "gui_support.h"
#include "filer.h"
#include "mount.h"
#include "menu.h"
#include "dnd.h"
#include "options.h"
#include "choices.h"
#include "savebox.h"
#include "type.h"

int number_of_windows = 0;	/* Quit when this reaches 0 again... */

static void child_died(int signum)
{
	int	    	status;
	int	    	child;

	/* Find out which children exited and allow them to die */
	do
	{
		child = waitpid(-1, &status, WNOHANG);

		if (child == 0 || child == -1)
			return;

		/* fprintf(stderr, "Child %d exited\n", child); */

	} while (1);
}

#define BUFLEN 40
void stderr_cb(gpointer data, gint source, GdkInputCondition condition)
{
	char buf[BUFLEN];
	static GtkWidget *log = NULL;
	static GtkWidget *window = NULL;
	ssize_t len;

	if (!window)
	{
		window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gtk_window_set_title(GTK_WINDOW(window), "ROX-Filer error log");
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
		gtk_window_set_default_size(GTK_WINDOW(window), 600, 300);
		gtk_signal_connect_object(GTK_OBJECT(window), "delete_event",
				gtk_widget_hide, GTK_OBJECT(window));
		log = gtk_text_new(NULL, NULL);
		gtk_container_add(GTK_CONTAINER(window), log);
	}

	if (!GTK_WIDGET_MAPPED(window))
		gtk_widget_show_all(window);
	
	len = read(source, buf, BUFLEN);
	if (len > 0)
		gtk_text_insert(GTK_TEXT(log), NULL, NULL, NULL, buf, len);
}

int main(int argc, char **argv)
{
	int		 stderr_pipe[2];
	struct sigaction act;

	gtk_init(&argc, &argv);
	choices_init("ROX-Filer");

	gui_support_init();
	menu_init();
	dnd_init();
	filer_init();
	mount_init();
	options_init();
	savebox_init();
	type_init();

	options_load();

	/* Let child processes die */
	act.sa_handler = child_died;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &act, NULL);

	if (geteuid() == 0)
	{
		if (get_choice("!!!DANGER!!!",
			"Running ROX-Filer as root is VERY dangerous. If I "
			"had a warranty (I don't) then doing this would "
			"void it", 2,
			"Don't click here", "Quit") != 0)
			exit(EXIT_SUCCESS);
	}

	if (argc < 2)
		filer_opendir(getenv("HOME"), FALSE, BOTTOM);
	else
	{
		int	 i = 1;
		gboolean panel = FALSE;
		Side	 side = BOTTOM;

		while (i < argc)
		{
			if (argv[i][0] == '-')
			{
				switch (argv[i][1] + (argv[i][2] << 8))
				{
					case 't': side = TOP; break;
					case 'b': side = BOTTOM; break;
					case 'l': side = LEFT; break;
					case 'r': side = RIGHT; break;
					default:
						fprintf(stderr,
							"Bad option.\n");
						return EXIT_FAILURE;
				}
				panel = TRUE;
			}
			else
			{
				filer_opendir(argv[i], panel, side);
				panel = FALSE;
				side = BOTTOM;
			}
			i++;
		}
	}

	pipe(stderr_pipe);
	dup2(stderr_pipe[1], STDERR_FILENO);
	gdk_input_add(stderr_pipe[0], GDK_INPUT_READ, stderr_cb, NULL);
	fcntl(STDERR_FILENO, F_SETFD, 0);

	gtk_main();

	return EXIT_SUCCESS;
}
