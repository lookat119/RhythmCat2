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
#include <stdarg.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define RCLIB_TYPE_DB (rclib_db_get_type())
#define RCLIB_DB(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), RCLIB_TYPE_DB, \
    RCLibDb))
#define RCLIB_DB_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), RCLIB_TYPE_DB, \
    RCLibDbClass))
#define RCLIB_IS_DB(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), RCLIB_TYPE_DB))
#define RCLIB_IS_DB_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), RCLIB_TYPE_DB))
#define RCLIB_DB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), \
    RCLIB_TYPE_DB, RCLibDbClass))
    
#define RCLIB_TYPE_DB_CATALOG_DATA (rclib_db_catalog_data_get_type())
#define RCLIB_TYPE_DB_PLAYLIST_DATA (rclib_db_playlist_data_get_type())
#define RCLIB_TYPE_DB_LIBRARY_DATA (rclib_db_library_data_get_type())
#define RCLIB_TYPE_DB_QUERY (rclib_db_query_get_type())

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

/**
 * RCLibDbLibraryType:
 * @RCLIB_DB_LIBRARY_TYPE_MISSING: the library item is missing
 * @RCLIB_DB_LIBRARY_TYPE_MUSIC: the library item is music
 * @RCLIB_DB_LIBRARY_TYPE_CUE: the library item is from CUE sheet
 *
 * The enum type for library.
 */

typedef enum {
    RCLIB_DB_LIBRARY_TYPE_MISSING = 0,
    RCLIB_DB_LIBRARY_TYPE_MUSIC = 1,
    RCLIB_DB_LIBRARY_TYPE_CUE = 2
}RCLibDbLibraryType;

/**
 * RCLibDbCatalogDataType:
 * @RCLIB_DB_CATALOG_DATA_TYPE_NONE: none type, not used by data
 * @RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST: the playlist
 *     (#RCLibDbPlaylistSequence)
 * @RCLIB_DB_CATALOG_DATA_TYPE_SELF_ITER: the iter pointed to self
 *     (#RCLibDbCatalogIter)
 * @RCLIB_DB_CATALOG_DATA_TYPE_NAME: the catalog name (string)
 * @RCLIB_DB_CATALOG_DATA_TYPE_TYPE: the catalog type (#RCLibDbCatalogType)
 * @RCLIB_DB_CATALOG_DATA_TYPE_STORE: the store pointer which can be used
 *     for UI (#gpointer)
 *
 * The enum type for set/get the data in the #RCLibDbCatalogData
 */

typedef enum {
    RCLIB_DB_CATALOG_DATA_TYPE_NONE = 0,
    RCLIB_DB_CATALOG_DATA_TYPE_PLAYLIST = 1,
    RCLIB_DB_CATALOG_DATA_TYPE_SELF_ITER = 2,
    RCLIB_DB_CATALOG_DATA_TYPE_NAME = 3,
    RCLIB_DB_CATALOG_DATA_TYPE_TYPE = 4,
    RCLIB_DB_CATALOG_DATA_TYPE_STORE = 5
}RCLibDbCatalogDataType;

/**
 * RCLibDbPlaylistDataType:
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_NONE: none type, not used by data
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG: the catalog (#RCLibDbCatalogSequence)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_SELF_ITER: the iter pointed to self
 *     (#RCLibDbPlaylistIter)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE: the playlist type (#RCLibDbPlaylistType)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_URI: the URI (string)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE: the title (string)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST: the artist (string)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUM: the album (string)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_FTYPE: the file type (string)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_LENGTH: the time length (#gint64)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_TRACKNUM: the track number (#gint)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_YEAR: the year (#gint)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_RATING: the rating (#gfloat)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICFILE: the lyric file path (string)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICSECFILE: the second lyric file
 *     path (string)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUMFILE: the album image file path (string)
 * @RCLIB_DB_PLAYLIST_DATA_TYPE_GENRE: the genre (string)
 *
 * The enum type for set/get the data in the #RCLibDbPlaylistData
 */

typedef enum {
    RCLIB_DB_PLAYLIST_DATA_TYPE_NONE = 0,
    RCLIB_DB_PLAYLIST_DATA_TYPE_CATALOG = 1,
    RCLIB_DB_PLAYLIST_DATA_TYPE_SELF_ITER = 2,
    RCLIB_DB_PLAYLIST_DATA_TYPE_TYPE = 3,
    RCLIB_DB_PLAYLIST_DATA_TYPE_URI = 4,
    RCLIB_DB_PLAYLIST_DATA_TYPE_TITLE = 5,
    RCLIB_DB_PLAYLIST_DATA_TYPE_ARTIST = 6,
    RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUM = 7,
    RCLIB_DB_PLAYLIST_DATA_TYPE_FTYPE = 8,
    RCLIB_DB_PLAYLIST_DATA_TYPE_LENGTH = 9,
    RCLIB_DB_PLAYLIST_DATA_TYPE_TRACKNUM = 10,
    RCLIB_DB_PLAYLIST_DATA_TYPE_YEAR = 11,
    RCLIB_DB_PLAYLIST_DATA_TYPE_RATING = 12,
    RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICFILE = 13,
    RCLIB_DB_PLAYLIST_DATA_TYPE_LYRICSECFILE = 14,
    RCLIB_DB_PLAYLIST_DATA_TYPE_ALBUMFILE = 15,
    RCLIB_DB_PLAYLIST_DATA_TYPE_GENRE = 16
}RCLibDbPlaylistDataType;

/**
 * RCLibDbLibraryDataType:
 * @RCLIB_DB_LIBRARY_DATA_TYPE_NONE: none type, not used by data
 * @RCLIB_DB_LIBRARY_DATA_TYPE_TYPE: the library item type
 *     (#RCLibDbLibraryType)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_URI: the URI (string)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_TITLE: the title (string)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST: the artist (string)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM: the album (string)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE: the file type (string)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH: the time length (#gint64)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM: the track number (#gint)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_YEAR: the year (#gint),
 * @RCLIB_DB_LIBRARY_DATA_TYPE_RATING: the rating (#gfloat)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_LYRICFILE: the lyric file path (string)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_LYRICSECFILE: the second lyric file
 *     path (string)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_ALBUMFILE: the album image file path (string)
 * @RCLIB_DB_LIBRARY_DATA_TYPE_GENRE: the genre (string)
 * 
 * The enum type for set/get the data in the #RCLibDbPlaylistData
 */

typedef enum {
    RCLIB_DB_LIBRARY_DATA_TYPE_NONE = 0,
    RCLIB_DB_LIBRARY_DATA_TYPE_TYPE = 1,
    RCLIB_DB_LIBRARY_DATA_TYPE_URI = 2,
    RCLIB_DB_LIBRARY_DATA_TYPE_TITLE = 3,
    RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST = 4,
    RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM = 5,
    RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE = 6,
    RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH = 7,
    RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM = 8,
    RCLIB_DB_LIBRARY_DATA_TYPE_YEAR = 9,
    RCLIB_DB_LIBRARY_DATA_TYPE_RATING = 10,
    RCLIB_DB_LIBRARY_DATA_TYPE_LYRICFILE = 11,
    RCLIB_DB_LIBRARY_DATA_TYPE_LYRICSECFILE = 12,
    RCLIB_DB_LIBRARY_DATA_TYPE_ALBUMFILE = 13,
    RCLIB_DB_LIBRARY_DATA_TYPE_GENRE = 14
}RCLibDbLibraryDataType;

/**
 * RCLibDbQueryDataType:
 * @RCLIB_DB_QUERY_DATA_TYPE_NONE: none type, not used by data
 * @RCLIB_DB_QUERY_DATA_TYPE_URI: the URI (string)
 * @RCLIB_DB_QUERY_DATA_TYPE_TITLE: the title (string)
 * @RCLIB_DB_QUERY_DATA_TYPE_ARTIST: the artist (string)
 * @RCLIB_DB_QUERY_DATA_TYPE_ALBUM: the album (string)
 * @RCLIB_DB_QUERY_DATA_TYPE_FTYPE: the file type (string)
 * @RCLIB_DB_QUERY_DATA_TYPE_LENGTH: the time length (#gint64)
 * @RCLIB_DB_QUERY_DATA_TYPE_TRACKNUM: the track number (#gint)
 * @RCLIB_DB_QUERY_DATA_TYPE_YEAR: the year (#gint),
 * @RCLIB_DB_QUERY_DATA_TYPE_RATING: the rating (#gfloat)
 * @RCLIB_DB_QUERY_DATA_TYPE_GENRE: the genre (string)
 * 
 * The enum type for query the given data type in the database.
 */

typedef enum {
    RCLIB_DB_QUERY_DATA_TYPE_NONE = 0,
    RCLIB_DB_QUERY_DATA_TYPE_URI = 1,
    RCLIB_DB_QUERY_DATA_TYPE_TITLE = 2,
    RCLIB_DB_QUERY_DATA_TYPE_ARTIST = 3,
    RCLIB_DB_QUERY_DATA_TYPE_ALBUM = 4,
    RCLIB_DB_QUERY_DATA_TYPE_FTYPE = 5,
    RCLIB_DB_QUERY_DATA_TYPE_LENGTH = 6,
    RCLIB_DB_QUERY_DATA_TYPE_TRACKNUM = 7,
    RCLIB_DB_QUERY_DATA_TYPE_YEAR = 8,
    RCLIB_DB_QUERY_DATA_TYPE_RATING = 9,
    RCLIB_DB_QUERY_DATA_TYPE_GENRE = 10
}RCLibDbQueryDataType;

/**
 * RCLibDbQueryConditionType:
 * @RCLIB_DB_QUERY_CONDITION_TYPE_NONE: none query type
 * @RCLIB_DB_QUERY_CONDITION_TYPE_SUBQUERY: the sub-query type
 * @RCLIB_DB_QUERY_CONDITION_TYPE_PROP_EQUALS: the property equals to the
 *     query condition
 * @RCLIB_DB_QUERY_CONDITION_TYPE_PROP_NOT_EQUAL: the property does not equal
 *     to the query condition
 * @RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LIKE: the property contains the
 *     sub-string which equals to the query condition
 * @RCLIB_DB_QUERY_CONDITION_TYPE_PROP_NOT_LIKE: the property does not
 *     contain the sub-string which equals to the query condition
 * @RCLIB_DB_QUERY_CONDITION_TYPE_PROP_PREFIX: the prefix of the property
 *     equals to the query condition
 * @RCLIB_DB_QUERY_CONDITION_TYPE_PROP_SUFFIX: the suffix of the property
 *     equals to the query condition
 * @RCLIB_DB_QUERY_CONDITION_TYPE_PROP_GREATER: the query condition is greater
 *     than the property
 * @RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LESS: the query condition is lesser
 *     than the property
 * @RCLIB_DB_QUERY_CONDITION_TYPE_PROP_GREATER_OR_EQUAL: the query condition
 *     is greater than or equal to the property
 * @RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LESS_OR_EQUAL: the query condition is
 *     lesser than or equal to the property
 * @RCLIB_DB_QUERY_CONDITION_TYPE_LAST: not used, just a last enum type 
 * 
 * The enum type for query the properties of the items in the library
 * by the given condition type.
 */

typedef enum {
    RCLIB_DB_QUERY_CONDITION_TYPE_NONE = 0,
    RCLIB_DB_QUERY_CONDITION_TYPE_SUBQUERY = 1,
    RCLIB_DB_QUERY_CONDITION_TYPE_PROP_EQUALS = 2,
    RCLIB_DB_QUERY_CONDITION_TYPE_PROP_NOT_EQUAL = 3,
    RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LIKE = 4,
    RCLIB_DB_QUERY_CONDITION_TYPE_PROP_NOT_LIKE = 5,
    RCLIB_DB_QUERY_CONDITION_TYPE_PROP_PREFIX = 6,
    RCLIB_DB_QUERY_CONDITION_TYPE_PROP_SUFFIX = 7,
    RCLIB_DB_QUERY_CONDITION_TYPE_PROP_GREATER = 8,
    RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LESS = 9,
    RCLIB_DB_QUERY_CONDITION_TYPE_PROP_GREATER_OR_EQUAL = 10,
    RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LESS_OR_EQUAL = 11,
    RCLIB_DB_QUERY_CONDITION_TYPE_LAST = 12
}RCLibDbQueryConditionType;

typedef struct _RCLibDbCatalogData RCLibDbCatalogData;
typedef struct _RCLibDbPlaylistData RCLibDbPlaylistData;
typedef struct _RCLibDbLibraryData RCLibDbLibraryData;
typedef struct _RCLibDb RCLibDb;
typedef struct _RCLibDbClass RCLibDbClass;
typedef struct _RCLibDbPrivate RCLibDbPrivate;

typedef struct _RCLibDbCatalogSequence RCLibDbCatalogSequence;
typedef struct _RCLibDbPlaylistSequence RCLibDbPlaylistSequence;
typedef struct _RCLibDbCatalogIter RCLibDbCatalogIter;
typedef struct _RCLibDbPlaylistIter RCLibDbPlaylistIter;

typedef GPtrArray RCLibDbQuery;

/**
 * RCLibDb:
 *
 * The playlist database. The contents of the #RCLibDb structure are
 * private and should only be accessed via the provided API.
 */

struct _RCLibDb {
    /*< private >*/
    GObject parent;
    RCLibDbPrivate *priv;
};

/**
 * RCLibDbClass:
 *
 * #RCLibDb class.
 */

struct _RCLibDbClass {
    /*< private >*/
    GObjectClass parent_class;
    void (*catalog_added)(RCLibDb *db, RCLibDbCatalogIter *iter);
    void (*catalog_changed)(RCLibDb *db, RCLibDbCatalogIter *iter);
    void (*catalog_delete)(RCLibDb *db, RCLibDbCatalogIter *iter);
    void (*catalog_reordered)(RCLibDb *db, gint *new_order);
    void (*playlist_added)(RCLibDb *db, RCLibDbPlaylistIter *iter);
    void (*playlist_changed)(RCLibDb *db, RCLibDbPlaylistIter *iter);
    void (*playlist_delete)(RCLibDb *db, RCLibDbPlaylistIter *iter);
    void (*playlist_reordered)(RCLibDb *db, RCLibDbCatalogIter *iter,
        gint *new_order);
    void (*import_updated)(RCLibDb *db, gint remaining);
    void (*refresh_updated)(RCLibDb *db, gint remaining);
    void (*library_added)(RCLibDb *db, const gchar *uri);
    void (*library_changed)(RCLibDb *db, const gchar *uri);
    void (*library_delete)(RCLibDb *db, const gchar *uri);
};

/*< private >*/
GType rclib_db_get_type();
GType rclib_db_catalog_data_get_type();
GType rclib_db_playlist_data_get_type();
GType rclib_db_library_data_get_type();
GType rclib_db_query_get_type();

/*< public >*/
gboolean rclib_db_init(const gchar *file);
void rclib_db_exit();
GObject *rclib_db_get_instance();
gulong rclib_db_signal_connect(const gchar *name,
    GCallback callback, gpointer data);
void rclib_db_signal_disconnect(gulong handler_id);
void rclib_db_import_cancel();
void rclib_db_refresh_cancel();
gint rclib_db_import_queue_get_length();
gint rclib_db_refresh_queue_get_length();
gboolean rclib_db_sync();
gboolean rclib_db_load_autosaved();
gboolean rclib_db_autosaved_exist();
void rclib_db_autosaved_remove();

/* Playlist Interface */
RCLibDbCatalogData *rclib_db_catalog_data_new();
RCLibDbCatalogData *rclib_db_catalog_data_ref(RCLibDbCatalogData *data);
void rclib_db_catalog_data_unref(RCLibDbCatalogData *data);
void rclib_db_catalog_data_free(RCLibDbCatalogData *data);
void rclib_db_catalog_data_set(RCLibDbCatalogData *data,
    RCLibDbCatalogDataType type1, ...);
void rclib_db_catalog_data_get(RCLibDbCatalogData *data,
    RCLibDbCatalogDataType type1, ...);
void rclib_db_catalog_data_iter_set(RCLibDbCatalogIter *iter,
    RCLibDbCatalogDataType type1, ...);
void rclib_db_catalog_data_iter_get(RCLibDbCatalogIter *iter,
    RCLibDbCatalogDataType type1, ...);
RCLibDbPlaylistData *rclib_db_playlist_data_new();
RCLibDbPlaylistData *rclib_db_playlist_data_ref(RCLibDbPlaylistData *data);
void rclib_db_playlist_data_unref(RCLibDbPlaylistData *data);
void rclib_db_playlist_data_free(RCLibDbPlaylistData *data);
void rclib_db_playlist_data_set(RCLibDbPlaylistData *data,
    RCLibDbPlaylistDataType type1, ...);
void rclib_db_playlist_data_get(RCLibDbPlaylistData *data,
    RCLibDbPlaylistDataType type1, ...);
void rclib_db_playlist_data_iter_set(RCLibDbPlaylistIter *iter,
    RCLibDbPlaylistDataType type1, ...);
void rclib_db_playlist_data_iter_get(RCLibDbPlaylistIter *iter,
    RCLibDbPlaylistDataType type1, ...);
gint rclib_db_catalog_get_length();
RCLibDbCatalogIter *rclib_db_catalog_get_begin_iter();
RCLibDbCatalogIter *rclib_db_catalog_get_last_iter();
RCLibDbCatalogIter *rclib_db_catalog_get_end_iter();
RCLibDbCatalogIter *rclib_db_catalog_get_iter_at_pos(gint pos);
RCLibDbCatalogData *rclib_db_catalog_iter_get_data(
    RCLibDbCatalogIter *iter);
gboolean rclib_db_catalog_iter_is_begin(RCLibDbCatalogIter *iter);
gboolean rclib_db_catalog_iter_is_end(RCLibDbCatalogIter *iter);
RCLibDbCatalogIter *rclib_db_catalog_iter_next(RCLibDbCatalogIter *iter);
RCLibDbCatalogIter *rclib_db_catalog_iter_prev(RCLibDbCatalogIter *iter);
gint rclib_db_catalog_iter_get_position(RCLibDbCatalogIter *iter);
RCLibDbCatalogIter *rclib_db_catalog_iter_range_get_midpoint(
    RCLibDbCatalogIter *begin, RCLibDbCatalogIter *end);
gint rclib_db_catalog_iter_compare(RCLibDbCatalogIter *a,
    RCLibDbCatalogIter *b);
gint rclib_db_playlist_get_length(RCLibDbCatalogIter *catalog_iter);
gint rclib_db_playlist_iter_get_length(RCLibDbPlaylistIter *playlist_iter);
RCLibDbPlaylistIter *rclib_db_playlist_get_begin_iter(
    RCLibDbCatalogIter *catalog_iter);
RCLibDbPlaylistIter *rclib_db_playlist_get_last_iter(
    RCLibDbCatalogIter *catalog_iter);
RCLibDbPlaylistIter *rclib_db_playlist_get_end_iter(
    RCLibDbCatalogIter *catalog_iter);
RCLibDbPlaylistIter *rclib_db_playlist_iter_get_begin_iter(
    RCLibDbPlaylistIter *playlist_iter);
RCLibDbPlaylistIter *rclib_db_playlist_iter_get_last_iter(
    RCLibDbPlaylistIter *playlist_iter);
RCLibDbPlaylistIter *rclib_db_playlist_iter_get_end_iter(
    RCLibDbPlaylistIter *playlist_iter);
RCLibDbPlaylistIter *rclib_db_playlist_get_iter_at_pos(
    RCLibDbCatalogIter *catalog_iter, gint pos);
RCLibDbPlaylistIter *rclib_db_playlist_iter_get_iter_at_pos(
    RCLibDbPlaylistIter *playlist_iter, gint pos);
RCLibDbPlaylistData *rclib_db_playlist_iter_get_data(
    RCLibDbPlaylistIter *iter);
gboolean rclib_db_playlist_iter_is_begin(RCLibDbPlaylistIter *iter);
gboolean rclib_db_playlist_iter_is_end(RCLibDbPlaylistIter *iter);
RCLibDbPlaylistIter *rclib_db_playlist_iter_next(RCLibDbPlaylistIter *iter);
RCLibDbPlaylistIter *rclib_db_playlist_iter_prev(RCLibDbPlaylistIter *iter);
gint rclib_db_playlist_iter_get_position(RCLibDbPlaylistIter *iter);
RCLibDbPlaylistIter *rclib_db_playlist_iter_range_get_midpoint(
    RCLibDbPlaylistIter *begin, RCLibDbPlaylistIter *end);
gint rclib_db_playlist_iter_compare(RCLibDbPlaylistIter *a,
    RCLibDbPlaylistIter *b);
void rclib_db_catalog_foreach(GFunc func, gpointer user_data);
void rclib_db_catalog_iter_foreach_range(RCLibDbCatalogIter *begin,
    RCLibDbCatalogIter *end, GFunc func, gpointer user_data);
void rclib_db_playlist_foreach(RCLibDbCatalogIter *catalog_iter,
    GFunc func, gpointer user_data);
void rclib_db_playlist_iter_foreach_range(RCLibDbPlaylistIter *begin,
    RCLibDbPlaylistIter *end, GFunc func, gpointer user_data);
RCLibDbCatalogSequence *rclib_db_get_catalog();
gboolean rclib_db_catalog_is_valid_iter(RCLibDbCatalogIter *catalog_iter);
RCLibDbCatalogIter *rclib_db_catalog_add(const gchar *name,
    RCLibDbCatalogIter *iter, gint type);
void rclib_db_catalog_delete(RCLibDbCatalogIter *iter);
void rclib_db_catalog_reorder(gint *new_order);
gboolean rclib_db_playlist_is_valid_iter(RCLibDbPlaylistIter *playlist_iter);
void rclib_db_playlist_add_music(RCLibDbCatalogIter *iter,
    RCLibDbPlaylistIter *insert_iter, const gchar *uri);
void rclib_db_playlist_add_music_and_play(RCLibDbCatalogIter *iter,
    RCLibDbPlaylistIter *insert_iter, const gchar *uri);
void rclib_db_playlist_delete(RCLibDbPlaylistIter *iter);
void rclib_db_playlist_update_metadata(RCLibDbPlaylistIter *iter,
    const RCLibDbPlaylistData *data);
void rclib_db_playlist_reorder(RCLibDbCatalogIter *iter, gint *new_order);
void rclib_db_playlist_move_to_another_catalog(RCLibDbPlaylistIter **iters,
    guint num, RCLibDbCatalogIter *catalog_iter);
void rclib_db_playlist_add_m3u_file(RCLibDbCatalogIter *iter,
    RCLibDbPlaylistIter *insert_iter, const gchar *filename);
void rclib_db_playlist_add_directory(RCLibDbCatalogIter *iter,
    RCLibDbPlaylistIter *insert_iter, const gchar *dir);
gboolean rclib_db_playlist_export_m3u_file(RCLibDbCatalogIter *iter,
    const gchar *sfilename);
gboolean rclib_db_playlist_export_all_m3u_files(const gchar *dir);
void rclib_db_playlist_refresh(RCLibDbCatalogIter *iter);
gboolean rclib_db_load_legacy();
gboolean rclib_db_playlist_data_query(RCLibDbPlaylistData *playlist_data,
    RCLibDbQuery *query);
gboolean rclib_db_playlist_iter_query(RCLibDbPlaylistIter *playlist_iter,
    RCLibDbQuery *query);
GPtrArray *rclib_db_playlist_query(RCLibDbCatalogIter *catalog_iter,
    RCLibDbQuery *query);

/* Library Interface */
RCLibDbLibraryData *rclib_db_library_data_new();
RCLibDbLibraryData *rclib_db_library_data_ref(RCLibDbLibraryData *data);
void rclib_db_library_data_unref(RCLibDbLibraryData *data);
void rclib_db_library_data_free(RCLibDbLibraryData *data);
void rclib_db_library_data_set(RCLibDbLibraryData *data,
    RCLibDbLibraryDataType type1, ...);
void rclib_db_library_data_get(const RCLibDbLibraryData *data,
    RCLibDbLibraryDataType type1, ...);
GHashTable *rclib_db_get_library_table();
gboolean rclib_db_library_has_uri(const gchar *uri);
void rclib_db_library_add_music(const gchar *uri);
void rclib_db_library_add_music_and_play(const gchar *uri);
void rclib_db_library_delete(const gchar *uri);

/* Query Interface */
RCLibDbQuery *rclib_db_query_parse(RCLibDbQueryConditionType condition1, ...);
RCLibDbQuery *rclib_db_query_parse_valist(
    RCLibDbQueryConditionType condition1, va_list args);
gboolean rclib_db_query_concatenate(RCLibDbQuery *target,
    const RCLibDbQuery *src);
RCLibDbQuery *rclib_db_query_copy(const RCLibDbQuery *query);
void rclib_db_query_free(RCLibDbQuery *query);
GType rclib_db_query_get_query_data_type(RCLibDbQueryDataType query_type);

G_END_DECLS

#endif

