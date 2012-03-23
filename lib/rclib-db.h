/*
 * RhythmCat Library Music Database Header Declaration
 *
 * rclib-db.h
 * This file is part of RhythmCat Library (LibRhythmCat)
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

#ifndef HAVE_RC_LIB_DB_H
#define HAVE_RC_LIB_DB_H

#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gst/gst.h>
#include <json.h>

G_BEGIN_DECLS

#define RCLIB_DB_TYPE (rclib_db_get_type())
#define RCLIB_DB(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), RCLIB_DB_TYPE, \
    RCLibDb))
#define RCLIB_DB_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), RCLIB_DB_TYPE, \
    RCLibDbClass))
#define RCLIB_IS_DB(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), RCLIB_DB_TYPE))
#define RCLIB_IS_DB_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), RCLIB_DB_TYPE))
#define RCLIB_DB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RCLIB_DB_TYPE, RCLibDbClass))

/**
 * RCLibDbCatalogType:
 * @RCLIB_DB_CATALOG_TYPE_PLAYLIST: the catalog is a playlist
 *
 * The enum type for catalog type.
 */

typedef enum {
    RCLIB_DB_CATALOG_TYPE_PLAYLIST = 1
}RCLibDbCatalogType;

/**
 * RCLibDbPlaylistType:
 * @RCLIB_DB_PLAYLIST_TYPE_MISSING: the playlist item is missing
 * @RCLIB_DB_PLAYLIST_TYPE_MUSIC: the playlist item is music
 * @RCLIB_DB_PLAYLIST_TYPE_CUE: the playlist item is from CUE sheet
 *
 * The enum type for playlist type.
 */

typedef enum {
    RCLIB_DB_PLAYLIST_TYPE_MISSING = 0,
    RCLIB_DB_PLAYLIST_TYPE_MUSIC = 1,
    RCLIB_DB_PLAYLIST_TYPE_CUE = 2
}RCLibDbPlaylistType;

typedef struct _RCLibDbCatalogData RCLibDbCatalogData;
typedef struct _RCLibDbPlaylistData RCLibDbPlaylistData;
typedef struct _RCLibDb RCLibDb;
typedef struct _RCLibDbClass RCLibDbClass;

/**
 * RCLibDbCatalogData:
 * @playlist: a playlist
 * @name: the name
 * @type: the type of the item in catalog
 * @store: the store, used in UI
 *
 * The structure for catalog item data.
 */

struct _RCLibDbCatalogData {
    /*< private >*/
    gint ref_count;
    
    /*< public >*/
    GSequence *playlist;
    gchar *name;
    RCLibDbCatalogType type;
    gpointer store;
};

/**
 * RCLibDbPlaylistData:
 * @catalog: the catalog item data
 * @type: the type of the item in playlist
 * @uri: the URI
 * @title: the title
 * @artist: the artist
 * @album: the album
 * @ftype: the format information
 * @length: the the duration (length) of the music (unit: nanosecond)
 * @tracknum: the track number
 * @year: the year
 * @rating: the rating level
 * @lyricfile: the lyric binding file path
 * @lyricsecfile: the secondary lyric binding file path
 * @albumfile: the album cover image binding file path
 *
 * The structure for playlist item data.
 */

struct _RCLibDbPlaylistData {
    /*< private >*/
    gint ref_count;

    /*< public >*/
    GSequenceIter *catalog;
    RCLibDbPlaylistType type;
    gchar *uri;
    gchar *title;
    gchar *artist;
    gchar *album;
    gchar *ftype;
    gint64 length;
    gint tracknum;
    gint year;
    guint rating;
    gchar *lyricfile;
    gchar *lyricsecfile;
    gchar *albumfile;
};

/**
 * RCLibDb:
 *
 * The playlist database. The contents of the #RCLibDb structure are
 * private and should only be accessed via the provided API.
 */

struct _RCLibDb {
    /*< private >*/
    GObject parent;
};

/**
 * RCLibDbClass:
 *
 * #RCLibDb class.
 */

struct _RCLibDbClass {
    /*< private >*/
    GObjectClass parent_class;
    void (*catalog_added)(RCLibDb *db, GSequenceIter *iter);
    void (*catalog_changed)(RCLibDb *db, GSequenceIter *iter);
    void (*catalog_delete)(RCLibDb *db, GSequenceIter *iter);
    void (*catalog_reordered)(RCLibDb *db, gint *new_order);
    void (*playlist_added)(RCLibDb *db, GSequenceIter *iter);
    void (*playlist_changed)(RCLibDb *db, GSequenceIter *iter);
    void (*playlist_delete)(RCLibDb *db, GSequenceIter *iter);
    void (*playlist_reordered)(RCLibDb *db, GSequenceIter *iter,
        gint *new_order);
    void (*import_updated)(RCLibDb *db, gint remaining);
    void (*refresh_updated)(RCLibDb *db, gint remaining);
};

/*< private >*/
GType rclib_db_get_type();

/*< public >*/
gboolean rclib_db_init(const gchar *file);
void rclib_db_exit();
GObject *rclib_db_get_instance();
gulong rclib_db_signal_connect(const gchar *name,
    GCallback callback, gpointer data);
void rclib_db_signal_disconnect(gulong handler_id);
RCLibDbCatalogData *rclib_db_catalog_data_new();
RCLibDbCatalogData *rclib_db_catalog_data_ref(RCLibDbCatalogData *data);
void rclib_db_catalog_data_unref(RCLibDbCatalogData *data);
void rclib_db_catalog_data_free(RCLibDbCatalogData *data);
RCLibDbPlaylistData *rclib_db_playlist_data_new();
RCLibDbPlaylistData *rclib_db_playlist_data_ref(RCLibDbPlaylistData *data);
void rclib_db_playlist_data_unref(RCLibDbPlaylistData *data);
void rclib_db_playlist_data_free(RCLibDbPlaylistData *data);
GSequenceIter *rclib_db_catalog_add(const gchar *name,
    GSequenceIter *iter, gint type);
GSequence *rclib_db_get_catalog();
void rclib_db_catalog_delete(GSequenceIter *iter);
void rclib_db_catalog_set_name(GSequenceIter *iter, const gchar *name);
void rclib_db_catalog_set_type(GSequenceIter *iter, RCLibDbCatalogType type);
void rclib_db_catalog_reorder(gint *new_order);
void rclib_db_playlist_add_music(GSequenceIter *iter,
    GSequenceIter *insert_iter, const gchar *uri);
void rclib_db_playlist_add_music_and_play(GSequenceIter *iter,
    GSequenceIter *insert_iter, const gchar *uri);
void rclib_db_playlist_delete(GSequenceIter *iter);
void rclib_db_playlist_update_metadata(GSequenceIter *iter,
    const RCLibDbPlaylistData *data);
void rclib_db_playlist_update_length(GSequenceIter *iter,
    gint64 length);
void rclib_db_playlist_set_type(GSequenceIter *iter,
    RCLibDbPlaylistType type);
void rclib_db_playlist_set_rating(GSequenceIter *iter, gint rating);
void rclib_db_playlist_set_lyric_bind(GSequenceIter *iter,
    const gchar *filename);
void rclib_db_playlist_set_lyric_secondary_bind(GSequenceIter *iter,
    const gchar *filename);
void rclib_db_playlist_set_album_bind(GSequenceIter *iter,
    const gchar *filename);
const gchar *rclib_db_playlist_get_lyric_bind(GSequenceIter *iter);
const gchar *rclib_db_playlist_get_lyric_secondary_bind(GSequenceIter *iter);
const gchar *rclib_db_playlist_get_album_bind(GSequenceIter *iter);  
void rclib_db_playlist_reorder(GSequenceIter *iter, gint *new_order);
void rclib_db_playlist_move_to_another_catalog(GSequenceIter **iters,
    guint num, GSequenceIter *catalog_iter);
void rclib_db_playlist_add_m3u_file(GSequenceIter *iter,
    GSequenceIter *insert_iter, const gchar *filename);
void rclib_db_playlist_add_directory(GSequenceIter *iter,
    GSequenceIter *insert_iter, const gchar *dir);
gboolean rclib_db_playlist_export_m3u_file(GSequenceIter *iter,
    const gchar *sfilename);
gboolean rclib_db_playlist_export_all_m3u_files(const gchar *dir);
void rclib_db_playlist_refresh(GSequenceIter *iter);
void rclib_db_playlist_import_cancel();
void rclib_db_playlist_refresh_cancel();
gint rclib_db_playlist_import_queue_get_length();
gint rclib_db_playlist_refresh_queue_get_length();
gboolean rclib_db_playlist_sync();

G_END_DECLS

#endif
