/*
 * RhythmCat Main Module
 * The main module for the player.
 *
 * rc-main.c
 * This file is part of RhythmCat Music Player (GTK+ Version)
 *
 * Copyright (C) 2012 - SuperCat, license: GPL v3
 *
 * RhythmCat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * RhythmCat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RhythmCat; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <glib.h>
#include <gst/gst.h>
#include <glib/gprintf.h>
#include <rclib.h>
#include "rc-ui-player.h"
#include "rc-ui-listview.h"
#include "rc-mpris2.h"
#include "rc-common.h"
#include "rc-ui-style.h"
#include "rc-ui-effect.h"

static gchar main_app_id[] = "org.rhythmcat.RhythmCat2";
static gboolean main_debug_flag = FALSE;
static gboolean main_malloc_flag = FALSE;
static gchar **main_remaining_args = NULL;
static gchar *main_data_dir = NULL;
static gchar *main_user_dir = NULL;

static inline void rc_main_settings_init()
{
    if(!rclib_settings_has_key("MainUI", "MinimizeToTray", NULL))
        rclib_settings_set_boolean("MainUI", "MinimizeToTray", FALSE);
    if(!rclib_settings_has_key("MainUI", "MinimizeWhenClose", NULL))
        rclib_settings_set_boolean("MainUI", "MinimizeWhenClose", FALSE);
}

static void rc_main_app_activate_cb(GApplication *application,
    gpointer data)
{
    const gchar *home_dir = NULL;
    gchar *theme;
    gchar *theme_file;
    gchar *plugin_dir;
    gchar *plugin_conf;
    GSequenceIter *catalog_iter = NULL;
    GSequence *catalog = NULL;
    RCLibDbCatalogData *catalog_data = NULL;
    GSequenceIter *playlist_iter = NULL;
    GFile *file;
    gchar *uri;
    gint i;
    gboolean theme_flag = FALSE;
    const RCUiStyleEmbededTheme *theme_embeded;
    guint theme_number;
    rc_ui_player_init(GTK_APPLICATION(application));
    rc_ui_effect_window_init();
    theme_embeded = rc_ui_style_get_embeded_theme(&theme_number);
    theme = rclib_settings_get_string("MainUI", "Theme", NULL);
    if(theme!=NULL && strlen(theme)>0)
    {
        if(g_str_has_prefix(theme, "embeded-theme:"))
        {
            for(i=0;i<theme_number;i++)
            {
                if(g_strcmp0(theme+14, theme_embeded[i].name)==0)
                {
                    theme_flag = rc_ui_style_css_set_data(
                        theme_embeded[i].data, theme_embeded[i].length);
                }
            }
        }
        else
        {
            theme_file = g_build_filename(theme, "gtk3.css", NULL);
            theme_flag = rc_ui_style_css_set_file(theme_file);
            g_free(theme_file);
        }
        if(!theme_flag)
        {
            rc_ui_style_css_set_data(theme_embeded[0].data,
                theme_embeded[0].length);
        }
    }
    else
    {
        rc_ui_style_css_set_data(theme_embeded[0].data,
            theme_embeded[0].length);
    }
    g_free(theme);
    rclib_settings_apply();
    rc_mpris2_init();
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    plugin_conf = g_build_filename(home_dir, ".RhythmCat2", "plugins.conf",
        NULL);
    rclib_plugin_init(plugin_conf);
    g_free(plugin_conf);
    plugin_dir = g_build_filename(home_dir, ".RhythmCat2", "Plugins", NULL);
    g_mkdir_with_parents(plugin_dir, 0700);
    rclib_plugin_load_from_dir(plugin_dir);
    g_free(plugin_dir);
    rclib_plugin_load_from_configure();
    catalog = rclib_db_get_catalog();
    if(rclib_settings_get_boolean("Player", "LoadLastPosition", NULL) &&
        catalog!=NULL)
    {
        catalog_iter = g_sequence_get_iter_at_pos(catalog,
            rclib_settings_get_integer("Player", "LastPlayedCatalog", NULL));
        if(catalog_iter!=NULL)
            catalog_data = g_sequence_get(catalog_iter);
        if(catalog_data!=NULL && catalog_data->playlist!=NULL)
        {
            playlist_iter = g_sequence_get_iter_at_pos(
                catalog_data->playlist, rclib_settings_get_integer(
                "Player", "LastPlayedMusic", NULL));
        }
        if(playlist_iter!=NULL)
        {
            rclib_player_play_db(playlist_iter);
            if(!rclib_settings_get_boolean("Player", "AutoPlayWhenStartup",
                NULL))
                rclib_core_pause();
        }
        
    }
    else if(rclib_settings_get_boolean("Player", "AutoPlayWhenStartup",
        NULL) && catalog!=NULL)
    {
        if(catalog!=NULL)
            catalog_iter = g_sequence_get_begin_iter(catalog);
        if(catalog_iter!=NULL)
            catalog_data = g_sequence_get(catalog_iter);
        if(catalog_data!=NULL && catalog_data->playlist!=NULL)
            playlist_iter = g_sequence_get_begin_iter(
                catalog_data->playlist);
        if(playlist_iter!=NULL)
            rclib_player_play_db(playlist_iter);
    }
    if(main_remaining_args!=NULL)
    {
        if(catalog!=NULL)
            catalog_iter = g_sequence_get_begin_iter(catalog);
        if(catalog_iter!=NULL)
        {
            for(i=0;main_remaining_args[i]!=NULL;i++)
            {
                file = g_file_new_for_commandline_arg(
                    main_remaining_args[i]);
                if(file==NULL) continue;
                uri = g_file_get_uri(file);
                if(uri!=NULL)
                    rclib_db_playlist_add_music(catalog_iter, NULL, uri);
                g_free(uri);
                g_object_unref(file);
            }
        }
    }
}

static void rc_main_app_open_cb(GApplication *application, GFile **files,
    gint n_files, gchar *hint, gpointer data)
{
    GtkTreeIter tree_iter;
    GSequenceIter *catalog_iter = NULL;
    GSequence *catalog;
    gint i;
    gchar *uri;
    if(files==NULL) return;
    if(rc_ui_listview_catalog_get_cursor(&tree_iter))
        catalog_iter = tree_iter.user_data;
    else
    {
        catalog = rclib_db_get_catalog();
        if(catalog!=NULL)
            catalog_iter = g_sequence_get_begin_iter(catalog);
    }
    if(catalog_iter==NULL) return;
    for(i=0;i<n_files;i++)
    {
        if(files[i]==NULL) continue;
        uri = g_file_get_uri(files[i]);
        if(uri==NULL) continue;
        rclib_db_playlist_add_music(catalog_iter, NULL, uri);
        g_free(uri);
    }
}

static gboolean rc_main_print_version(const gchar *option_name,
    const gchar *value, gpointer data, GError **error)
{
    g_print(_("RhythmCat2, library version %d.%d.%d\n"
        "Copyright (C) 2012 SuperCat <supercatexpert@gmail.com>\n"
        "A music player based on GTK+ 3.0 & GStreamer 0.10\n"),
        rclib_major_version, rclib_minor_version, rclib_micro_version);
    exit(0);
    return TRUE;
}

gint rc_main_run(gint *argc, gchar **argv[])
{
    static GOptionEntry options[] =
    {
        {"debug", 'd', 0, G_OPTION_ARG_NONE, &main_debug_flag,
            N_("Enable debug mode"), NULL},
        {"use-std-malloc", 0, 0, G_OPTION_ARG_NONE, &main_malloc_flag,
            N_("Use malloc function in stardard C library"), NULL},
        {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY,
            &main_remaining_args, NULL, "[URI...]"},
        {"version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
            rc_main_print_version,
            N_("Output version information and exit"), NULL},
        {NULL}
    };
    GOptionContext *context;
    GtkApplication *app;
    GSequence *catalog;
    GError *error = NULL;
    const gchar *home_dir = NULL;
    gint status;
    GFile **remote_files;
    guint remote_file_num;
    guint i;
    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, options, GETTEXT_PACKAGE);
    g_option_context_add_group(context, gst_init_get_option_group());
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    setlocale(LC_ALL, NULL);
    if(!g_option_context_parse(context, argc, argv, &error))
    {
        g_print(_("%s\nRun '%s --help' to see a full list of available "
            "command line options.\n"), error->message, *argv[0]);
        g_error_free(error);
        g_option_context_free(context);
        exit(1);
    }
    g_option_context_free(context);
    g_random_set_seed(time(0));
    if(main_malloc_flag)
        g_slice_set_config(G_SLICE_CONFIG_ALWAYS_MALLOC, TRUE);
    if(main_debug_flag)
        ;
    g_set_application_name("RhythmCat2");
    g_set_prgname("RhythmCat2");
    app = gtk_application_new(main_app_id, G_APPLICATION_HANDLES_OPEN);
    if(!g_application_register(G_APPLICATION(app), NULL, &error))
    {
        g_warning("Cannot register player: %s", error->message);
        g_error_free(error);
        error = NULL;
    }
    if(g_application_get_is_registered(G_APPLICATION(app)))
    {
        if(g_application_get_is_remote(G_APPLICATION(app)))
        {
            g_message("This player is running already!");
            if(main_remaining_args==NULL) exit(0);
            remote_file_num = g_strv_length(main_remaining_args);
            if(remote_file_num<1) exit(0);
            remote_files = g_new0(GFile *, remote_file_num);
            for(i=0;main_remaining_args[i]!=NULL;i++)
            {
                remote_files[i] = g_file_new_for_commandline_arg(
                    main_remaining_args[i]);
            }
            g_application_open(G_APPLICATION(app), remote_files,
                remote_file_num, "RhythmCat2::open");
            for(i=0;i<remote_file_num;i++)
                g_object_unref(remote_files[i]);
            g_free(remote_files);
            exit(0);
        }
    }
    home_dir = g_getenv("HOME");
    if(home_dir==NULL)
        home_dir = g_get_home_dir();
    main_user_dir = g_build_filename(home_dir, ".RhythmCat2", NULL);
    if(!rclib_init(argc, argv, main_user_dir, &error))
    {
        g_error("Cannot load core: %s", error->message);
        g_error_free(error);
        g_free(main_user_dir);
        main_user_dir = NULL;
        return 1;
    }
    main_data_dir = rclib_util_get_data_dir("RhythmCat2", *argv[0]);
    rc_main_settings_init();
    gdk_threads_init();
    g_print("LibRhythmCat loaded. Version: %d.%d.%d, build date: %s\n",
        rclib_major_version, rclib_minor_version, rclib_micro_version,
        rclib_build_date);
    catalog = rclib_db_get_catalog();
    if(catalog!=NULL && g_sequence_get_length(catalog)==0)
        rclib_db_catalog_add(_("Default Playlist"), NULL,
        RCLIB_DB_CATALOG_TYPE_PLAYLIST);
    g_signal_connect(app, "activate", G_CALLBACK(rc_main_app_activate_cb),
        NULL);
    g_signal_connect(app, "open", G_CALLBACK(rc_main_app_open_cb), NULL);
    status = g_application_run(G_APPLICATION(app), *argc, *argv);
    g_free(main_user_dir);
    g_object_unref(app);
    return status;
}

/**
 * rc_main_exit:
 *
 * Exit from the player, save configure data and release all used resources.
 */

void rc_main_exit()
{
    rc_ui_effect_window_destroy();
    rc_ui_player_exit();
    rclib_plugin_exit();
    rc_mpris2_exit();
    rclib_exit();
    g_free(main_data_dir);
    g_free(main_user_dir);
}

/**
 * rc_main_get_data_dir:
 *
 * Get the program data directory path of the player.
 *
 * Returns: the program data directory path.
 */

const gchar *rc_main_get_data_dir()
{
    return main_data_dir;
}

/**
 * rc_main_get_user_dir:
 *
 * Get the user data directory path of the player.
 * (should be ~/.RhythmCat2)
 *
 * Returns: the user data directory path.
 */

const gchar *rc_main_get_user_dir()
{
    return main_user_dir;
}

