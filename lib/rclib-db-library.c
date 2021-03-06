/*
 * RhythmCat Library Music Database Module (Library Part.)
 * Manage the library part of the music database.
 *
 * rclib-db-library.c
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

#include "rclib-db.h"
#include "rclib-db-priv.h"
#include "rclib-common.h"
#include "rclib-tag.h"
#include "rclib-cue.h"
#include "rclib-core.h"
#include "rclib-util.h"
#include "rclib-marshal.h"

enum
{
    SIGNAL_LIBRARY_QUERY_RESULT_ADDED,
    SIGNAL_LIBRARY_QUERY_RESULT_DELETE,
    SIGNAL_LIBRARY_QUERY_RESULT_CHANGED,
    SIGNAL_LIBRARY_QUERY_RESULT_REORDERED,
    SIGNAL_LIBRARY_QUERY_RESULT_PROP_ADDED,
    SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE,
    SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED,
    SIGNAL_LIBRARY_QUERY_RESULT_PROP_REORDERED,
    SIGNAL_LIBRARY_QUERY_RESULT_LAST
};

struct _RCLibDbLibraryQueryResultIter
{
    gint dummy;
};

struct _RCLibDbLibraryQueryResultPropIter
{
    gint dummy;
};

typedef struct _RCLibDbLibraryQueryResultPropItem
{
    GSequence *prop_sequence;
    GHashTable *prop_iter_table;
    GHashTable *prop_name_table;
    GHashTable *prop_uri_table;
    guint count;
    gboolean sort_direction;
}RCLibDbLibraryQueryResultPropItem;

typedef struct _RCLibDbLibraryQueryResultPropData
{
    gchar *prop_name;
    guint prop_count;
}RCLibDbLibraryQueryResultPropData;

static gint db_library_query_result_signals[SIGNAL_LIBRARY_QUERY_RESULT_LAST] =
    {0};
static gpointer rclib_db_library_query_result_parent_class = NULL;

static inline RCLibDbLibraryDataType rclib_db_library_query_type_to_data_type(
    RCLibDbQueryDataType query_type)
{
    switch(query_type)
    {
        case RCLIB_DB_QUERY_DATA_TYPE_NONE:
            return RCLIB_DB_LIBRARY_DATA_TYPE_NONE;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_URI:
            return RCLIB_DB_LIBRARY_DATA_TYPE_URI;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_TITLE:
            return RCLIB_DB_LIBRARY_DATA_TYPE_TITLE;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_ARTIST:
            return RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_ALBUM:
            return RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_FTYPE:
            return RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_LENGTH:
            return RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_TRACKNUM:
            return RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_YEAR:
            return RCLIB_DB_LIBRARY_DATA_TYPE_YEAR;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_RATING:
            return RCLIB_DB_LIBRARY_DATA_TYPE_RATING;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_GENRE:
            return RCLIB_DB_LIBRARY_DATA_TYPE_GENRE;
            break;
    }
    return RCLIB_DB_LIBRARY_DATA_TYPE_NONE;
}

static gint rclib_db_variant_sort_asc_func(GSequenceIter *a, GSequenceIter *b,
    gpointer user_data)
{
    GHashTable *new_positions = user_data;
    GVariant *variant1, *variant2;
    variant1 = g_hash_table_lookup(new_positions, a);
    variant2 = g_hash_table_lookup(new_positions, b);
    return g_variant_compare(variant1, variant2);
}

static gint rclib_db_variant_sort_dsc_func(GSequenceIter *a, GSequenceIter *b,
    gpointer user_data)
{
    GHashTable *new_positions = user_data;
    GVariant *variant1, *variant2;
    variant1 = g_hash_table_lookup(new_positions, a);
    variant2 = g_hash_table_lookup(new_positions, b);
    return g_variant_compare(variant2, variant1);
}

static gint rclib_db_library_query_result_item_compare_func(gconstpointer a,
    gconstpointer b, gpointer data)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    gint ret = 0;
    GType column_type;
    GVariant *variant1, *variant2;
    RCLibDbLibraryData *data1 = (RCLibDbLibraryData *)a;
    RCLibDbLibraryData *data2 = (RCLibDbLibraryData *)b;
    priv = (RCLibDbLibraryQueryResultPrivate *)data;
    if(data==NULL) return 0;
    switch(priv->sort_column)
    {
        case RCLIB_DB_LIBRARY_DATA_TYPE_TITLE:
        case RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST:
        case RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM:
        case RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE:
        case RCLIB_DB_LIBRARY_DATA_TYPE_GENRE:
        {
            column_type = G_TYPE_STRING;
            break;
        }
        case RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH:
        {
            column_type = G_TYPE_INT64;
            break;
        }
        case RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM:
        case RCLIB_DB_LIBRARY_DATA_TYPE_YEAR:
        {
            column_type = G_TYPE_INT;
            break;
        }
        case RCLIB_DB_LIBRARY_DATA_TYPE_RATING:
        {
            column_type = G_TYPE_FLOAT;
            break;
        }
        default:
        {
            g_warning("Error column type: %u", priv->sort_column);
            return 0;
            break;
        }
    }
    switch(column_type)
    {
        case G_TYPE_STRING:
        {
            gchar *vstr = NULL;
            gchar *cstr = NULL;
            gchar *uri = NULL;
            if(data1!=NULL)
            {
                rclib_db_library_data_get(data1, priv->sort_column, &vstr,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                if(priv->sort_column==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                    (vstr==NULL || strlen(vstr)==0))
                {
                    rclib_db_library_data_get(data1,
                        RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                        RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                    if(uri!=NULL)
                        vstr = rclib_tag_get_name_from_uri(uri);
                    g_free(uri);
                }
            }                
            if(vstr==NULL) vstr = g_strdup("");
            cstr = g_utf8_collate_key(vstr, -1);
            variant1 = g_variant_new_string(cstr);
            g_free(cstr);
            g_free(vstr);
            vstr = NULL;
            if(data2!=NULL)
            {
                rclib_db_library_data_get(data2, priv->sort_column, &vstr,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                if(priv->sort_column==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                    (vstr==NULL || strlen(vstr)==0))
                {
                    rclib_db_library_data_get(data2,
                        RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                        RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                    if(uri!=NULL)
                        vstr = rclib_tag_get_name_from_uri(uri);
                    g_free(uri);
                }
            }                
            if(vstr==NULL) vstr = g_strdup("");
            cstr = g_utf8_collate_key(vstr, -1);
            variant2 = g_variant_new_string(cstr);
            g_free(cstr);
            g_free(vstr);
            break;
        }
        case G_TYPE_INT:
        {
            gint vint = 0;
            if(data1!=NULL)
            {
                rclib_db_library_data_get(data1, priv->sort_column, &vint,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            }
            variant1 = g_variant_new_int32(vint);
            vint = 0;
            if(data2!=NULL)
            {
                rclib_db_library_data_get(data2, priv->sort_column, &vint,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            }
            variant2 = g_variant_new_int32(vint);
            break;
        }
        case G_TYPE_INT64:
        {
            gint64 vint64 = 0;
            if(data1!=NULL)
            {
                rclib_db_library_data_get(data1, priv->sort_column, &vint64,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            }
            variant1 = g_variant_new_int64(vint64);
            vint64 = 0;
            if(data2!=NULL)
            {
                rclib_db_library_data_get(data2, priv->sort_column, &vint64,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            }
            variant2 = g_variant_new_int64(vint64);
            break;
        }
        case G_TYPE_FLOAT:
        {
            gfloat vfloat = 0;
            if(data1!=NULL)
            {
                rclib_db_library_data_get(data1, priv->sort_column, &vfloat,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            }
            variant1 = g_variant_new_double(vfloat);
            if(data2!=NULL)
            {
                rclib_db_library_data_get(data2, priv->sort_column, &vfloat,
                    RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            }
            variant2 = g_variant_new_double(vfloat);
            break;
        }
        default:
        {
            variant1 = g_variant_new_boolean(FALSE);
            variant2 = g_variant_new_boolean(FALSE);
            g_warning("Wrong column data type, this should not happen!");
            break;
        }
    }
    ret = g_variant_compare(variant1, variant2);
    if(priv->sort_direction)
        return -ret;
    else
        return ret;
}


static gint rclib_db_library_query_result_prop_item_compare_func(
    gconstpointer a, gconstpointer b, gpointer data)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropData *data1 =
        (RCLibDbLibraryQueryResultPropData *)a;
    RCLibDbLibraryQueryResultPropData *data2 =
        (RCLibDbLibraryQueryResultPropData *)b;
    gchar *m = NULL, *n = NULL;
    gint ret = 0;
    priv = (RCLibDbLibraryQueryResultPrivate *)data;
    if(data==NULL) return 0;
    if(data1->prop_name!=NULL)
        m = g_utf8_collate_key(data1->prop_name, -1);
    else
        m = g_strdup("");
    if(data2->prop_name!=NULL)
        n = g_utf8_collate_key(data2->prop_name, -1);
    else
        n = g_strdup("");
    if(priv->sort_direction)
        ret = g_utf8_collate(n, m);
    else
        ret = g_utf8_collate(m, n);
    g_free(m);
    g_free(n);
    return ret;
}


static RCLibDbLibraryQueryResultPropData *
    rclib_db_library_query_result_prop_data_new()
{
    RCLibDbLibraryQueryResultPropData *data;
    data = g_new0(RCLibDbLibraryQueryResultPropData, 1);
    return data;
}

static void rclib_db_library_query_result_prop_data_free(
    RCLibDbLibraryQueryResultPropData *data)
{
    if(data==NULL) return;
    if(data->prop_name!=NULL)
    {
        g_free(data->prop_name);
    }
    g_free(data);
}

static RCLibDbLibraryQueryResultPropItem *
    rclib_db_library_query_result_prop_item_new()
{
    RCLibDbLibraryQueryResultPropItem *item;
    item = g_new0(RCLibDbLibraryQueryResultPropItem, 1);
    item->prop_sequence = g_sequence_new((GDestroyNotify)
        rclib_db_library_query_result_prop_data_free);
    item->prop_iter_table = g_hash_table_new(g_direct_hash, g_direct_equal);
    item->prop_name_table = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, NULL);
    item->prop_uri_table = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, NULL);
    return item;
}

static void rclib_db_library_query_result_prop_item_free(
    RCLibDbLibraryQueryResultPropItem *item)
{
    if(item==NULL) return;
    if(item->prop_sequence!=NULL)
        g_sequence_free(item->prop_sequence);
    if(item->prop_iter_table!=NULL)
        g_hash_table_destroy(item->prop_iter_table);
    if(item->prop_name_table!=NULL)
        g_hash_table_destroy(item->prop_name_table);
    if(item->prop_uri_table!=NULL)
        g_hash_table_destroy(item->prop_uri_table);
    g_free(item);
}

static gboolean rclib_db_library_changed_entry_idle_cb(gpointer data)
{
    GObject *instance;
    RCLibDbPrivate *priv;
    gchar *uri = (gchar *)data;
    if(data==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    g_rw_lock_reader_lock(&(priv->library_rw_lock));
    if(!g_hash_table_contains(priv->library_table, uri)) return FALSE;
    g_signal_emit_by_name(instance, "library-changed", uri);
    g_free(uri);
    g_rw_lock_reader_unlock(&(priv->library_rw_lock));
    return FALSE;
}

static gboolean rclib_db_library_deleted_entry_idle_cb(gpointer data)
{
    GObject *instance;
    RCLibDbPrivate *priv;
    gchar *uri = (gchar *)data;
    if(data==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    g_signal_emit_by_name(instance, "library-deleted", uri);
    g_free(uri);
    return FALSE;
}

static void rclib_db_library_query_result_added_cb(RCLibDb *db,
    const gchar *uri, gpointer data)
{
    RCLibDbLibraryQueryResult *object;
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryData *library_data;
    RCLibDbLibraryQueryResultPropItem *prop_item;
    RCLibDbLibraryQueryResultPropData *prop_data;
    guint prop_type;
    GHashTableIter prop_iter;
    gchar *prop_string;
    GSequenceIter *iter = NULL;
    object = (RCLibDbLibraryQueryResult *)data;
    if(data==NULL || uri==NULL) return;
    priv = object->priv;
    if(priv==NULL) return;
    if(priv->query==NULL) return;
    library_data = rclib_db_library_get_data(uri);
    if(library_data==NULL) return;
    if(rclib_db_library_data_query(library_data, priv->query, NULL) &&
        !g_hash_table_contains(priv->query_uri_table, uri))
    {
        if(priv->list_need_sort && priv->query_sequence!=NULL)
        {
            iter = g_sequence_insert_sorted(priv->query_sequence, 
                rclib_db_library_data_ref(library_data),
                rclib_db_library_query_result_item_compare_func, priv);
            g_hash_table_replace(priv->query_iter_table, iter, iter);
        }
        
        g_hash_table_replace(priv->query_uri_table, g_strdup(uri), iter);
        g_signal_emit(object, db_library_query_result_signals[
            SIGNAL_LIBRARY_QUERY_RESULT_ADDED], 0, uri);
            
        g_hash_table_iter_init(&prop_iter, priv->prop_table);
        while(g_hash_table_iter_next(&prop_iter, (gpointer *)&prop_type,
            (gpointer *)&prop_item))
        {
            if(prop_item==NULL) continue;
            if(rclib_db_query_get_query_data_type((RCLibDbQueryDataType)
                prop_type)!=G_TYPE_STRING)
            {
                continue;
            }
            prop_string = NULL;
            rclib_db_library_data_get(library_data,
                rclib_db_library_query_type_to_data_type(prop_type),
                &prop_string, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            if(prop_string==NULL)
                prop_string = g_strdup("");
            iter = g_hash_table_lookup(prop_item->prop_name_table,
                prop_string);
            if(iter!=NULL)
            {
                g_hash_table_insert(prop_item->prop_uri_table, g_strdup(uri),
                    iter);
                prop_data = g_sequence_get(iter);
                prop_data->prop_count++;
                prop_item->count++;
                g_signal_emit(object, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0, prop_type,
                    prop_string);
            }
            else
            {
                prop_data = rclib_db_library_query_result_prop_data_new();
                prop_data->prop_count = 1;
                prop_item->count++;
                prop_data->prop_name = g_strdup(prop_string);
                iter = g_sequence_insert_sorted(prop_item->prop_sequence,
                    prop_data,
                    rclib_db_library_query_result_prop_item_compare_func,
                    priv);
                g_hash_table_insert(prop_item->prop_iter_table, iter, iter);
                g_hash_table_insert(prop_item->prop_name_table,
                    g_strdup(prop_string), iter);
                g_hash_table_insert(prop_item->prop_uri_table,
                    g_strdup(uri), iter);
                g_signal_emit(object, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_ADDED], 0, prop_type,
                    prop_string);
            }
            g_free(prop_string);
        }
    }
    rclib_db_library_data_unref(library_data);
}

static void rclib_db_library_query_result_deleted_cb(RCLibDb *db,
    const gchar *uri, gpointer data)
{
    RCLibDbLibraryQueryResult *object;
    RCLibDbLibraryQueryResultPrivate *priv;
    GSequenceIter *iter;
    GHashTableIter prop_iter;
    RCLibDbLibraryQueryResultPropItem *prop_item;
    RCLibDbLibraryQueryResultPropData *prop_data;
    guint prop_type;
    object = (RCLibDbLibraryQueryResult *)data;
    if(data==NULL || uri==NULL) return;
    priv = object->priv;
    if(priv==NULL) return;
    if(g_hash_table_contains(priv->query_uri_table, uri))
    {
        g_signal_emit(object, db_library_query_result_signals[
            SIGNAL_LIBRARY_QUERY_RESULT_DELETE], 0, uri);
        iter = g_hash_table_lookup(priv->query_uri_table, uri);
        if(iter!=NULL)
        {
            g_sequence_remove(iter);
            g_hash_table_remove(priv->query_iter_table, iter);
        }
        g_hash_table_remove(priv->query_uri_table, uri);
    }
    
    g_hash_table_iter_init(&prop_iter, priv->prop_table);
    while(g_hash_table_iter_next(&prop_iter, (gpointer *)&prop_type,
        (gpointer *)&prop_item))
    {
        if(prop_item==NULL) continue;
        iter = g_hash_table_lookup(prop_item->prop_uri_table, uri);
        if(iter==NULL) continue;
        prop_data = g_sequence_get(iter);
        if(prop_data==NULL) continue;
        if(prop_data->prop_count>1)
        {
            prop_data->prop_count--;
            prop_item->count--;
            g_hash_table_remove(prop_item->prop_uri_table, uri);
            if(prop_data->prop_name!=NULL && strlen(prop_data->prop_name)>0)
            {
                g_signal_emit(object, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0, prop_type,
                    prop_data->prop_name);
            }
            else
            {
                g_signal_emit(object, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0, prop_type,
                    "");
            }
        }
        else
        {
            prop_item->count--;
            if(prop_data->prop_name!=NULL && strlen(prop_data->prop_name)>0)
            {
                g_signal_emit(object, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE], 0, prop_type,
                    prop_data->prop_name);
            }
            else
            {
                g_signal_emit(object, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE], 0, prop_type,
                    "");
            }
            g_hash_table_remove(prop_item->prop_name_table,
                prop_data->prop_name);
            g_hash_table_remove(prop_item->prop_iter_table, iter);
            g_hash_table_remove(prop_item->prop_uri_table, uri);
            g_sequence_remove(iter);
        }
    }
}

static void rclib_db_library_query_result_changed_cb(RCLibDb *db,
    const gchar *uri, gpointer data)
{
    RCLibDbLibraryQueryResult *object;
    RCLibDbLibraryData *library_data;
    GHashTableIter prop_iter;
    RCLibDbLibraryQueryResultPropItem *prop_item;
    RCLibDbLibraryQueryResultPropData *prop_data;
    RCLibDbLibraryQueryResultPrivate *priv;
    gchar *prop_string;
    guint prop_type;
    GSequenceIter *iter, *prop_item_iter;
    gboolean exist = FALSE;
    object = (RCLibDbLibraryQueryResult *)data;
    if(data==NULL || uri==NULL) return;
    priv = object->priv;
    if(priv==NULL) return;
    if(g_hash_table_contains(priv->query_uri_table, uri))
    {
        library_data = rclib_db_library_get_data(uri);
        iter = g_hash_table_lookup(priv->query_uri_table, uri);
        if(library_data!=NULL)
        {
            if(!rclib_db_library_data_query(library_data, priv->query, NULL))
            {
                g_signal_emit(object, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_DELETE], 0, uri);
                if(iter!=NULL)
                {
                    g_sequence_remove(iter);
                    g_hash_table_remove(priv->query_iter_table, iter);
                }
                g_hash_table_remove(priv->query_uri_table, uri);
                
                g_hash_table_iter_init(&prop_iter, priv->prop_table);
                while(g_hash_table_iter_next(&prop_iter,
                    (gpointer *)&prop_type, (gpointer *)&prop_item))
                {
                    if(prop_item==NULL) continue;
                    if(rclib_db_query_get_query_data_type(
                        (RCLibDbQueryDataType)prop_type)!=G_TYPE_STRING)
                    {
                        continue;
                    }
                    prop_string = NULL;
                    rclib_db_library_data_get(library_data,
                        rclib_db_library_query_type_to_data_type(prop_type),
                        &prop_string, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                    if(prop_string==NULL)
                        prop_string = g_strdup("");
                    prop_item_iter = g_hash_table_lookup(
                        prop_item->prop_name_table, prop_string);
                    if(prop_item_iter==NULL)
                    {
                        g_free(prop_string);
                        continue;
                    }
                    prop_data = g_sequence_get(prop_item_iter);
                    if(prop_data==NULL)
                    {
                        g_free(prop_string);
                        continue;
                    }
                    if(prop_data->prop_count>1)
                    {
                        prop_data->prop_count--;
                        prop_item->count--;
                        g_hash_table_remove(prop_item->prop_uri_table, uri);
                        g_signal_emit(object, db_library_query_result_signals[
                            SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0,
                            prop_type, prop_string);
                    }
                    else
                    {
                        prop_item->count--;
                        g_signal_emit(object, db_library_query_result_signals[
                            SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE], 0,
                            prop_type, prop_string);
                        g_sequence_remove(prop_item_iter);
                        g_hash_table_remove(prop_item->prop_name_table,
                            prop_string);
                        g_hash_table_remove(prop_item->prop_iter_table,
                            prop_item_iter);
                        g_hash_table_remove(prop_item->prop_uri_table, uri);
                    }
                    g_free(prop_string);
                }
            }
            else
            {
                exist = TRUE;
            }
        }
        if(exist)
        {
            g_signal_emit(object, db_library_query_result_signals[
                SIGNAL_LIBRARY_QUERY_RESULT_CHANGED], 0, uri);
                
            g_hash_table_iter_init(&prop_iter, priv->prop_table);
            while(g_hash_table_iter_next(&prop_iter,
                (gpointer *)&prop_type, (gpointer *)&prop_item))
            {
                if(prop_item==NULL) continue;
                if(rclib_db_query_get_query_data_type(
                    (RCLibDbQueryDataType)prop_type)!=G_TYPE_STRING)
                {
                    continue;
                }
                prop_string = NULL;
                rclib_db_library_data_get(library_data,
                    rclib_db_library_query_type_to_data_type(prop_type),
                    &prop_string, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                if(prop_string==NULL)
                    prop_string = g_strdup("");
                prop_item_iter = g_hash_table_lookup(
                    prop_item->prop_name_table, prop_string);
                if(prop_item_iter==NULL)
                {
                    prop_data = rclib_db_library_query_result_prop_data_new();
                    prop_data->prop_count = 1;
                    prop_data->prop_name = g_strdup(prop_string);
                    prop_item->count++;
                    prop_item_iter = g_sequence_insert_sorted(
                        prop_item->prop_sequence, prop_data,
                        rclib_db_library_query_result_prop_item_compare_func,
                        priv);
                    g_hash_table_insert(prop_item->prop_iter_table,
                        prop_item_iter, prop_item_iter);
                    g_hash_table_insert(prop_item->prop_name_table,
                        g_strdup(prop_string), prop_item_iter);
                    g_hash_table_insert(prop_item->prop_uri_table,
                        g_strdup(uri), prop_item_iter);
                    g_signal_emit(object, db_library_query_result_signals[
                        SIGNAL_LIBRARY_QUERY_RESULT_PROP_ADDED], 0, prop_type,
                        prop_string);
                }
                else
                {
                    prop_data = g_sequence_get(prop_item_iter);
                    if(prop_data==NULL)
                    {
                        g_free(prop_string);
                        continue;
                    }
                    
                    if(g_strcmp0(prop_data->prop_name, prop_string)!=0)
                    {
                        if(prop_data->prop_count>1)
                        {
                            prop_data->prop_count--;
                            prop_item->count--;
                            g_hash_table_remove(prop_item->prop_uri_table, uri);
                            g_signal_emit(object,
                                db_library_query_result_signals[
                                SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0,
                                prop_type, prop_string);
                        }
                        else
                        {
                            prop_item->count--;
                            g_signal_emit(object,
                                db_library_query_result_signals[
                                SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE], 0,
                                prop_type, prop_string);
                            g_sequence_remove(prop_item_iter);
                            g_hash_table_remove(prop_item->prop_name_table,
                                prop_string);
                            g_hash_table_remove(prop_item->prop_iter_table,
                                prop_item_iter);
                            g_hash_table_remove(prop_item->prop_uri_table,
                                uri);
                        }
                    }
                }
                g_free(prop_string);
            }
        }
        rclib_db_library_data_unref(library_data);
    }
    else
    {
        rclib_db_library_query_result_added_cb(db, uri, data);
    }
}

static void rclib_db_library_query_result_base_added_cb(
    RCLibDbLibraryQueryResult *base, const gchar *uri,
    gpointer data)
{
    RCLibDbLibraryQueryResult *qr;
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryData *library_data;
    RCLibDbLibraryQueryResultPropItem *prop_item;
    RCLibDbLibraryQueryResultPropData *prop_data;
    gchar *prop_string;
    guint prop_type;
    GHashTableIter prop_iter;
    GSequenceIter *iter = NULL;
    qr = (RCLibDbLibraryQueryResult *)data;
    if(data==NULL) return;
    priv = qr->priv;
    if(uri==NULL) return;
    if(priv==NULL || priv->base_query_result==NULL) return;
    library_data = rclib_db_library_get_data(uri);
    if(library_data==NULL) return;
    library_data = rclib_db_library_data_ref(library_data);
    if(priv->query!=NULL && !rclib_db_library_data_query(library_data,
        priv->query, NULL))
    {
        rclib_db_library_data_unref(library_data);
        return;
    }
    if(g_hash_table_contains(priv->query_uri_table, uri))
    {
        rclib_db_library_data_unref(library_data);
        return;
    }
    if(priv->list_need_sort && priv->query_sequence!=NULL)
    {
        iter = g_sequence_insert_sorted(priv->query_sequence, 
            rclib_db_library_data_ref(library_data),
            rclib_db_library_query_result_item_compare_func, priv);
        g_hash_table_replace(priv->query_iter_table, iter, iter);
    }
    
    g_hash_table_replace(priv->query_uri_table, g_strdup(uri), iter);
    g_signal_emit(qr, db_library_query_result_signals[
        SIGNAL_LIBRARY_QUERY_RESULT_ADDED], 0, uri);
    
    g_hash_table_iter_init(&prop_iter, priv->prop_table);
    while(g_hash_table_iter_next(&prop_iter, (gpointer *)&prop_type,
        (gpointer *)&prop_item))
    {
        if(prop_item==NULL) continue;
        if(rclib_db_query_get_query_data_type((RCLibDbQueryDataType)
            prop_type)!=G_TYPE_STRING)
        {
            continue;
        }
        prop_string = NULL;
        rclib_db_library_data_get(library_data,
            rclib_db_library_query_type_to_data_type(prop_type), &prop_string,
            RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
        if(prop_string==NULL)
            prop_string = g_strdup("");
        iter = g_hash_table_lookup(prop_item->prop_name_table,
            prop_string);
        if(iter!=NULL)
        {
            g_hash_table_insert(prop_item->prop_uri_table, g_strdup(uri),
                iter);
            prop_data = g_sequence_get(iter);
            prop_data->prop_count++;
            prop_item->count++;
            g_signal_emit(qr, db_library_query_result_signals[
                SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0, prop_type,
                prop_string);
        }
        else
        {
            prop_data = rclib_db_library_query_result_prop_data_new();
            prop_data->prop_count = 1;
            prop_data->prop_name = g_strdup(prop_string);
            prop_item->count++;
            iter = g_sequence_insert_sorted(prop_item->prop_sequence,
                prop_data,
                rclib_db_library_query_result_prop_item_compare_func, priv);
            g_hash_table_insert(prop_item->prop_iter_table, iter, iter);
            g_hash_table_insert(prop_item->prop_name_table,
                g_strdup(prop_string), iter);
            g_hash_table_insert(prop_item->prop_uri_table,
                g_strdup(uri), iter);
            g_signal_emit(qr, db_library_query_result_signals[
                SIGNAL_LIBRARY_QUERY_RESULT_PROP_ADDED], 0, prop_type,
                prop_string);
        }
        g_free(prop_string);
    }
    
    rclib_db_library_data_unref(library_data);
}

static void rclib_db_library_query_result_base_delete_cb(
    RCLibDbLibraryQueryResult *base, const gchar *uri, gpointer data)
{
    RCLibDbLibraryQueryResult *qr;
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *prop_item;
    RCLibDbLibraryQueryResultPropData *prop_data;
    guint prop_type;
    GHashTableIter prop_iter;
    qr = (RCLibDbLibraryQueryResult *)data;
    GSequenceIter *iter;
    if(data==NULL) return;
    if(uri==NULL) return;
    priv = qr->priv;
    if(priv==NULL) return;
    if(g_hash_table_contains(priv->query_uri_table, uri))
    {
        iter = g_hash_table_lookup(priv->query_uri_table, uri);
        g_signal_emit(qr, db_library_query_result_signals[
            SIGNAL_LIBRARY_QUERY_RESULT_DELETE], 0, uri);
        if(iter!=NULL)
        {
            g_sequence_remove(iter);
            g_hash_table_remove(priv->query_iter_table, iter);
        }
        g_hash_table_remove(priv->query_uri_table, uri);
    }

    g_hash_table_iter_init(&prop_iter, priv->prop_table);
    while(g_hash_table_iter_next(&prop_iter, (gpointer *)&prop_type,
        (gpointer *)&prop_item))
    {
        if(prop_item==NULL) continue;
        iter = g_hash_table_lookup(prop_item->prop_uri_table, uri);
        if(iter==NULL) continue;
        prop_data = g_sequence_get(iter);
        if(prop_data==NULL) continue;
        if(prop_data->prop_count>1)
        {
            prop_data->prop_count--;
            prop_item->count--;
            g_hash_table_remove(prop_item->prop_uri_table, uri);
            if(prop_data->prop_name!=NULL && strlen(prop_data->prop_name)>0)
            {
                g_signal_emit(qr, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0, prop_type,
                    prop_data->prop_name);
            }
            else
            {
                g_signal_emit(qr, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0, prop_type,
                    "");
            }
        }
        else
        {
            prop_item->count--;
            if(prop_data->prop_name!=NULL && strlen(prop_data->prop_name)>0)
            {
                g_signal_emit(qr, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE], 0, prop_type,
                    prop_data->prop_name);
            }
            else
            {
                g_signal_emit(qr, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE], 0, prop_type,
                    "");
            }
            g_hash_table_remove(prop_item->prop_name_table,
                prop_data->prop_name);
            g_hash_table_remove(prop_item->prop_iter_table, iter);
            g_hash_table_remove(prop_item->prop_uri_table, uri);
            g_sequence_remove(iter);
        }
    }
}

static void rclib_db_library_query_result_base_changed_cb(
    RCLibDbLibraryQueryResult *base, const gchar *uri, gpointer data)
{
    RCLibDbLibraryQueryResult *qr;
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryData *library_data;
    GSequenceIter *iter, *prop_item_iter;
    RCLibDbLibraryQueryResultPropItem *prop_item;
    RCLibDbLibraryQueryResultPropData *prop_data;
    gchar *prop_string;
    guint prop_type;
    GHashTableIter prop_iter;
    qr = (RCLibDbLibraryQueryResult *)data;
    if(data==NULL) return;
    priv = qr->priv;
    if(priv==NULL || priv->base_query_result==NULL) return;
    library_data = rclib_db_library_get_data(uri);
    if(library_data==NULL) return;
    library_data = rclib_db_library_data_ref(library_data);
    if(!g_hash_table_contains(priv->query_uri_table, uri))
    {
        rclib_db_library_query_result_base_added_cb(base, uri, data);
        rclib_db_library_data_unref(library_data);
        return;
    }
    if(priv->query!=NULL && !rclib_db_library_data_query(library_data,
        priv->query, NULL))
    {
        iter = g_hash_table_lookup(priv->query_uri_table, uri);
        g_signal_emit(qr, db_library_query_result_signals[
            SIGNAL_LIBRARY_QUERY_RESULT_DELETE], 0, uri);
        if(iter!=NULL)
        {
            g_sequence_remove(iter);
            g_hash_table_remove(priv->query_iter_table, iter);
        }
        g_hash_table_remove(priv->query_uri_table, uri);
        
        g_hash_table_iter_init(&prop_iter, priv->prop_table);
        while(g_hash_table_iter_next(&prop_iter,
            (gpointer *)&prop_type, (gpointer *)&prop_item))
        {
            if(prop_item==NULL) continue;
            prop_item_iter = g_hash_table_lookup(prop_item->prop_uri_table,
                uri);
            if(prop_item_iter==NULL) continue;
            prop_data = g_sequence_get(prop_item_iter);
            if(prop_data==NULL) continue;
            if(prop_data->prop_count>1)
            {
                prop_data->prop_count--;
                prop_item->count--;
                g_hash_table_remove(prop_item->prop_uri_table, uri);
                if(prop_data->prop_name!=NULL &&
                    strlen(prop_data->prop_name)>0)
                {
                    g_signal_emit(qr, db_library_query_result_signals[
                        SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0,
                        prop_type, prop_data->prop_name);
                }
                else
                {
                    g_signal_emit(qr, db_library_query_result_signals[
                        SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0,
                        prop_type, "");
                }
            }
            else
            {
                prop_item->count--;
                if(prop_data->prop_name!=NULL &&
                    strlen(prop_data->prop_name)>0)
                {
                    g_signal_emit(qr, db_library_query_result_signals[
                        SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE], 0,
                        prop_type, prop_data->prop_name);
                }
                else
                {
                    g_signal_emit(qr, db_library_query_result_signals[
                        SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE], 0,
                        prop_type, "");
                }
                g_sequence_remove(prop_item_iter);
                g_hash_table_remove(prop_item->prop_name_table,
                    prop_string);
                g_hash_table_remove(prop_item->prop_iter_table,
                    prop_item_iter);
                g_hash_table_remove(prop_item->prop_uri_table, uri);
            }
        }
    }
    else
    {
        g_signal_emit(qr, db_library_query_result_signals[
            SIGNAL_LIBRARY_QUERY_RESULT_CHANGED], 0, uri);
    
        g_hash_table_iter_init(&prop_iter, priv->prop_table);
        while(g_hash_table_iter_next(&prop_iter,
            (gpointer *)&prop_type, (gpointer *)&prop_item))
        {
            if(prop_item==NULL) continue;
            if(rclib_db_query_get_query_data_type(
                (RCLibDbQueryDataType)prop_type)!=G_TYPE_STRING)
            {
                continue;
            }
            prop_string = NULL;
            rclib_db_library_data_get(library_data,
                rclib_db_library_query_type_to_data_type(prop_type),
                &prop_string, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            if(prop_string==NULL)
                prop_string = g_strdup("");
            prop_item_iter = g_hash_table_lookup(
                prop_item->prop_name_table, prop_string);
            if(prop_item_iter==NULL)
            {
                prop_data = rclib_db_library_query_result_prop_data_new();
                prop_data->prop_count = 1;
                prop_data->prop_name = g_strdup(prop_string);
                prop_item->count++;
                prop_item_iter = g_sequence_insert_sorted(
                    prop_item->prop_sequence, prop_data,
                    rclib_db_library_query_result_prop_item_compare_func,
                    priv);
                g_hash_table_insert(prop_item->prop_iter_table,
                    prop_item_iter, prop_item_iter);
                g_hash_table_insert(prop_item->prop_name_table,
                    g_strdup(prop_string), prop_item_iter);
                g_hash_table_insert(prop_item->prop_uri_table,
                    g_strdup(uri), prop_item_iter);
                g_signal_emit(qr, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_ADDED], 0, prop_type,
                    prop_string);
            }
            else
            {
                prop_data = g_sequence_get(prop_item_iter);
                if(prop_data==NULL)
                {
                    g_free(prop_string);
                    continue;
                }
                
                if(g_strcmp0(prop_data->prop_name, prop_string)!=0)
                {
                    if(prop_data->prop_count>1)
                    {
                        prop_data->prop_count--;
                        prop_item->count--;
                        g_hash_table_remove(prop_item->prop_uri_table, uri);
                        g_signal_emit(qr, db_library_query_result_signals[
                            SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0,
                            prop_type, prop_string);
                    }
                    else
                    {
                        prop_item->count--;
                        g_signal_emit(qr, db_library_query_result_signals[
                            SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE], 0,
                            prop_type, prop_string);
                        g_sequence_remove(prop_item_iter);
                        g_hash_table_remove(prop_item->prop_name_table,
                            prop_string);
                        g_hash_table_remove(prop_item->prop_iter_table,
                            prop_item_iter);
                        g_hash_table_remove(prop_item->prop_uri_table,
                            uri);
                    }
                }    
            }
            g_free(prop_string);
        }
    }
    
    rclib_db_library_data_unref(library_data);
}

static inline gboolean rclib_db_library_keyword_table_add_entry(
    GHashTable *library_keyword_table, RCLibDbLibraryData *library_data,
    const gchar *keyword)
{
    gboolean present = TRUE;
    GHashTable *keyword_table;
    keyword_table = g_hash_table_lookup(library_keyword_table, keyword);
    if(keyword_table!=NULL)
    {
        present = (g_hash_table_lookup(keyword_table, library_data)!=NULL);
        g_hash_table_insert(keyword_table, library_data, GINT_TO_POINTER(1));
    }
    else
    {
        present = FALSE;
        keyword_table = g_hash_table_new(g_direct_hash, g_direct_equal);
        g_hash_table_insert(keyword_table, library_data, GINT_TO_POINTER(1));
        g_hash_table_insert(library_keyword_table, g_strdup(keyword),
            keyword_table);
    }
    return present;
}

static inline gboolean rclib_db_library_keyword_table_remove_entry(
    GHashTable *library_keyword_table, RCLibDbLibraryData *library_data,
    const gchar *keyword)
{
    GHashTable *keyword_table;
    gboolean ret;
    keyword_table = g_hash_table_lookup(library_keyword_table, keyword);
    if(keyword_table!=NULL)
    {
        ret = g_hash_table_remove(keyword_table, library_data);
        if(g_hash_table_size(keyword_table)==0)
        {
            g_hash_table_remove(library_keyword_table, keyword);
        }
    }
    else
    {
        ret = FALSE;
    }
    return ret;
}

static inline gboolean rclib_db_library_keyword_table_contains_entry(
    GHashTable *library_keyword_table, RCLibDbLibraryData *library_data,
    const gchar *keyword)
{
    GHashTable *keyword_table;
    gboolean ret;
    keyword_table = g_hash_table_lookup(library_keyword_table, keyword);
    if(keyword_table!=NULL)
    {
        ret = (g_hash_table_lookup(keyword_table, library_data)!=NULL);
    }
    else
    {
        ret = FALSE;
    }
    return ret;
}

static inline void rclib_db_library_import_idle_data_free(
    RCLibDbLibraryImportIdleData *data)
{
    if(data==NULL) return;
    if(data->mmd!=NULL) rclib_tag_free(data->mmd);
    g_free(data);
}

static inline void rclib_db_library_refresh_idle_data_free(
    RCLibDbLibraryRefreshIdleData *data)
{
    if(data==NULL) return;
    if(data->mmd!=NULL) rclib_tag_free(data->mmd);
    g_free(data);
}

gboolean _rclib_db_library_import_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbLibraryData *library_data;
    RCLibTagMetadata *mmd;
    RCLibDbLibraryImportIdleData *idle_data;
    if(data==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    idle_data = (RCLibDbLibraryImportIdleData *)data;
    if(idle_data->mmd==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    if(idle_data->mmd->uri==NULL)
    {
        rclib_db_library_import_idle_data_free(idle_data);
        return FALSE;
    }
    mmd = idle_data->mmd;
    library_data = rclib_db_library_data_new();
    library_data->type = idle_data->type;
    library_data->uri = g_strdup(mmd->uri);
    library_data->title = g_strdup(mmd->title);
    library_data->artist = g_strdup(mmd->artist);
    library_data->album = g_strdup(mmd->album);
    library_data->ftype = g_strdup(mmd->ftype);
    library_data->genre = g_strdup(mmd->genre);
    library_data->length = mmd->length;
    library_data->tracknum = mmd->tracknum;
    library_data->year = mmd->year;
    library_data->rating = 3.0;
    _rclib_db_library_append_data_internal(library_data->uri,
        library_data);
    rclib_db_library_data_unref(library_data);
    priv->dirty_flag = TRUE;        
    rclib_db_library_import_idle_data_free(idle_data);
    return FALSE;
}

gboolean _rclib_db_library_refresh_idle_cb(gpointer data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbLibraryData *library_data;
    RCLibTagMetadata *mmd;
    RCLibDbLibraryRefreshIdleData *idle_data;
    if(data==NULL) return FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    idle_data = (RCLibDbLibraryRefreshIdleData *)data;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    if(instance==NULL) return FALSE;
    if(idle_data->mmd->uri==NULL)
    {
        rclib_db_library_refresh_idle_data_free(idle_data);
        return FALSE;
    }
    mmd = idle_data->mmd;
    g_rw_lock_writer_lock(&(priv->library_rw_lock));
    library_data = g_hash_table_lookup(priv->library_table, mmd->uri);
    if(library_data==NULL)
    {
        rclib_db_library_refresh_idle_data_free(idle_data);
        g_rw_lock_writer_unlock(&(priv->library_rw_lock));
        return FALSE;
    }
    library_data->type = idle_data->type;
    if(mmd!=NULL)
    {
        g_free(library_data->title);
        g_free(library_data->artist);
        g_free(library_data->album);
        g_free(library_data->ftype);
        library_data->title = g_strdup(mmd->title);
        library_data->artist = g_strdup(mmd->artist);
        library_data->album = g_strdup(mmd->album);
        library_data->ftype = g_strdup(mmd->ftype);
        library_data->genre = g_strdup(mmd->genre);
        library_data->length = mmd->length;
        library_data->tracknum = mmd->tracknum;
        library_data->year = mmd->year;
    }
    g_rw_lock_writer_unlock(&(priv->library_rw_lock));
    priv->dirty_flag = TRUE;
    
    /* Send a signal that the operation succeeded. */
    g_signal_emit_by_name(instance, "library-changed",
        library_data->uri);
    
    rclib_db_library_refresh_idle_data_free(idle_data);
    return FALSE;
}

void _rclib_db_library_append_data_internal(const gchar *uri,
    RCLibDbLibraryData *library_data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    g_rw_lock_writer_lock(&(priv->library_rw_lock));
    if(g_hash_table_contains(priv->library_table, uri))
    {
        g_rw_lock_writer_unlock(&(priv->library_rw_lock));
        return;
    }
    library_data = rclib_db_library_data_ref(library_data);
    g_hash_table_replace(priv->library_table, g_strdup(uri),
        library_data);
    g_hash_table_replace(priv->library_ptr_table, library_data,
        library_data);
    g_rw_lock_writer_unlock(&(priv->library_rw_lock));
    g_signal_emit_by_name(instance, "library-added", uri);
}

/**
 * rclib_db_library_data_new:
 * 
 * Create a new empty #RCLibDbLibraryData structure,
 * and set the reference count to 1. MT safe.
 *
 * Returns: The new empty allocated #RCLibDbLibraryData structure.
 */

RCLibDbLibraryData *rclib_db_library_data_new()
{
    RCLibDbLibraryData *data = g_slice_new0(RCLibDbLibraryData);
    g_rw_lock_init(&(data->lock));
    data->ref_count = 1;
    return data;
}

/**
 * rclib_db_library_data_ref:
 * @data: the #RCLibDbLibraryData structure
 *
 * Increase the reference of #RCLibDbLibraryData by 1. MT safe.
 *
 * Returns: (transfer none): The #RCLibDbLibraryData structure.
 */

RCLibDbLibraryData *rclib_db_library_data_ref(RCLibDbLibraryData *data)
{
    if(data==NULL) return NULL;
    g_atomic_int_add(&(data->ref_count), 1);
    return data;
}

/**
 * rclib_db_library_data_unref:
 * @data: the #RCLibDbLibraryData structure
 *
 * Decrease the reference of #RCLibDbLibraryData by 1.
 * If the reference down to zero, the structure will be freed.
 * MT safe.
 *
 * Returns: The #RCLibDbLibraryData structure.
 */

void rclib_db_library_data_unref(RCLibDbLibraryData *data)
{
    if(data==NULL) return;
    if(g_atomic_int_dec_and_test(&(data->ref_count)))
        rclib_db_library_data_free(data);
}

/**
 * rclib_db_library_data_free:
 * @data: the data to free
 *
 * Free the #RCLibDbLibraryData structure.
 * Please use #rclib_db_library_data_unref() in case of multi-threading.
 */

void rclib_db_library_data_free(RCLibDbLibraryData *data)
{
    if(data==NULL) return;
    g_rw_lock_writer_lock(&(data->lock));
    g_free(data->uri);
    g_free(data->title);
    g_free(data->artist);
    g_free(data->album);
    g_free(data->ftype);
    g_free(data->lyricfile);
    g_free(data->lyricsecfile);
    g_free(data->albumfile);
    g_free(data->genre);
    g_rw_lock_writer_unlock(&(data->lock));
    g_rw_lock_clear(&(data->lock));
    g_slice_free(RCLibDbLibraryData, data);
}

static gboolean rclib_db_library_query_result_query_idle_cb(gpointer data)
{
    RCLibDbLibraryQueryResult *object;
    RCLibDbLibraryQueryResultPrivate *priv;
    GPtrArray *result = NULL;
    RCLibDbLibraryData *library_data;
    RCLibDbLibraryQueryResultPropItem *prop_item;
    RCLibDbLibraryQueryResultPropData *prop_data;
    GHashTableIter prop_iter;
    guint prop_type;
    gchar *prop_string;
    guint i;
    gchar *uri;
    gpointer *idle_data = (gpointer *)data;
    GSequenceIter *iter;
    if(data==NULL) return FALSE;
    object = (RCLibDbLibraryQueryResult *)idle_data[0];
    result = (GPtrArray *)idle_data[1];
    if(result==NULL)
        return FALSE;
    if(object==NULL || object->priv==NULL)
    {
        if(result!=NULL)
        {
            g_ptr_array_free(result, TRUE);
            g_free(idle_data);
            return FALSE;
        }
    }
    priv = object->priv;
    for(i=0;i<result->len;i++)
    {
        library_data = g_ptr_array_index(result, i);
        if(library_data==NULL) continue;
        uri = NULL;
        iter = NULL;
        library_data = rclib_db_library_data_ref(library_data);
        rclib_db_library_data_get(library_data, RCLIB_DB_LIBRARY_DATA_TYPE_URI,
            &uri, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
        if(priv->list_need_sort && priv->query_sequence!=NULL)
        {
            iter = g_sequence_insert_sorted(priv->query_sequence, library_data,
                rclib_db_library_query_result_item_compare_func, priv);
            g_hash_table_replace(priv->query_iter_table, iter, iter);
        }
        
        g_hash_table_replace(priv->query_uri_table, g_strdup(uri), iter);
        g_signal_emit(object, db_library_query_result_signals[
            SIGNAL_LIBRARY_QUERY_RESULT_ADDED], 0, uri);
            
        g_hash_table_iter_init(&prop_iter, priv->prop_table);
        while(g_hash_table_iter_next(&prop_iter, (gpointer *)&prop_type,
            (gpointer *)&prop_item))
        {
            if(prop_item==NULL) continue;
            if(rclib_db_query_get_query_data_type((RCLibDbQueryDataType)
                prop_type)!=G_TYPE_STRING)
            {
                continue;
            }
            prop_string = NULL;
            rclib_db_library_data_get(library_data,
                rclib_db_library_query_type_to_data_type(prop_type),
                &prop_string, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            if(prop_string==NULL)
                prop_string = g_strdup("");
            iter = g_hash_table_lookup(prop_item->prop_name_table,
                prop_string);
            if(iter!=NULL)
            {
                prop_data = g_sequence_get(iter);
                prop_data->prop_count++;
                prop_item->count++;
                g_hash_table_insert(prop_item->prop_uri_table, g_strdup(uri),
                    iter);
                g_signal_emit(object, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0, prop_type,
                    prop_string);
            }
            else
            {
                prop_data = rclib_db_library_query_result_prop_data_new();
                prop_data->prop_count = 1;
                prop_data->prop_name = g_strdup(prop_string);
                prop_item->count++;
                iter = g_sequence_insert_sorted(prop_item->prop_sequence,
                    prop_data,
                    rclib_db_library_query_result_prop_item_compare_func,
                    priv);
                g_hash_table_insert(prop_item->prop_iter_table, iter, iter);
                g_hash_table_insert(prop_item->prop_name_table,
                    g_strdup(prop_string), iter);
                g_hash_table_insert(prop_item->prop_uri_table, g_strdup(uri),
                    iter);
                g_signal_emit(object, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_ADDED], 0, prop_type,
                    prop_string);
            }
            
            g_free(prop_string);
        }
    }
    g_free(idle_data);
    return FALSE;
}

static gpointer rclib_db_library_query_result_query_thread_cb(gpointer data)
{
    RCLibDbLibraryQueryResultPrivate *priv = NULL;
    RCLibDbLibraryQueryResult *object = RCLIB_DB_LIBRARY_QUERY_RESULT(data);
    RCLibDbQuery **query_data = NULL;
    RCLibDbQuery *query = NULL;
    gpointer *idle_data;
    GPtrArray *result;
    if(data==NULL) return NULL;
    priv = object->priv;
    if(priv==NULL) return NULL;
    while(priv->query_queue!=NULL)
    {
        query_data = g_async_queue_pop(priv->query_queue);
        query = *query_data;
        g_free(query_data);
        if(query==NULL) break;
        result = rclib_db_library_query(query, priv->cancellable);
        if(result==NULL) continue;
        idle_data = g_new(gpointer, 2);
        idle_data[0] = object;
        idle_data[1] = result;
        g_idle_add(rclib_db_library_query_result_query_idle_cb, idle_data);
    }
    return NULL;
}

static void rclib_db_library_query_result_finalize(GObject *object)
{
    RCLibDbLibraryQueryResultPrivate *priv =
        RCLIB_DB_LIBRARY_QUERY_RESULT(object)->priv;
    RCLibDbQuery **query_data = NULL;
    RCLIB_DB_LIBRARY_QUERY_RESULT(object)->priv = NULL;
    if(priv->base_added_id>0)
    {
        g_signal_handler_disconnect(priv->base_query_result,
            priv->base_added_id);
        priv->base_added_id = 0;
    }    
    if(priv->base_changed_id>0)
    {
        g_signal_handler_disconnect(priv->base_query_result,
            priv->base_changed_id);
        priv->base_changed_id = 0;
    }
    if(priv->base_delete_id>0)
    {
        g_signal_handler_disconnect(priv->base_query_result,
            priv->base_delete_id);
        priv->base_delete_id = 0;
    }
    if(priv->base_query_result!=NULL)
    {
        g_object_unref(priv->base_query_result);
        priv->base_query_result = NULL;
    }
    if(priv->library_added_id>0)
    {
        rclib_db_signal_disconnect(priv->library_added_id);
        priv->library_added_id = 0;
    }
    if(priv->library_changed_id>0)
    {
        rclib_db_signal_disconnect(priv->library_changed_id);
        priv->library_changed_id = 0;
    }
    if(priv->library_deleted_id>0)
    {
        rclib_db_signal_disconnect(priv->library_deleted_id);
        priv->library_deleted_id = 0;
    }
    if(priv->cancellable!=NULL)
    {
        g_cancellable_cancel(priv->cancellable);
        g_object_unref(priv->cancellable);
        priv->cancellable = NULL;
    }
    if(priv->query_queue!=NULL)
    {
        query_data = g_new(RCLibDbQuery *, 1);
        *query_data = NULL;
        g_async_queue_push(priv->query_queue, query_data);
    }
    if(priv->query_thread!=NULL)
    {
        g_thread_join(priv->query_thread);
        priv->query_thread = NULL;
    }
    if(priv->query_queue!=NULL)
    {
        g_async_queue_unref(priv->query_queue);
        priv->query_queue = NULL;
    }
    if(priv->query!=NULL)
    {
        rclib_db_query_free(priv->query);
        priv->query = NULL;
    }
    if(priv->query_iter_table!=NULL)
    {
        g_hash_table_destroy(priv->query_iter_table);
        priv->query_iter_table = NULL;
    }
    if(priv->query_uri_table!=NULL)
    {
        g_hash_table_destroy(priv->query_uri_table);
        priv->query_uri_table = NULL;
    }
    if(priv->query_sequence!=NULL)
    {
        g_sequence_free(priv->query_sequence);
        priv->query_sequence = NULL;
    }
    if(priv->prop_table!=NULL)
    {
        g_hash_table_destroy(priv->prop_table);
        priv->prop_table = NULL;
    }
    G_OBJECT_CLASS(rclib_db_library_query_result_parent_class)->
        finalize(object);
}

static void rclib_db_library_query_result_class_init(
    RCLibDbLibraryQueryResultClass *klass)
{
    GObjectClass *object_class = (GObjectClass *)klass;
    rclib_db_library_query_result_parent_class =
        g_type_class_peek_parent(klass);
    object_class->finalize = rclib_db_library_query_result_finalize;
    g_type_class_add_private(klass, sizeof(RCLibDbLibraryQueryResultPrivate));

    /**
     * RCLibDbLibraryQueryResult::query-result-added:
     * @qr: the #RCLibDb that received the signal
     * @uri: the URI of the added item
     * 
     * The ::query-result-added signal is emitted when a new item has been
     * added to the query result.
     */
    db_library_query_result_signals[SIGNAL_LIBRARY_QUERY_RESULT_ADDED] =
        g_signal_new("query-result-added", RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT,
        G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbLibraryQueryResultClass,
        query_result_added), NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING, NULL);

    /**
     * RCLibDbLibraryQueryResult::query-result-delete:
     * @qr: the #RCLibDb that received the signal
     * @uri: the URI of the deleted item
     * 
     * The ::query-result-delete signal is emitted before the @uri which
     * pointed to the item removed from the query result.
     */
    db_library_query_result_signals[SIGNAL_LIBRARY_QUERY_RESULT_DELETE] =
        g_signal_new("query-result-delete", RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT,
        G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbLibraryQueryResultClass,
        query_result_delete), NULL, NULL, g_cclosure_marshal_VOID__STRING,
        G_TYPE_NONE, 1, G_TYPE_STRING, NULL);

    /**
     * RCLibDbLibraryQueryResult::query-result-changed:
     * @qr: the #RCLibDb that received the signal
     * @uri: the URI of the changed item
     * 
     * The ::query-result-changed signal is emitted when the @uri which
     * pointed to the item has been changed.
     */
    db_library_query_result_signals[SIGNAL_LIBRARY_QUERY_RESULT_CHANGED] =
        g_signal_new("query-result-changed",
        RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibDbLibraryQueryResultClass, query_result_changed),
        NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1,
        G_TYPE_STRING, NULL);
        
    /**
     * RCLibDbLibraryQueryResult::query-result-reordered:
     * @qr: the #RCLibDb that received the signal
     * @new_order: an array of integers mapping the current position of each
     * playlist item to its old position before the re-ordering,
     * i.e. @new_order<literal>[newpos] = oldpos</literal>
     * 
     * The ::query-result-reordered signal is emitted when the query result
     * items have been reordered.
     */
    db_library_query_result_signals[SIGNAL_LIBRARY_QUERY_RESULT_REORDERED] =
        g_signal_new("query-result-reordered",
        RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibDbLibraryQueryResultClass, query_result_reordered),
        NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1,
        G_TYPE_POINTER, NULL);

    /**
     * RCLibDbLibraryQueryResult::prop-added:
     * @qr: the #RCLibDb that received the signal
     * @prop_type: the property type
     * @prop_string: the property text of the new item
     * 
     * The ::query-result-changed signal is emitted when new property item 
     * which the @prop)_string pointed to has been added.
     */
    db_library_query_result_signals[SIGNAL_LIBRARY_QUERY_RESULT_PROP_ADDED] =
        g_signal_new("prop-added", RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT,
        G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbLibraryQueryResultClass,
        prop_added), NULL, NULL, rclib_marshal_VOID__UINT_STRING,
        G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING, NULL);

    /**
     * RCLibDbLibraryQueryResult::prop-delete:
     * @qr: the #RCLibDb that received the signal
     * @prop_type: the property type
     * @prop_string: the property text of the item
     * 
     * The ::query-result-changed signal is emitted when the property item 
     * which the @prop_string pointed to is about to be deleted.
     */
    db_library_query_result_signals[SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE] =
        g_signal_new("prop-delete", RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT,
        G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbLibraryQueryResultClass,
        prop_delete), NULL, NULL, rclib_marshal_VOID__UINT_STRING,
        G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING, NULL);

    /**
     * RCLibDbLibraryQueryResult::prop-changed:
     * @qr: the #RCLibDb that received the signal
     * @prop_type: the property type
     * @prop_string: the property text of the item
     * 
     * The ::query-result-changed signal is emitted when the property item 
     * which the @prop_string pointed to has been changed.
     */
    db_library_query_result_signals[SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED] =
        g_signal_new("prop-changed", RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT,
        G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(RCLibDbLibraryQueryResultClass,
        prop_changed), NULL, NULL, rclib_marshal_VOID__UINT_STRING,
        G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING, NULL);
        
    /**
     * RCLibDbLibraryQueryResult::prop-reordered:
     * @qr: the #RCLibDb that received the signal
     * @prop_type: the property type
     * @new_order: an array of integers mapping the current position of each
     * playlist item to its old position before the re-ordering,
     * i.e. @new_order<literal>[newpos] = oldpos</literal>
     * 
     * The ::prop-reordered signal is emitted when the query result property
     * items have been reordered.
     */
    db_library_query_result_signals[
        SIGNAL_LIBRARY_QUERY_RESULT_PROP_REORDERED] =
        g_signal_new("prop-reordered",
        RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT, G_SIGNAL_RUN_FIRST,
        G_STRUCT_OFFSET(RCLibDbLibraryQueryResultClass, prop_reordered),
        NULL, NULL, rclib_marshal_VOID__UINT_POINTER, G_TYPE_NONE, 2,
        G_TYPE_UINT, G_TYPE_POINTER, NULL);
}

static void rclib_db_library_query_result_instance_init(
    RCLibDbLibraryQueryResult *object)
{
    RCLibDbLibraryQueryResultPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE(
        object, RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT,
        RCLibDbLibraryQueryResultPrivate);
    object->priv = priv;
    priv->sort_column = RCLIB_DB_LIBRARY_DATA_TYPE_TITLE;
    priv->sort_direction = FALSE;
    priv->query_sequence = g_sequence_new((GDestroyNotify)
        rclib_db_library_data_unref);
    priv->query_iter_table = g_hash_table_new(g_direct_hash,
        g_direct_equal);
    priv->query_uri_table = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, NULL);
    priv->prop_table = g_hash_table_new_full(g_direct_hash, g_direct_equal,
        NULL, (GDestroyNotify)rclib_db_library_query_result_prop_item_free);
    priv->base_query_result = NULL;
    priv->list_need_sort = TRUE;
}
    
GType rclib_db_library_query_result_get_type()
{
    static volatile gsize g_define_type_id__volatile = 0;
    GType g_define_type_id;
    static const GTypeInfo class_info = {
        .class_size = sizeof(RCLibDbLibraryQueryResultClass),
        .base_init = NULL,
        .base_finalize = NULL,
        .class_init = (GClassInitFunc)rclib_db_library_query_result_class_init,
        .class_finalize = NULL,
        .class_data = NULL,
        .instance_size = sizeof(RCLibDbLibraryQueryResult),
        .n_preallocs = 0,
        .instance_init = (GInstanceInitFunc)
            rclib_db_library_query_result_instance_init
    };
    if(g_once_init_enter(&g_define_type_id__volatile))
    {
        g_define_type_id = g_type_register_static(G_TYPE_OBJECT,
            g_intern_static_string("RCLibDbLibraryQueryResult"), &class_info,
            0);
        g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
    }
    return g_define_type_id__volatile;
}

gboolean _rclib_db_instance_init_library(RCLibDb *db, RCLibDbPrivate *priv)
{
    RCLibDbQueryDataType prop_types[2] = {RCLIB_DB_QUERY_DATA_TYPE_NONE,
        RCLIB_DB_QUERY_DATA_TYPE_NONE};
    if(db==NULL || priv==NULL) return FALSE;
    
    /* GSequence<RCLibDbLibraryData *> */
    priv->library_query = g_sequence_new((GDestroyNotify)
        rclib_db_library_data_unref);
        
    /* GHashTable<gchar *, RCLibDbLibraryData *> */
    priv->library_table = g_hash_table_new_full(g_str_hash,
        g_str_equal, g_free, (GDestroyNotify)rclib_db_library_data_unref);
    priv->library_ptr_table = g_hash_table_new(g_direct_hash, g_direct_equal);
        
    /* GHashTable<gchar *, GHashTable<RCLibDbLibraryData *, 1>> */
    priv->library_keyword_table = g_hash_table_new_full(g_str_hash,
        g_str_equal, g_free, (GDestroyNotify)g_hash_table_destroy);
    
    g_rw_lock_init(&(priv->library_rw_lock));
    
    prop_types[0] = RCLIB_DB_QUERY_DATA_TYPE_GENRE;
    priv->library_query_base = rclib_db_library_query_result_new(db, NULL, 
        prop_types);
    prop_types[0] = RCLIB_DB_QUERY_DATA_TYPE_ARTIST;
    priv->library_query_genre = rclib_db_library_query_result_new(db, 
        RCLIB_DB_LIBRARY_QUERY_RESULT(priv->library_query_base), prop_types);
    prop_types[0] = RCLIB_DB_QUERY_DATA_TYPE_ALBUM;
    priv->library_query_artist = rclib_db_library_query_result_new(db, 
        RCLIB_DB_LIBRARY_QUERY_RESULT(priv->library_query_genre), prop_types);
    prop_types[0] = RCLIB_DB_QUERY_DATA_TYPE_NONE;
    priv->library_query_album = rclib_db_library_query_result_new(db, 
        RCLIB_DB_LIBRARY_QUERY_RESULT(priv->library_query_artist), prop_types);

    return TRUE;
}

void _rclib_db_instance_finalize_library(RCLibDbPrivate *priv)
{
    if(priv==NULL) return;
    if(priv->library_query_album!=NULL)
    {
        g_object_unref(priv->library_query_album);
        priv->library_query_album = NULL;
    }
    if(priv->library_query_artist!=NULL)
    {
        g_object_unref(priv->library_query_artist);
        priv->library_query_artist = NULL;
    }
    if(priv->library_query_genre!=NULL)
    {
        g_object_unref(priv->library_query_genre);
        priv->library_query_genre = NULL;
    }
    if(priv->library_query_base!=NULL)
    {
        g_object_unref(priv->library_query_base);
        priv->library_query_base = NULL;
    }
    g_rw_lock_writer_lock(&(priv->library_rw_lock));
    if(priv->library_query!=NULL)
        g_sequence_free(priv->library_query);
    if(priv->library_keyword_table!=NULL)
        g_hash_table_destroy(priv->library_keyword_table);
    if(priv->library_table!=NULL)
        g_hash_table_destroy(priv->library_table);
    if(priv->library_ptr_table!=NULL)
        g_hash_table_destroy(priv->library_ptr_table);
    priv->library_query = NULL;
    priv->library_table = NULL;
    priv->library_ptr_table = NULL;
    g_rw_lock_writer_unlock(&(priv->library_rw_lock));
    g_rw_lock_clear(&(priv->library_rw_lock));
}

static inline gboolean rclib_db_library_data_set_valist(
    RCLibDbLibraryData *data, RCLibDbLibraryDataType type1,
    va_list var_args)
{
    gboolean send_signal = FALSE;
    RCLibDbLibraryDataType type;
    RCLibDbLibraryType new_type;
    const gchar *str;   
    gint64 length;
    gint vint;
    gdouble rating;
    type = type1;
    g_rw_lock_writer_lock(&(data->lock));
    while(type!=RCLIB_DB_LIBRARY_DATA_TYPE_NONE)
    {
        switch(type)
        {
            case RCLIB_DB_LIBRARY_DATA_TYPE_TYPE:
            {
                new_type = va_arg(var_args, RCLibDbLibraryType);
                if(new_type==data->type) break;
                data->type = new_type;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_URI:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->uri)==0) break;
                if(data->uri!=NULL) g_free(data->uri);
                data->uri = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_TITLE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->title)==0) break;
                if(data->title!=NULL) g_free(data->title);
                data->title = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->artist)==0) break;
                if(data->artist!=NULL) g_free(data->artist);
                data->artist = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->album)==0) break;
                if(data->album!=NULL) g_free(data->album);
                data->album = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->ftype)==0) break;
                if(data->ftype!=NULL) g_free(data->ftype);
                data->ftype = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH:
            {
                length = va_arg(var_args, gint64);
                if(data->length==length) break;
                data->length = length;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM:
            {
                vint = va_arg(var_args, gint);
                if(vint==data->tracknum) break;
                data->tracknum = vint;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_YEAR:
            {
                vint = va_arg(var_args, gint);
                if(vint==data->year) break;
                data->year = vint;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_RATING:
            {
                rating = va_arg(var_args, gdouble);
                if(rating==data->rating) break;
                data->rating = rating;
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LYRICFILE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->lyricfile)==0) break;
                if(data->lyricfile!=NULL) g_free(data->lyricfile);
                str = va_arg(var_args, const gchar *);
                data->lyricfile = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LYRICSECFILE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->lyricsecfile)==0) break;
                if(data->lyricsecfile!=NULL) g_free(data->lyricsecfile);
                data->lyricsecfile = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ALBUMFILE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->albumfile)==0) break;
                if(data->albumfile!=NULL) g_free(data->albumfile);
                data->albumfile = g_strdup(str);
                send_signal = TRUE;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_GENRE:
            {
                str = va_arg(var_args, const gchar *);
                if(g_strcmp0(str, data->genre)==0) break;
                if(data->genre!=NULL) g_free(data->genre);
                data->genre = g_strdup(str);
                send_signal = TRUE;
                break;
            }           
            default:
            {
                g_warning("rclib_db_library_data_set: Wrong data type %d!",
                    type);
                break;
            }
        }
        type = va_arg(var_args, RCLibDbLibraryDataType);
    }
    g_rw_lock_writer_unlock(&(data->lock));
    return send_signal;
}

static inline void rclib_db_library_data_get_valist(
    RCLibDbLibraryData *data, RCLibDbLibraryDataType type1,
    va_list var_args)
{
    RCLibDbLibraryDataType type;
    RCLibDbLibraryType *library_type;
    gchar **str;   
    gint64 *length;
    gint *vint;
    gfloat *rating;
    type = type1;
    g_rw_lock_reader_lock(&(data->lock));
    while(type!=RCLIB_DB_LIBRARY_DATA_TYPE_NONE)
    {
        switch(type)
        {
            case RCLIB_DB_LIBRARY_DATA_TYPE_TYPE:
            {
                library_type = va_arg(var_args, RCLibDbLibraryType *);
                *library_type = data->type;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_URI:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->uri);
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_TITLE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->title);
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->artist);
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->album);
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->ftype);
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH:
            {
                length = va_arg(var_args, gint64 *);
                *length = data->length;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM:
            {
                vint = va_arg(var_args, gint *);
                *vint = data->tracknum;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_YEAR:
            {
                vint = va_arg(var_args, gint *);
                *vint = data->year;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_RATING:
            {
                rating = va_arg(var_args, gfloat *);
                *rating = data->rating;
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LYRICFILE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->lyricfile);
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_LYRICSECFILE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->lyricsecfile);
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_ALBUMFILE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->albumfile);
                break;
            }
            case RCLIB_DB_LIBRARY_DATA_TYPE_GENRE:
            {
                str = va_arg(var_args, gchar **);
                *str = g_strdup(data->genre);
                break;
            }
            default:
            {
                g_warning("rclib_db_library_data_get: Wrong data type %d!",
                    type);
                break;
            }
        }
        type = va_arg(var_args, RCLibDbLibraryDataType);
    }
    g_rw_lock_reader_unlock(&(data->lock));
}

/**
 * rclib_db_library_data_set: (skip)
 * @data: the #RCLibDbLibraryDataType data
 * @type1: the first property in playlist data to set
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %RCLIB_DB_LIBRARY_DATA_TYPE_NONE
 *
 * Sets properties on a #RCLibDbPlaylistData. MT safe.
 */

void rclib_db_library_data_set(RCLibDbLibraryData *data,
    RCLibDbLibraryDataType type1, ...)
{
    RCLibDbPrivate *priv = NULL;
    GObject *instance = NULL;
    gboolean exist;
    va_list var_args;
    gboolean send_signal = FALSE;
    if(data==NULL) return;
    instance = rclib_db_get_instance();
    if(instance!=NULL)
        priv = RCLIB_DB(instance)->priv;
    if(priv!=NULL)
    {
        g_rw_lock_reader_lock(&(priv->library_rw_lock));
        exist = g_hash_table_contains(priv->library_ptr_table, data);
        g_rw_lock_reader_unlock(&(priv->library_rw_lock));
        if(!exist) return;
    }
    va_start(var_args, type1);
    send_signal = rclib_db_library_data_set_valist(data, type1, var_args);
    va_end(var_args);
    if(send_signal)
    {
        instance = rclib_db_get_instance();
        if(instance!=NULL)
            priv = RCLIB_DB(instance)->priv;
        if(data->uri!=NULL)
        {
            if(priv!=NULL)
                priv->dirty_flag = TRUE;
            g_main_context_invoke(NULL, rclib_db_library_changed_entry_idle_cb,
                g_strdup(data->uri));
        }
    }
}

/**
 * rclib_db_library_data_get: (skip)
 * @data: the #RCLibDbLibraryData data
 * @type1: the first property in library data to get
 * @...: return location for the first property, followed optionally by more
 *  name/return location pairs, followed by %RCLIB_DB_LIBRARY_DATA_TYPE_NONE
 *
 * Gets properties of a #RCLibDbLibraryData. The property contents will
 * be copied. MT safe.
 */

void rclib_db_library_data_get(RCLibDbLibraryData *data,
    RCLibDbLibraryDataType type1, ...)
{
    gboolean exist;
    GObject *instance = NULL;
    RCLibDbPrivate *priv = NULL;
    va_list var_args;
    if(data==NULL) return;
    instance = rclib_db_get_instance();
    if(instance!=NULL)
        priv = RCLIB_DB(instance)->priv;
    if(priv!=NULL)
    {
        g_rw_lock_reader_lock(&(priv->library_rw_lock));
        exist = g_hash_table_contains(priv->library_ptr_table, data);
        g_rw_lock_reader_unlock(&(priv->library_rw_lock));
        if(!exist) return;
    }
    va_start(var_args, type1);
    rclib_db_library_data_get_valist(data, type1, var_args);
    va_end(var_args);
}

/**
 * rclib_db_get_library_table:
 *
 * Get the library table.
 *
 * Returns: (transfer none): (skip): The library table, NULL if the
 *     table does not exist.
 */

GHashTable *rclib_db_get_library_table()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    return priv->library_table;
}

/**
 * rclib_db_library_has_uri:
 * @uri: the URI to find
 *
 * Check whether the give URI exists in the library. MT safe.
 *
 * Returns: Whether the URI exists in the library.
 */

gboolean rclib_db_library_has_uri(const gchar *uri)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    gboolean result = FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    g_rw_lock_reader_lock(&(priv->library_rw_lock));
    result = g_hash_table_contains(priv->library_table, uri);
    g_rw_lock_reader_unlock(&(priv->library_rw_lock));
    return result;
}

/**
 * rclib_db_library_exist:
 * @library_data: the library data
 * 
 * Check whether the library data exists in the library.
 * 
 * Returns: Whether the library data exists in the library.
 */

gboolean rclib_db_library_exist(RCLibDbLibraryData *library_data)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    gboolean result = FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return FALSE;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return FALSE;
    g_rw_lock_reader_lock(&(priv->library_rw_lock));
    result = g_hash_table_contains(priv->library_ptr_table, library_data);
    g_rw_lock_reader_unlock(&(priv->library_rw_lock));
    return result;
}

/**
 * rclib_db_library_add_music:
 * @uri: the URI of the music to add
 *
 * Add music to the music library. MT safe.
 */

void rclib_db_library_add_music(const gchar *uri)
{
    RCLibDbImportData *import_data;
    GObject *instance;
    RCLibDbPrivate *priv;
    instance = rclib_db_get_instance();
    if(uri==NULL || instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    priv->import_work_flag = TRUE;
    import_data = g_new0(RCLibDbImportData, 1);
    import_data->type = RCLIB_DB_IMPORT_TYPE_LIBRARY;
    import_data->uri = g_strdup(uri);
    g_async_queue_push(priv->import_queue, import_data);
}

/**
 * rclib_db_library_add_music_and_play:
 * @uri: the URI of the music to add
 *
 * Add music to the music library, and then play it if the add
 * operation succeeds. MT safe.
 */

void rclib_db_library_add_music_and_play(const gchar *uri)
{
    RCLibDbImportData *import_data;
    GObject *instance;
    RCLibDbPrivate *priv;
    instance = rclib_db_get_instance();
    if(uri==NULL || instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    priv->import_work_flag = TRUE;
    import_data = g_new0(RCLibDbImportData, 1);
    import_data->type = RCLIB_DB_IMPORT_TYPE_LIBRARY;
    import_data->uri = g_strdup(uri);
    import_data->play_flag = TRUE;
    g_async_queue_push(priv->import_queue, import_data);
}

/**
 * rclib_db_library_delete:
 * @uri: the URI of the music item to delete
 *
 * Delete the the music item by the given URI. MT safe.
 */

void rclib_db_library_delete(const gchar *uri)
{
    RCLibDbPrivate *priv;
    GObject *instance;
    RCLibDbLibraryData *library_data;
    if(uri==NULL) return;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    g_rw_lock_writer_lock(&(priv->library_rw_lock));
    if(!g_hash_table_contains(priv->library_table, uri)) return;
    library_data = g_hash_table_lookup(priv->library_table, uri);
    g_hash_table_remove(priv->library_ptr_table, library_data);
    g_hash_table_remove(priv->library_table, uri);
    g_rw_lock_writer_unlock(&(priv->library_rw_lock));
    g_idle_add(rclib_db_library_deleted_entry_idle_cb,
        g_strdup(uri));
    priv->dirty_flag = TRUE;
}

/**
 * rclib_db_library_get_data:
 * @uri: the URI of the #RCLibDbPlaylistData entry 
 * 
 * Get the library entry data which @uri points to. MT safe.
 * 
 * Returns: (transfer full): The library entry data, #NULL if not found.
 *     Free the data with #rclib_db_library_data_unref() after usage.
 */
 
RCLibDbLibraryData *rclib_db_library_get_data(const gchar *uri)
{
    RCLibDbLibraryData *library_data = NULL;
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    g_rw_lock_reader_lock(&(priv->library_rw_lock));
    library_data = g_hash_table_lookup(priv->library_table, uri);
    if(library_data!=NULL)
        library_data = rclib_db_library_data_ref(library_data);
    g_rw_lock_reader_unlock(&(priv->library_rw_lock));
    return library_data;
}

/**
 * rclib_db_library_data_uri_set: (skip)
 * @uri: the URI of the #RCLibDbPlaylistData entry 
 * @type1: the first property in playlist data to set
 * @...: value for the first property, followed optionally by more
 *  name/value pairs, followed by %RCLIB_DB_LIBRARY_DATA_TYPE_NONE
 *
 * Sets properties on the #RCLibDbPlaylistData which @uri points to.
 * MT safe.
 */

void rclib_db_library_data_uri_set(const gchar *uri,
    RCLibDbLibraryDataType type1, ...)
{
    RCLibDbPrivate *priv;
    RCLibDbLibraryData *library_data;
    va_list var_args;
    GObject *instance;
    gboolean send_signal = FALSE;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    g_rw_lock_writer_lock(&(priv->library_rw_lock));
    library_data = g_hash_table_lookup(priv->library_table, uri);
    if(library_data!=NULL)
    {
        va_start(var_args, type1);
        send_signal = rclib_db_library_data_set_valist(library_data, type1,
            var_args);
        va_end(var_args);
        if(send_signal)
        {
            priv->dirty_flag = TRUE;
            g_main_context_invoke(NULL, rclib_db_library_changed_entry_idle_cb,
                g_strdup(uri));
        }
    }
    g_rw_lock_writer_unlock(&(priv->library_rw_lock));
}

/**
 * rclib_db_library_data_uri_get: (skip)
 * @uri: the URI of the #RCLibDbPlaylistData entry 
 * @type1: the first property in library data to get
 * @...: return location for the first property, followed optionally by more
 *  name/return location pairs, followed by %RCLIB_DB_LIBRARY_DATA_TYPE_NONE
 *
 * Gets properties of the #RCLibDbLibraryData which @uri points to.
 * The property contents will be copied. MT safe.
 */

void rclib_db_library_data_uri_get(const gchar *uri,
    RCLibDbLibraryDataType type1, ...)
{
    RCLibDbPrivate *priv;
    RCLibDbLibraryData *library_data;
    va_list var_args;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return;
    g_rw_lock_reader_lock(&(priv->library_rw_lock));
    library_data = g_hash_table_lookup(priv->library_table, uri);
    if(library_data!=NULL)
    {
        va_start(var_args, type1);
        rclib_db_library_data_get_valist(library_data, type1,
            var_args);
        va_end(var_args);
    }
    g_rw_lock_reader_unlock(&(priv->library_rw_lock));
}

static inline RCLibDbLibraryDataType rclib_db_playlist_property_convert(
    RCLibDbQueryDataType query_type)
{
    switch(query_type)
    {
        case RCLIB_DB_QUERY_DATA_TYPE_URI:
            return RCLIB_DB_LIBRARY_DATA_TYPE_URI;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_TITLE:
            return RCLIB_DB_LIBRARY_DATA_TYPE_TITLE;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_ARTIST:
            return RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_ALBUM:
            return RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM;
        case RCLIB_DB_QUERY_DATA_TYPE_FTYPE:
            return RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_LENGTH:
            return RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_TRACKNUM:
            return RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_RATING:
            return RCLIB_DB_LIBRARY_DATA_TYPE_RATING;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_YEAR:
            return RCLIB_DB_LIBRARY_DATA_TYPE_YEAR;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_GENRE:
            return RCLIB_DB_LIBRARY_DATA_TYPE_GENRE;
            break;
        default:
            return RCLIB_DB_LIBRARY_DATA_TYPE_NONE;
            break;
    }
    return RCLIB_DB_LIBRARY_DATA_TYPE_NONE;
}

static inline RCLibDbPlaylistDataType rclib_db_library_property_convert(
    RCLibDbQueryDataType query_type)
{
    switch(query_type)
    {
        case RCLIB_DB_QUERY_DATA_TYPE_URI:
            return RCLIB_DB_LIBRARY_DATA_TYPE_URI;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_TITLE:
            return RCLIB_DB_LIBRARY_DATA_TYPE_TITLE;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_ARTIST:
            return RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_ALBUM:
            return RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM;
        case RCLIB_DB_QUERY_DATA_TYPE_FTYPE:
            return RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_LENGTH:
            return RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_TRACKNUM:
            return RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_RATING:
            return RCLIB_DB_LIBRARY_DATA_TYPE_RATING;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_YEAR:
            return RCLIB_DB_LIBRARY_DATA_TYPE_YEAR;
            break;
        case RCLIB_DB_QUERY_DATA_TYPE_GENRE:
            return RCLIB_DB_LIBRARY_DATA_TYPE_GENRE;
            break;
        default:
            return RCLIB_DB_LIBRARY_DATA_TYPE_NONE;
            break;
    }
    return RCLIB_DB_LIBRARY_DATA_TYPE_NONE;
}

/**
 * rclib_db_library_data_query:
 * @library_data: the library data to check
 * @query: the query condition
 * @cancellable: (allow-none): optional #GCancellable object, NULL to ignore
 *
 * Check whether the library data satisfied the query condition.
 * MT safe.
 * 
 * Returns: Whether the library data satisfied the query condition.
 */

gboolean rclib_db_library_data_query(RCLibDbLibraryData *library_data,
    RCLibDbQuery *query, GCancellable *cancellable)
{
    RCLibDbQueryData *query_data;
    gboolean result = FALSE;
    gboolean or_flag = FALSE;
    gboolean ret = TRUE;
    guint i;
    RCLibDbLibraryDataType ltype;
    GType dtype;
    gchar *lstring;
    gint lint;
    gint64 lint64;
    gdouble ldouble;
    gchar *uri;
    if(library_data==NULL || query==NULL)
        return FALSE;
    if(cancellable!=NULL)
        cancellable = g_object_ref(cancellable);
    for(i=0;i<((GPtrArray *)query)->len;i++)
    {
        if(cancellable!=NULL)
        {
            if(g_cancellable_is_cancelled(cancellable))
            {
                g_object_unref(cancellable);
                return FALSE;
            }
        }
        query_data = g_ptr_array_index((GPtrArray *)query, i);
        if(query_data==NULL) continue;
        switch(query_data->type)
        {
            case RCLIB_DB_QUERY_CONDITION_TYPE_SUBQUERY:
            {
                if(query_data->subquery==NULL)
                {
                    result = FALSE;
                    break;
                }
                result = rclib_db_library_data_query(library_data,
                    query_data->subquery, cancellable);
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_EQUALS:
            {
                if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ltype = rclib_db_library_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lstring = NULL;
                        rclib_db_library_data_get(library_data, ltype,
                            &lstring, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(lstring==NULL)
                            lstring = g_strdup("");
                        if(ltype==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                            (lstring==NULL || strlen(lstring)==0))
                        {
                            uri = NULL;
                            if(lstring!=NULL) g_free(lstring);
                            lstring = NULL;
                            rclib_db_library_data_get(library_data,
                                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                lstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), lstring)==0);
                        g_free(lstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)==lint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint64 = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint64, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)==lint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        ldouble = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &ldouble, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (ABS(g_value_get_double(query_data->val) -
                            ldouble)<=10e-5);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_NOT_EQUAL:
            {
               if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ltype = rclib_db_library_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lstring = NULL;
                        rclib_db_library_data_get(library_data, ltype,
                            &lstring, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(lstring==NULL)
                            lstring = g_strdup("");
                        if(ltype==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                            (lstring==NULL || strlen(lstring)==0))
                        {
                            uri = NULL;
                            if(lstring!=NULL) g_free(lstring);
                            lstring = NULL;
                            rclib_db_library_data_get(library_data,
                                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                lstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), lstring)!=0);
                        g_free(lstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)!=lint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint64 = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint64, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)!=lint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        ldouble = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &ldouble, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (ABS(g_value_get_double(query_data->val) -
                            ldouble)>10e-5);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_GREATER:
            {
               if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ltype = rclib_db_library_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lstring = NULL;
                        rclib_db_library_data_get(library_data, ltype,
                            &lstring, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(lstring==NULL)
                            lstring = g_strdup("");
                        if(ltype==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                            (lstring==NULL || strlen(lstring)==0))
                        {
                            uri = NULL;
                            if(lstring!=NULL) g_free(lstring);
                            lstring = NULL;
                            rclib_db_library_data_get(library_data,
                                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                lstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), lstring)>0);
                        g_free(lstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)>lint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint64 = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint64, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)>lint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        ldouble = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &ldouble, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_double(query_data->val) -
                            ldouble>10e-5);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_GREATER_OR_EQUAL:
            {
               if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ltype = rclib_db_library_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lstring = NULL;
                        rclib_db_library_data_get(library_data, ltype,
                            &lstring, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(lstring==NULL)
                            lstring = g_strdup("");
                        if(ltype==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                            (lstring==NULL || strlen(lstring)==0))
                        {
                            uri = NULL;
                            if(lstring!=NULL) g_free(lstring);
                            lstring = NULL;
                            rclib_db_library_data_get(library_data,
                                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                lstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), lstring)>=0);
                        g_free(lstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)>=lint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint64 = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint64, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)>=lint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        ldouble = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &ldouble, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_double(query_data->val) -
                            ldouble>=0);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LESS:
            {
               if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ltype = rclib_db_library_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lstring = NULL;
                        rclib_db_library_data_get(library_data, ltype,
                            &lstring, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(lstring==NULL)
                            lstring = g_strdup("");
                        if(ltype==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                            (lstring==NULL || strlen(lstring)==0))
                        {
                            uri = NULL;
                            if(lstring!=NULL) g_free(lstring);
                            lstring = NULL;
                            rclib_db_library_data_get(library_data,
                                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                lstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), lstring)<0);
                        g_free(lstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)<lint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint64 = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint64, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)<lint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        ldouble = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &ldouble, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_double(query_data->val) -
                            ldouble<-10e-5);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LESS_OR_EQUAL:
            {
               if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ltype = rclib_db_library_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lstring = NULL;
                        rclib_db_library_data_get(library_data, ltype,
                            &lstring, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(lstring==NULL)
                            lstring = g_strdup("");
                        if(ltype==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                            (lstring==NULL || strlen(lstring)==0))
                        {
                            uri = NULL;
                            if(lstring!=NULL) g_free(lstring);
                            lstring = NULL;
                            rclib_db_library_data_get(library_data,
                                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                lstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = (g_strcmp0(g_value_get_string(
                            query_data->val), lstring)<=0);
                        g_free(lstring);
                        break;
                    }
                    case G_TYPE_INT:
                    {
                        if(!G_VALUE_HOLDS_INT(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int(query_data->val)<=lint);
                        break;
                    }
                    case G_TYPE_INT64:
                    {
                        if(!G_VALUE_HOLDS_INT64(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lint64 = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &lint64, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_int64(query_data->val)<=lint64);
                        break;
                    }
                    case G_TYPE_DOUBLE:
                    {
                        if(!G_VALUE_HOLDS_DOUBLE(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        ldouble = 0;
                        rclib_db_library_data_get(library_data, ltype,
                            &ldouble, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        result = (g_value_get_double(query_data->val) -
                            ldouble<=0);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_LIKE:
            {
                if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }

                ltype = rclib_db_library_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lstring = NULL;
                        rclib_db_library_data_get(library_data, ltype,
                            &lstring, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(lstring==NULL)
                            lstring = g_strdup("");
                        if(ltype==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                            (lstring==NULL || strlen(lstring)==0))
                        {
                            uri = NULL;
                            if(lstring!=NULL) g_free(lstring);
                            lstring = NULL;
                            rclib_db_library_data_get(library_data,
                                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                lstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        if(lstring==NULL)
                        {
                            result = FALSE;
                            break;
                        }
                        result = (g_strstr_len(lstring, -1,
                            g_value_get_string(query_data->val))!=NULL);
                        g_free(lstring);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_NOT_LIKE:
            {
                if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ltype = rclib_db_library_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lstring = NULL;
                        rclib_db_library_data_get(library_data, ltype,
                            &lstring, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(lstring==NULL)
                            lstring = g_strdup("");
                        if(ltype==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                            (lstring==NULL || strlen(lstring)==0))
                        {
                            uri = NULL;
                            if(lstring!=NULL) g_free(lstring);
                            lstring = NULL;
                            rclib_db_library_data_get(library_data,
                                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                lstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        if(lstring==NULL)
                        {
                            result = FALSE;
                            break;
                        }
                        result = (g_strstr_len(lstring, -1,
                            g_value_get_string(query_data->val))==NULL);
                        g_free(lstring);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_PREFIX:
            {
                if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ltype = rclib_db_library_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lstring = NULL;
                        rclib_db_library_data_get(library_data, ltype,
                            &lstring, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(lstring==NULL)
                            lstring = g_strdup("");
                        if(ltype==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                            (lstring==NULL || strlen(lstring)==0))
                        {
                            uri = NULL;
                            if(lstring!=NULL) g_free(lstring);
                            lstring = NULL;
                            rclib_db_library_data_get(library_data,
                                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                lstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = g_str_has_prefix(lstring,
                            g_value_get_string(query_data->val));
                        g_free(lstring);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_PROP_SUFFIX:
            {
                if(query_data->val==NULL)
                {
                    result = FALSE;
                    break;
                }
                ltype = rclib_db_library_property_convert(
                    query_data->propid);
                dtype = rclib_db_query_get_query_data_type(
                    query_data->propid);
                switch(dtype)
                {
                    case G_TYPE_STRING:
                    {
                        if(!G_VALUE_HOLDS_STRING(query_data->val))
                        {
                            result = FALSE;
                            break;
                        }
                        lstring = NULL;
                        rclib_db_library_data_get(library_data, ltype,
                            &lstring, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(lstring==NULL)
                            lstring = g_strdup("");
                        if(ltype==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                            (lstring==NULL || strlen(lstring)==0))
                        {
                            uri = NULL;
                            if(lstring!=NULL) g_free(lstring);
                            lstring = NULL;
                            rclib_db_library_data_get(library_data,
                                RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                                RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                            if(uri!=NULL)
                            {
                                lstring = rclib_tag_get_name_from_uri(uri);
                            }
                            g_free(uri);
                        }
                        result = g_str_has_suffix(lstring,
                            g_value_get_string(query_data->val));
                        g_free(lstring);
                        break;
                    }
                    default:
                    {
                        result = FALSE;
                        break;
                    }
                }
                break;
            }
            case RCLIB_DB_QUERY_CONDITION_TYPE_OR:
            {
                or_flag = TRUE;
                break;
            }
            default:
            {
                result = (i==0);
                break;
            }
        }
        if(or_flag)
        {
            if(result) ret = TRUE;
        }
        else
        {
            if(!result) ret = FALSE;
        }
        if(or_flag && query_data->type!=RCLIB_DB_QUERY_CONDITION_TYPE_OR)
        {
            or_flag = FALSE;
        }
    }
    if(cancellable!=NULL)
        g_object_unref(cancellable);
    return ret;
}

/**
 * rclib_db_library_query:
 * @query: he query condition
 * @cancellable: (allow-none): optional #GCancellable object, NULL to ignore
 *
 * Query data from the music library. MT safe.
 * 
 * Returns: (transfer full): The library data array which satisfied
 *     the query condition. Free with #g_ptr_array_free() or
 *     #g_ptr_array_unref() after usage.
 */

GPtrArray *rclib_db_library_query(RCLibDbQuery *query,
    GCancellable *cancellable)
{
    GPtrArray *query_result = NULL;
    GObject *instance;
    RCLibDbPrivate *priv;
    GHashTableIter iter;
    RCLibDbLibraryData *library_data = NULL;
    gchar *uri = NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    if(cancellable!=NULL)
        cancellable = g_object_ref(cancellable);
    query_result = g_ptr_array_new_with_free_func((GDestroyNotify)
        rclib_db_library_data_unref);
    g_rw_lock_reader_lock(&(priv->library_rw_lock));
    g_hash_table_iter_init(&iter, priv->library_table);
    while(g_hash_table_iter_next(&iter, (gpointer *)&uri,
        (gpointer *)&library_data))
    {
        if(cancellable!=NULL)
        {
            if(g_cancellable_is_cancelled(cancellable))
                break;
        }
        if(library_data==NULL) continue;
        if(rclib_db_library_data_query(library_data, query, cancellable))
        {
            g_ptr_array_add(query_result, rclib_db_library_data_ref(
                library_data));
        }
    }
    g_rw_lock_reader_unlock(&(priv->library_rw_lock));
    if(cancellable!=NULL) 
        g_object_unref(cancellable);   
    return query_result;
}

/**
 * rclib_db_library_query_get_uris:
 * @query: he query condition
 * @cancellable: (allow-none): optional #GCancellable object, NULL to ignore
 *
 * Query data from the music library, and get an array of the URIs which
 * points to the music that satisfied the query condition. MT safe.
 * 
 * Returns: (transfer full): The URI array which points to the music that
 *     satisfied the query condition. Free with #g_ptr_array_free() or
 *     #g_ptr_array_unref() after usage.
 */

GPtrArray *rclib_db_library_query_get_uris(RCLibDbQuery *query,
    GCancellable *cancellable)
{
    GPtrArray *query_result = NULL;
    GObject *instance;
    RCLibDbPrivate *priv;
    GHashTableIter iter;
    RCLibDbLibraryData *library_data = NULL;
    gchar *uri = NULL;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    if(cancellable!=NULL)
        cancellable = g_object_ref(cancellable);
    query_result = g_ptr_array_new_with_free_func(g_free);
    g_rw_lock_reader_lock(&(priv->library_rw_lock));
    g_hash_table_iter_init(&iter, priv->library_table);
    while(g_hash_table_iter_next(&iter, (gpointer *)&uri,
        (gpointer *)&library_data))
    {
        if(cancellable!=NULL)
        {
            if(g_cancellable_is_cancelled(cancellable))
                break;
        }
        if(uri==NULL || library_data==NULL) continue;
        if(rclib_db_library_data_query(library_data, query, cancellable))
        {
            g_ptr_array_add(query_result, g_strdup(uri));
        }
    }
    g_rw_lock_reader_unlock(&(priv->library_rw_lock));
    if(cancellable!=NULL)
        g_object_unref(cancellable);    
    return query_result;
}

/**
 * rclib_db_library_get_base_query_result:
 * 
 * Get the built-in base query result object instance of the library.
 * 
 * Returns: (transfer full): The base query result object, unref after usage.
 */

GObject *rclib_db_library_get_base_query_result()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    return g_object_ref(priv->library_query_base);
}

/**
 * rclib_db_library_get_genre_query_result:
 * 
 * Get the built-in genre query result object instance of the library.
 * 
 * Returns: (transfer full): The genre query result object, unref after usage.
 */

GObject *rclib_db_library_get_genre_query_result()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    return g_object_ref(priv->library_query_genre); 
}

/**
 * rclib_db_library_get_artist_query_result:
 * 
 * Get the built-in artist query result object instance of the library.
 * 
 * Returns: (transfer full): The artist query result object, unref after usage.
 */

GObject *rclib_db_library_get_artist_query_result()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    return g_object_ref(priv->library_query_artist); 
}

/**
 * rclib_db_library_get_album_query_result:
 * 
 * Get the built-in album query result object instance of the library.
 * 
 * Returns: (transfer full): The album query result object, unref after usage.
 */

GObject *rclib_db_library_get_album_query_result()
{
    RCLibDbPrivate *priv;
    GObject *instance;
    instance = rclib_db_get_instance();
    if(instance==NULL) return NULL;
    priv = RCLIB_DB(instance)->priv;
    if(priv==NULL) return NULL;
    return g_object_ref(priv->library_query_album); 
}

/**
 * rclib_db_library_query_result_new:
 * @db: the #RCLibDb instance
 * @base: the base instance of the query result, #NULL to create a base
 *     instance 
 * @prop_types: the property type list, end with #RCLIB_DB_QUERY_DATA_TYPE_NONE
 * 
 * Create a new library query result object instance.
 * 
 * Returns: (transfer full): The #RCLibDbLibraryQueryResult instance, #NULL
 *     the operation failed.
 */

GObject *rclib_db_library_query_result_new(RCLibDb *db,
    RCLibDbLibraryQueryResult *base, RCLibDbQueryDataType *prop_types)
{
    GObject *query_result_instance;
    guint i;
    RCLibDbLibraryQueryResultPropItem *prop_item;
    RCLibDbLibraryQueryResultPrivate *priv;
    query_result_instance = g_object_new(RCLIB_TYPE_DB_LIBRARY_QUERY_RESULT,
        NULL);
    if(query_result_instance==NULL)
        return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result_instance)->priv;
    if(priv==NULL)
        return NULL;
    
    priv->query = rclib_db_query_parse(RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
    
    for(i=0;prop_types!=NULL && prop_types[i]!=RCLIB_DB_QUERY_DATA_TYPE_NONE;
        i++)
    {
        prop_item = rclib_db_library_query_result_prop_item_new();
        g_hash_table_insert(priv->prop_table,
            GUINT_TO_POINTER(prop_types[i]), prop_item);
    }
    
    if(base==NULL)
    {
        priv->query_queue = g_async_queue_new_full(g_free);
        priv->query_thread = g_thread_new("RC2-Library-Query-Thread",
            rclib_db_library_query_result_query_thread_cb,
            query_result_instance);
        priv->library_added_id = g_signal_connect(db, "library-added",
            G_CALLBACK(rclib_db_library_query_result_added_cb),
            query_result_instance);
        priv->library_deleted_id = g_signal_connect(db, "library-deleted",
            G_CALLBACK(rclib_db_library_query_result_deleted_cb),
            query_result_instance);
        priv->library_changed_id = g_signal_connect(db, "library-changed",
            G_CALLBACK(rclib_db_library_query_result_changed_cb),
            query_result_instance);
    }
    else
    {
        rclib_db_library_query_result_chain(RCLIB_DB_LIBRARY_QUERY_RESULT(
            query_result_instance), base, TRUE);
    }
        
    return query_result_instance;
}

/**
 * rclib_db_library_query_result_copy_contents:
 * @dst: the destination #RCLibDbLibraryQueryResult instance
 * @src: the source #RCLibDbLibraryQueryResult instance
 * 
 * Deep copy the content data from @src to @dst.
 */

void rclib_db_library_query_result_copy_contents(
    RCLibDbLibraryQueryResult *dst, RCLibDbLibraryQueryResult *src)
{
    RCLibDbLibraryQueryResultPrivate *dst_priv, *src_priv;
    GSequenceIter *dst_iter, *iter;
    RCLibDbLibraryData *library_data;
    RCLibDbLibraryQueryResultPropItem *prop_item;
    RCLibDbLibraryQueryResultPropData *prop_data;
    gchar *prop_string;
    gchar *uri;
    guint prop_type;
    GHashTableIter src_uri_iter, prop_iter;
    if(dst==NULL || src==NULL)
        return;
    dst_priv = dst->priv;
    src_priv = src->priv;
    if(dst_priv==NULL || src_priv==NULL)
        return;

    g_hash_table_iter_init(&src_uri_iter, src_priv->query_uri_table);
    while(g_hash_table_iter_next(&src_uri_iter, (gpointer *)&uri,
        NULL))
    {
        if(uri==NULL) continue;
        library_data = rclib_db_library_get_data(uri);
        if(library_data==NULL) continue;
        if(dst_priv->query!=NULL)
        {
            if(!rclib_db_library_data_query(library_data, dst_priv->query,
                NULL))
            {
                continue;
            }
        }
        if(!g_hash_table_contains(dst_priv->query_uri_table, uri))
        {
            dst_iter = NULL;
            if(dst_priv->list_need_sort && dst_priv->query_sequence!=NULL)
            {
                dst_iter = g_sequence_insert_sorted(dst_priv->query_sequence, 
                    rclib_db_library_data_ref(library_data),
                    rclib_db_library_query_result_item_compare_func, dst_priv);
                g_hash_table_replace(dst_priv->query_iter_table, dst_iter,
                    dst_iter);
            }
            
            g_hash_table_replace(dst_priv->query_uri_table, g_strdup(uri),
                dst_iter);
            g_signal_emit(dst, db_library_query_result_signals[
                SIGNAL_LIBRARY_QUERY_RESULT_ADDED], 0, uri);
        }
        
        g_hash_table_iter_init(&prop_iter, dst_priv->prop_table);
        while(g_hash_table_iter_next(&prop_iter, (gpointer *)&prop_type,
            (gpointer *)&prop_item))
        {
            if(prop_item==NULL) continue;
            if(rclib_db_query_get_query_data_type((RCLibDbQueryDataType)
                prop_type)!=G_TYPE_STRING)
            {
                continue;
            }
            prop_string = NULL;
            rclib_db_library_data_get(library_data,
                rclib_db_library_query_type_to_data_type(prop_type),
                &prop_string, RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
            if(prop_string==NULL)
                prop_string = g_strdup("");
            iter = g_hash_table_lookup(prop_item->prop_name_table,
                prop_string);
            if(iter!=NULL)
            {
                g_hash_table_insert(prop_item->prop_uri_table, g_strdup(uri),
                    iter);
                prop_data = g_sequence_get(iter);
                prop_data->prop_count++;
                prop_item->count++;
                g_signal_emit(dst, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_CHANGED], 0, prop_type,
                    prop_string);
            }
            else
            {
                prop_data = rclib_db_library_query_result_prop_data_new();
                prop_data->prop_count = 1;
                prop_item->count++;
                prop_data->prop_name = g_strdup(prop_string);
                iter = g_sequence_insert_sorted(prop_item->prop_sequence,
                    prop_data,
                    rclib_db_library_query_result_prop_item_compare_func,
                    dst_priv);
                g_hash_table_insert(prop_item->prop_iter_table, iter, iter);
                g_hash_table_insert(prop_item->prop_name_table,
                    g_strdup(prop_string), iter);
                g_hash_table_insert(prop_item->prop_uri_table,
                    g_strdup(uri), iter);
                g_signal_emit(dst, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_PROP_ADDED], 0, prop_type,
                    prop_string);
            }
            g_free(prop_string);
        }
    }
}

/**
 * rclib_db_library_query_result_chain:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @base: the base #RCLibDbLibraryQueryResult instance
 * @import_entries: whether to copy the contents from @base
 * 
 * Chain @query_result to @base,  All changes made to the base instance will
 * be reflected in the child instance.
 */

void rclib_db_library_query_result_chain(
    RCLibDbLibraryQueryResult *query_result, RCLibDbLibraryQueryResult *base,
    gboolean import_entries)
{
    RCLibDbLibraryQueryResultPrivate *priv, *base_priv;
    RCLibDbQuery **query_data;
    if(query_result==NULL || base==NULL) return;
    priv = query_result->priv;
    base_priv = base->priv;
    if(priv==NULL || base_priv==NULL) return;
    if(priv->base_query_result!=NULL) return;
    base = g_object_ref(base);
    base_priv->list_need_sort = FALSE;
    if(base_priv->query_sequence!=NULL)
    {
        g_sequence_free(base_priv->query_sequence);
        base_priv->query_sequence = NULL;
    }
    if(import_entries)
    {
        rclib_db_library_query_result_copy_contents(query_result, base);
    }
    if(priv->library_added_id>0)
    {
        rclib_db_signal_disconnect(priv->library_added_id);
        priv->library_added_id = 0;
    }
    if(priv->library_changed_id>0)
    {
        rclib_db_signal_disconnect(priv->library_changed_id);
        priv->library_changed_id = 0;
    }
    if(priv->library_deleted_id>0)
    {
        rclib_db_signal_disconnect(priv->library_deleted_id);
        priv->library_deleted_id = 0;
    }
    if(priv->query_queue!=NULL)
    {
        query_data = g_new(RCLibDbQuery *, 1);
        *query_data = NULL;
        g_async_queue_push(priv->query_queue, query_data);
    }
    if(priv->query_thread!=NULL)
    {
        g_thread_join(priv->query_thread);
        priv->query_thread = NULL;
    }
    if(priv->query_queue!=NULL)
    {
        g_async_queue_unref(priv->query_queue);
        priv->query_queue = NULL;
    }
    priv->base_added_id = g_signal_connect(base, "query-result-added",
        G_CALLBACK(rclib_db_library_query_result_base_added_cb), query_result);
    priv->base_delete_id = g_signal_connect(base, "query-result-delete",
        G_CALLBACK(rclib_db_library_query_result_base_delete_cb), query_result);
    priv->base_changed_id = g_signal_connect(base, "query-result-changed",
        G_CALLBACK(rclib_db_library_query_result_base_changed_cb),
        query_result);
    priv->base_query_result = base;
}

/**
 * rclib_db_library_query_result_get_base:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * 
 * Get the base query result of current query result, if the query result is
 * chained to another query result, or return #NULL.
 * 
 * Returns: (transfer full): The base query result, #NULL if not exist
 * or any error occurs. Dereference it after Usage.
 */

RCLibDbLibraryQueryResult *rclib_db_library_query_result_get_base(
    RCLibDbLibraryQueryResult *query_result)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return NULL;
    priv = query_result->priv;
    if(priv==NULL || priv->base_query_result==NULL) return NULL;
    return g_object_ref(priv->base_query_result);
}

/**
 * rclib_db_library_query_result_get_data:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @iter: the iter
 * 
 * Get the library data which the @iter points to.
 * 
 * Returns: (transfer full): The library data, #NULL if not found or
 *     any error occurs.
 */

RCLibDbLibraryData *rclib_db_library_query_result_get_data(
    RCLibDbLibraryQueryResult *query_result,
    RCLibDbLibraryQueryResultIter *iter)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryData *library_data = NULL;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL) return NULL;
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->query_iter_table, iter))
            break;
        if(g_sequence_iter_is_end((GSequenceIter *)iter))
            break;
        library_data = g_sequence_get((GSequenceIter *)iter);
        if(library_data==NULL) break;
        if(!rclib_db_library_exist(library_data))
            break;
        library_data = rclib_db_library_data_ref(library_data);
    }
    G_STMT_END;
    return library_data;
}

/**
 * rclib_db_library_query_result_get_iter_by_uri:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @uri: the URI of the query result item
 * 
 * Get the iter of the query result item by the given @uri.
 * 
 * Returns: (transfer none): (skip): The iter, #NULL if the URI does not
 *     exist in the query result.
 */

RCLibDbLibraryQueryResultIter *rclib_db_library_query_result_get_iter_by_uri(
    RCLibDbLibraryQueryResult *query_result, const gchar *uri)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->query_uri_table==NULL) return NULL;
    return g_hash_table_lookup(priv->query_uri_table, uri);
}

/**
 * rclib_db_library_query_result_get_length:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * 
 * Get the query result length number.
 * 
 * Returns: The number of the query result.
 */

guint rclib_db_library_query_result_get_length(
    RCLibDbLibraryQueryResult *query_result)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    guint length = 0;
    if(query_result==NULL) return 0;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->query_sequence==NULL) return 0;
    length = g_sequence_get_length(priv->query_sequence);
    return length;
}

/**
 * rclib_db_library_query_result_get_begin_iter:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * 
 * Get the begin iter of the query result.
 * 
 * Returns: (transfer none): (skip): The begin iter, #NULL if the query
 *     result is empty or any error occurs.
 */

RCLibDbLibraryQueryResultIter *rclib_db_library_query_result_get_begin_iter(
    RCLibDbLibraryQueryResult *query_result)
{
    RCLibDbLibraryQueryResultIter *iter = NULL;
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->query_sequence==NULL) return NULL;
    iter = (RCLibDbLibraryQueryResultIter *)g_sequence_get_begin_iter(
        priv->query_sequence);
    if(g_sequence_iter_is_end((GSequenceIter *)iter))
        iter = NULL;
    return iter;
}
 
/**
 * rclib_db_library_query_result_get_last_iter:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * 
 * Get the last iter of the query result.
 * 
 * Returns: (transfer none): (skip): The last iter, #NULL if the query
 *     result is empty or any error occurs.
 */
    
RCLibDbLibraryQueryResultIter *rclib_db_library_query_result_get_last_iter(
    RCLibDbLibraryQueryResult *query_result)
{
    RCLibDbLibraryQueryResultIter *iter = NULL;
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->query_sequence==NULL) return NULL;
    iter = (RCLibDbLibraryQueryResultIter *)g_sequence_get_end_iter(
        priv->query_sequence);
    iter = (RCLibDbLibraryQueryResultIter *)g_sequence_iter_prev(
        (GSequenceIter *)iter);
    if(g_sequence_iter_is_end((GSequenceIter *)iter))
        iter = NULL;
    return iter;
}

/**
 * rclib_db_library_query_result_get_next_iter:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @iter: the iter
 * 
 * Get the next iter of @iter.
 * 
 * Returns: (transfer none): (skip): The next iter, #NULL if there is no
 *     next one or any error occurs.
 */

RCLibDbLibraryQueryResultIter *rclib_db_library_query_result_get_next_iter(
    RCLibDbLibraryQueryResult *query_result,
    RCLibDbLibraryQueryResultIter *iter)
{
    RCLibDbLibraryQueryResultIter *iter_next = NULL;
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->query_sequence==NULL) return NULL;
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->query_iter_table, iter))
            break;
        iter_next = (RCLibDbLibraryQueryResultIter *)g_sequence_iter_next(
            (GSequenceIter *)iter);
        if(g_sequence_iter_is_end((GSequenceIter *)iter_next))
            iter_next = NULL;
    }
    G_STMT_END;
    return iter_next; 
}

/**
 * rclib_db_library_query_result_get_prev_iter:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @iter: the iter
 * 
 * Get the previous iter of @iter.
 * 
 * Returns: (transfer none): (skip): The previous iter, #NULL if there is no
 *     previous one or any error occurs.
 */

RCLibDbLibraryQueryResultIter *rclib_db_library_query_result_get_prev_iter(
    RCLibDbLibraryQueryResult *query_result,
    RCLibDbLibraryQueryResultIter *iter)
{
    RCLibDbLibraryQueryResultIter *iter_prev = NULL;
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->query_sequence==NULL) return NULL;
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->query_iter_table, iter))
            break;
        if(g_sequence_iter_is_begin((GSequenceIter *)iter))
            iter_prev = NULL;
        else
        {
            iter_prev = (RCLibDbLibraryQueryResultIter *)g_sequence_iter_prev(
                (GSequenceIter *)iter);
        }
    }
    G_STMT_END;
    return iter_prev; 
}

/**
 * rclib_db_library_query_result_get_position:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @iter: the iter
 *
 * Returns the position of @iter.
 *
 * Returns: the position of @iter, -1 if the @iter is not valid.
 */

gint rclib_db_library_query_result_get_position(
    RCLibDbLibraryQueryResult *query_result,
    RCLibDbLibraryQueryResultIter *iter)
{
    gint pos = 0;
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return -1;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->query_sequence==NULL) return -1;
    G_STMT_START
    {
        if(!g_hash_table_contains(priv->query_iter_table, iter))
        {
            pos = -1;
            break;
        }
        pos = g_sequence_iter_get_position((GSequenceIter *)iter);

    }
    G_STMT_END;
    return pos; 
}

/**
 * rclib_db_library_query_result_get_iter_at_pos:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @pos: the position in the query result sequence, or -1 for the end.
 *
 * Return the iterator at position @pos. If @pos is negative or larger
 * than the number of items in the query result sequence, the end iterator
 * is returned.
 *
 * Returns: (transfer none): (skip): The #RCLibDbLibraryQueryResultIter at
 *     position @pos.
 */

RCLibDbLibraryQueryResultIter *rclib_db_library_query_result_get_iter_at_pos(
    RCLibDbLibraryQueryResult *query_result, gint pos)
{
    RCLibDbLibraryQueryResultIter *iter_new = NULL;
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->query_sequence==NULL) return NULL;
    iter_new = (RCLibDbLibraryQueryResultIter *)g_sequence_get_iter_at_pos(
        (GSequence *)priv->query_sequence, pos);
    if(g_sequence_iter_is_end((GSequenceIter *)iter_new))
        return NULL;
    return iter_new; 
}

/**
 * rclib_db_library_query_result_get_random_iter:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 *
 * Return the iterator at arandom position. If there is nothing in the query
 * result, #NULL is returned.
 *
 * Returns: (transfer none): (skip): The #RCLibDbLibraryQueryResultIter at
 *     a random position.
 */

RCLibDbLibraryQueryResultIter *rclib_db_library_query_result_get_random_iter(
    RCLibDbLibraryQueryResult *query_result)
{
    RCLibDbLibraryQueryResultIter *iter_new = NULL;
    RCLibDbLibraryQueryResultPrivate *priv;
    gint length;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->query_sequence==NULL) return NULL;
    length = g_sequence_get_length(priv->query_sequence);
    if(length==0)
        return NULL;
    iter_new = (RCLibDbLibraryQueryResultIter *)g_sequence_get_iter_at_pos(
        (GSequence *)priv->query_sequence, g_random_int_range(0, length));
    if(g_sequence_iter_is_end((GSequenceIter *)iter_new))
        return NULL;
    return iter_new; 
}

/**
 * rclib_db_library_query_result_iter_is_begin:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @iter: the iter
 *
 * Return whether @iter is the begin iterator.
 * Notice that this function will NOT check whether the @iter is valid.
 *
 * Returns: whether @iter is the begin iterator
 */

gboolean rclib_db_library_query_result_iter_is_begin(
    RCLibDbLibraryQueryResult *query_result,
    RCLibDbLibraryQueryResultIter *iter)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL)
        return FALSE;
    if(iter==NULL)
        return TRUE;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL)
        return FALSE;
    return g_sequence_iter_is_begin((GSequenceIter *)iter);
}

/**
 * rclib_db_library_query_result_iter_is_end:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @iter: the iter
 *
 * Return whether @iter is the end iterator.
 * Notice that this function will NOT check whether the @iter is valid.
 *
 * Returns: whether @iter is the end iterator
 */
 
gboolean rclib_db_library_query_result_iter_is_end(
    RCLibDbLibraryQueryResult *query_result,
    RCLibDbLibraryQueryResultIter *iter)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL)
        return FALSE;
    if(iter==NULL)
        return TRUE;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL)
        return FALSE;
    return g_sequence_iter_is_end((GSequenceIter *)iter);
}

/**
 * rclib_db_library_query_result_set_query:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @query: the query to set
 * 
 * Set the query condition with @query.
 */

void rclib_db_library_query_result_set_query(
    RCLibDbLibraryQueryResult *query_result, const RCLibDbQuery *query)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return;
    priv = query_result->priv;
    if(priv==NULL) return;
    if(priv->query!=NULL)
    {
        rclib_db_query_free(priv->query);
    }
    if(query==NULL)
    {
        query = rclib_db_query_parse(RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
    }
    priv->query = rclib_db_query_copy(query);
}

/**
 * rclib_db_library_query_result_query_start:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @clear: whether to clear the query result data
 * 
 * Start query operation.
 */

void rclib_db_library_query_result_query_start(
    RCLibDbLibraryQueryResult *query_result, gboolean clear)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbQuery **query_data = NULL;
    if(query_result==NULL) return;
    priv = query_result->priv;
    if(priv==NULL) return;
    rclib_db_library_query_result_query_clear(query_result);
    if(priv->base_query_result!=NULL)
    {
        rclib_db_library_query_result_copy_contents(query_result,
            priv->base_query_result);
        return;
    }
    if(priv->query_queue==NULL) return;
    query_data = g_new(RCLibDbQuery *, 1);
    if(priv->query!=NULL)
        *query_data = rclib_db_query_copy(priv->query);
    else
        *query_data = rclib_db_query_parse(RCLIB_DB_QUERY_CONDITION_TYPE_NONE);
    g_async_queue_push(priv->query_queue, query_data);
}

/**
 * rclib_db_library_query_result_query_cancel:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * 
 * Cancel the current query process.
 */

void rclib_db_library_query_result_query_cancel(
    RCLibDbLibraryQueryResult *query_result)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL) return;
    if(priv->base_query_result!=NULL)
    {
        rclib_db_library_query_result_query_cancel(priv->base_query_result);
        return;
    }
    if(priv->cancellable==NULL)
        return;
    g_cancellable_cancel(priv->cancellable);
}

/**
 * rclib_db_library_query_result_query_clear:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * 
 * Clear the query result.
 */
 
void rclib_db_library_query_result_query_clear(
    RCLibDbLibraryQueryResult *query_result)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    GSequenceIter *iter;
    GHashTableIter hash_iter, prop_iter;
    gchar *uri, *prop_name;
    guint prop_type;
    RCLibDbLibraryQueryResultPropItem *prop_item;
    if(query_result==NULL) return;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL)
    {
        return;
    }
    
    if(priv->query_uri_table!=NULL)
    {
        g_hash_table_iter_init(&hash_iter, priv->query_uri_table);
        while(g_hash_table_iter_next(&hash_iter, (gpointer *)&uri,
            (gpointer *)&iter))
        {
            if(uri!=NULL)
            {
                g_signal_emit(query_result, db_library_query_result_signals[
                    SIGNAL_LIBRARY_QUERY_RESULT_DELETE], 0, uri);
            }
            if(iter!=NULL)
            {
                g_sequence_remove(iter);
            }
        }
    }

    if(priv->query_iter_table!=NULL)
        g_hash_table_remove_all(priv->query_iter_table);
    if(priv->query_uri_table)
        g_hash_table_remove_all(priv->query_uri_table);
    
    if(priv->prop_table!=NULL)
    {
        g_hash_table_iter_init(&hash_iter, priv->prop_table);
        while(g_hash_table_iter_next(&hash_iter, (gpointer *)&prop_type,
            (gpointer *)&prop_item))
        {
            if(prop_item==NULL) continue;
            if(prop_item->prop_name_table!=NULL)
            {
                g_hash_table_iter_init(&prop_iter, prop_item->prop_name_table);
                while(g_hash_table_iter_next(&prop_iter, (gpointer *)&prop_name,
                    (gpointer *)&iter))
                {
                    if(prop_name==NULL) continue;
                    g_signal_emit(query_result,
                        db_library_query_result_signals[
                        SIGNAL_LIBRARY_QUERY_RESULT_PROP_DELETE], 0, prop_type, 
                        prop_name);
                    if(iter!=NULL)
                        g_sequence_remove(iter);
                }
            }

            if(prop_item->prop_iter_table!=NULL)
                g_hash_table_remove_all(prop_item->prop_iter_table);
            if(prop_item->prop_name_table)
                g_hash_table_remove_all(prop_item->prop_name_table);
            if(prop_item->prop_uri_table)
                g_hash_table_remove_all(prop_item->prop_uri_table);
            prop_item->count = 0;
        }
    }
}

/**
 * rclib_db_library_query_result_get_query:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * 
 * Get the current query applied to the query result.
 * 
 * Returns: (transfer none): The current query appiled to the query result.
 */

const RCLibDbQuery *rclib_db_library_query_result_get_query(
    RCLibDbLibraryQueryResult *query_result)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL)
        return NULL;
    return priv->query;
}

/**
 * rclib_db_library_query_result_sort:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @column: the column to sort
 * @direction: the sort direction, #FALSE to use ascending, #TRUE to use
 *     descending
 * 
 * Sort the query result items by the alphabets order (ascending or descending)
 * of the given @column.
 */

void rclib_db_library_query_result_sort(
    RCLibDbLibraryQueryResult *query_result, RCLibDbLibraryDataType column,
    gboolean direction)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    gint i;
    gint *order = NULL;
    GHashTable *new_positions, *sort_table;
    GSequenceIter *ptr;
    gint length;
    GType column_type = G_TYPE_NONE;
    if(query_result==NULL) return;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    GVariant *variant;
    RCLibDbLibraryData *library_data;
    if(priv==NULL) return;
    switch(column)
    {
        case RCLIB_DB_LIBRARY_DATA_TYPE_TITLE:
        case RCLIB_DB_LIBRARY_DATA_TYPE_ARTIST:
        case RCLIB_DB_LIBRARY_DATA_TYPE_ALBUM:
        case RCLIB_DB_LIBRARY_DATA_TYPE_FTYPE:
        case RCLIB_DB_LIBRARY_DATA_TYPE_GENRE:
        {
            column_type = G_TYPE_STRING;
            break;
        }
        case RCLIB_DB_LIBRARY_DATA_TYPE_LENGTH:
        {
            column_type = G_TYPE_INT64;
            break;
        }
        case RCLIB_DB_LIBRARY_DATA_TYPE_TRACKNUM:
        case RCLIB_DB_LIBRARY_DATA_TYPE_YEAR:
        {
            column_type = G_TYPE_INT;
            break;
        }
        case RCLIB_DB_LIBRARY_DATA_TYPE_RATING:
        {
            column_type = G_TYPE_FLOAT;
            break;
        }
        default:
        {
            g_warning("Error column type: %u", column);
            return;
            break;
        }
    }
    sort_table = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
        (GDestroyNotify)g_variant_unref);
    new_positions = g_hash_table_new(g_direct_hash, g_direct_equal);
    for(i=0, ptr=g_sequence_get_begin_iter(priv->query_sequence);
        ptr!=NULL && !g_sequence_iter_is_end(ptr);
        ptr=g_sequence_iter_next(ptr))
    {
        variant = NULL;
        library_data = g_sequence_get(ptr);
        switch(column_type)
        {
            case G_TYPE_STRING:
            {
                gchar *vstr = NULL;
                gchar *cstr = NULL;
                gchar *uri = NULL;
                if(library_data!=NULL)
                {
                    rclib_db_library_data_get(library_data, column, &vstr,
                        RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                    if(column==RCLIB_DB_LIBRARY_DATA_TYPE_TITLE &&
                        (vstr==NULL || strlen(vstr)==0))
                    {
                        rclib_db_library_data_get(library_data,
                            RCLIB_DB_LIBRARY_DATA_TYPE_URI, &uri,
                            RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                        if(uri!=NULL)
                            vstr = rclib_tag_get_name_from_uri(uri);
                        g_free(uri);
                    }
                }                
                if(vstr==NULL) vstr = g_strdup("");
                cstr = g_utf8_casefold(vstr, -1);
                variant = g_variant_new_string(cstr);
                g_free(cstr);
                g_free(vstr);
                break;
            }
            case G_TYPE_INT:
            {
                gint vint = 0;
                if(library_data!=NULL)
                {
                    rclib_db_library_data_get(library_data, column, &vint,
                        RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                }
                variant = g_variant_new_int32(vint);
                break;
            }
            case G_TYPE_INT64:
            {
                gint64 vint64 = 0;
                if(library_data!=NULL)
                {
                    rclib_db_library_data_get(library_data, column, &vint64,
                        RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                }
                variant = g_variant_new_int64(vint64);
                break;
            }
            case G_TYPE_FLOAT:
            {
                gfloat vfloat = 0;
                if(library_data!=NULL)
                {
                    rclib_db_library_data_get(library_data, column, &vfloat,
                        RCLIB_DB_LIBRARY_DATA_TYPE_NONE);
                }
                variant = g_variant_new_double(vfloat);
                break;
            }
            default:
            {
                variant = g_variant_new_boolean(FALSE);
                g_warning("Wrong column data type, this should not happen!");
                break;
            }
        }
        g_hash_table_insert(new_positions, ptr, GINT_TO_POINTER(i));
        g_hash_table_insert(sort_table, ptr, g_variant_take_ref(variant));
        i++;
    }
    if(direction)
    {
        g_sequence_sort_iter(priv->query_sequence,
            rclib_db_variant_sort_dsc_func, sort_table);
    }
    else
    {
        g_sequence_sort_iter(priv->query_sequence,
            rclib_db_variant_sort_asc_func, sort_table);
    }
    g_hash_table_destroy(sort_table);
    length = g_hash_table_size(new_positions);
    order = g_new(gint, length);
    for(i=0, ptr=g_sequence_get_begin_iter(priv->query_sequence);
        ptr!=NULL && !g_sequence_iter_is_end(ptr);
        ptr=g_sequence_iter_next(ptr))
    {
        order[i] = GPOINTER_TO_INT(g_hash_table_lookup(new_positions,
            ptr));
        i++;
    }
    g_hash_table_destroy(new_positions);
    g_signal_emit(query_result, db_library_query_result_signals[
        SIGNAL_LIBRARY_QUERY_RESULT_REORDERED], 0, order);
    g_free(order);
    priv->sort_column = column;
    priv->sort_direction = direction;
}

/**
 * rclib_db_library_query_result_prop_get_data:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * @iter: the iter pointed to the property
 * @prop_name: (out): the property name
 * @prop_count: (out): the number of the property name used
 * 
 * Get the data of the property in the query result.
 * 
 * Returns: Whether the property data exists.
 */

gboolean rclib_db_library_query_result_prop_get_data(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type,
    RCLibDbLibraryQueryResultPropIter *iter, gchar **prop_name,
    guint *prop_count)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    RCLibDbLibraryQueryResultPropData *data;
    if(query_result==NULL) return FALSE;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->prop_table==NULL)
        return FALSE;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL)
        return FALSE;
    if(!g_hash_table_contains(item->prop_iter_table, iter))
        return FALSE;
    data = g_sequence_get((GSequenceIter *)iter);
    if(data==NULL)
        return FALSE;
    if(prop_name!=NULL)
        *prop_name = g_strdup(data->prop_name);
    if(prop_count!=NULL)
        *prop_count = data->prop_count;
    return TRUE;
}

/**
 * rclib_db_library_query_result_prop_get_total_count:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * @count: (out): the total count of music in this property type
 * 
 * Get the total count of music in the given property type in the query result.
 * 
 * Returns: Whether the property type exists.
 */

gboolean rclib_db_library_query_result_prop_get_total_count(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type,
    guint *count)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    if(query_result==NULL) return FALSE;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->prop_table==NULL)
        return FALSE;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL)
        return FALSE;
    if(count!=NULL)
        *count = item->count;
    return TRUE;
}

/**
 * rclib_db_library_query_result_prop_get_iter_by_prop:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * @prop_text: the property text
 * 
 * Get the iter pointed to the given @prop_text of the given property type
 * in the the query result.
 * 
 * Returns: (transfer none): (skip): The iter, #NULL if the property text
 *     does not exist or any error occurs.
 */

RCLibDbLibraryQueryResultPropIter *
    rclib_db_library_query_result_prop_get_iter_by_prop(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type,
    const gchar *prop_text)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->prop_table==NULL)
        return NULL;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL)
        return NULL;
    return g_hash_table_lookup(item->prop_name_table, prop_text);
}

/**
 * rclib_db_library_query_result_prop_get_length:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * 
 * Get length of the given property type in the the query result.
 * 
 * Returns: The length of the given property type.
 */

guint rclib_db_library_query_result_prop_get_length(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    if(query_result==NULL) return 0;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->prop_table==NULL)
        return 0;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL)
        return 0;
    return g_sequence_get_length(item->prop_sequence);
}

/**
 * rclib_db_library_query_result_prop_get_begin_iter:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * 
 * Get the begin iter of the given property type in the the query result.
 * 
 * Returns: (transfer none): (skip): The begin iter, #NULL if the query
 *     result is empty or any error occurs.
 */

RCLibDbLibraryQueryResultPropIter *
    rclib_db_library_query_result_prop_get_begin_iter(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    RCLibDbLibraryQueryResultPropIter *iter;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->prop_table==NULL)
        return NULL;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL)
        return NULL;
    iter = (RCLibDbLibraryQueryResultPropIter *)g_sequence_get_begin_iter(
        item->prop_sequence);
    if(g_sequence_iter_is_end((GSequenceIter *)iter))
        iter = NULL;
    return iter;
}

/**
 * rclib_db_library_query_result_prop_get_last_iter:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * 
 * Get the last iter of the given property type in the the query result.
 * 
 * Returns: (transfer none): (skip): The last iter, #NULL if the query
 *     result is empty or any error occurs.
 */
 
RCLibDbLibraryQueryResultPropIter *
    rclib_db_library_query_result_prop_get_last_iter(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    RCLibDbLibraryQueryResultPropIter *iter;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->prop_table==NULL)
        return NULL;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL)
        return NULL;
    iter = (RCLibDbLibraryQueryResultPropIter *)g_sequence_get_end_iter(
        item->prop_sequence);
    iter = (RCLibDbLibraryQueryResultPropIter *)g_sequence_iter_prev(
        (GSequenceIter *)iter);
    if(g_sequence_iter_is_end((GSequenceIter *)iter))
        iter = NULL;
    return iter;
}

/**
 * rclib_db_library_query_result_prop_get_next_iter:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * @iter: the iter pointed to the property
 * 
 * Get the next iter of @iter.
 * 
 * Returns: (transfer none): (skip): The next iter, #NULL if there is no
 *     next one or any error occurs.
 */

RCLibDbLibraryQueryResultPropIter *
    rclib_db_library_query_result_prop_get_next_iter(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type,
    RCLibDbLibraryQueryResultPropIter *iter)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    RCLibDbLibraryQueryResultPropIter *iter_next = NULL;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->prop_table==NULL)
        return NULL;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL)
        return NULL;
    if(!g_hash_table_contains(item->prop_iter_table, iter))
        return NULL;
    iter_next = (RCLibDbLibraryQueryResultPropIter *)g_sequence_iter_next(
        (GSequenceIter *)iter);
    if(g_sequence_iter_is_end((GSequenceIter *)iter_next))
        iter_next = NULL;
    return iter_next;
}

/**
 * rclib_db_library_query_result_prop_get_prev_iter:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * @iter: the iter pointed to the property
 * 
 * Get the previous iter of @iter.
 * 
 * Returns: (transfer none): (skip): The previous iter, #NULL if there is no
 *     next one or any error occurs.
 */

RCLibDbLibraryQueryResultPropIter *
    rclib_db_library_query_result_prop_get_prev_iter(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type,
    RCLibDbLibraryQueryResultPropIter *iter)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    RCLibDbLibraryQueryResultPropIter *iter_prev = NULL;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->prop_table==NULL)
        return NULL;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL)
        return NULL;
    if(!g_hash_table_contains(item->prop_iter_table, iter))
        return NULL;
    if(g_sequence_iter_is_begin((GSequenceIter *)iter))
        iter_prev = NULL;
    else
    {
        iter_prev = (RCLibDbLibraryQueryResultPropIter *)g_sequence_iter_prev(
            (GSequenceIter *)iter);
    }
    return iter_prev;
}

/**
 * rclib_db_library_query_result_prop_get_position:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * @iter: the iter pointed to the property
 * 
 * Returns the position of @iter.
 *
 * Returns: the position of @iter, -1 if the @iter is not valid.
 */

gint rclib_db_library_query_result_prop_get_position(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type,
    RCLibDbLibraryQueryResultPropIter *iter)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    gint pos = 0;
    if(query_result==NULL) return -1;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->prop_table==NULL)
        return -1;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL)
        return -1;
    if(!g_hash_table_contains(item->prop_iter_table, iter))
        return -1;
    pos = g_sequence_iter_get_position((GSequenceIter *)iter);
    return pos;  
}

/**
 * rclib_db_library_query_result_prop_get_iter_at_pos:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * @pos: the position in the property sequence, or -1 for the end.
 * 
 * Return the iterator at position @pos. If @pos is negative or larger
 * than the number of items in the property sequence, the end iterator
 * is returned.
 *
 * Returns: (transfer none): (skip): The #RCLibDbLibraryQueryResultPropIter at
 *     position @pos.
 */

RCLibDbLibraryQueryResultPropIter *
    rclib_db_library_query_result_prop_get_iter_at_pos(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type,
    gint pos)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    RCLibDbLibraryQueryResultPropIter *iter = NULL;
    if(query_result==NULL) return NULL;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL || priv->prop_table==NULL)
        return NULL;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL)
        return NULL;
    iter = (RCLibDbLibraryQueryResultPropIter *)g_sequence_get_iter_at_pos(
        (GSequence *)item->prop_sequence, pos);
    if(g_sequence_iter_is_end((GSequenceIter *)iter))
        return NULL;
    return iter;
}

/**
 * rclib_db_library_query_result_prop_iter_is_begin:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * @iter: the iter
 *
 * Return whether @iter is the begin iterator.
 * Notice that this function will NOT check whether the @iter is valid.
 *
 * Returns: whether @iter is the begin iterator.
 */

gboolean rclib_db_library_query_result_prop_iter_is_begin(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type,
    RCLibDbLibraryQueryResultPropIter *iter)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL)
        return FALSE;
    if(iter==NULL)
        return TRUE;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL)
        return FALSE;
    return g_sequence_iter_is_begin((GSequenceIter *)iter);
}
    
/**
 * rclib_db_library_query_result_prop_iter_is_end:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * @iter: the iter
 *
 * Return whether @iter is the end iterator.
 * Notice that this function will NOT check whether the @iter is valid.
 *
 * Returns: whether @iter is the end iterator.
 */
    
gboolean rclib_db_library_query_result_prop_iter_is_end(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type,
    RCLibDbLibraryQueryResultPropIter *iter)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    if(query_result==NULL)
        return FALSE;
    if(iter==NULL)
        return TRUE;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    if(priv==NULL)
        return FALSE;
    return g_sequence_iter_is_end((GSequenceIter *)iter);
}

/**
 * rclib_db_library_query_result_prop_sort:
 * @query_result: the #RCLibDbLibraryQueryResult instance
 * @prop_type: the property type
 * @direction: the sort direction, #FALSE to use ascending, #TRUE to use
 *     descending
 * 
 * Sort the query result property items by the alphabets order 
 * (ascending or descending) of the given @column.
 */

void rclib_db_library_query_result_prop_sort(
    RCLibDbLibraryQueryResult *query_result, RCLibDbQueryDataType prop_type,
    gboolean direction)
{
    RCLibDbLibraryQueryResultPrivate *priv;
    RCLibDbLibraryQueryResultPropItem *item;
    RCLibDbLibraryQueryResultPropData *prop_data;
    gint i;
    gint *order = NULL;
    GHashTable *new_positions, *sort_table;
    GSequenceIter *ptr;
    gint length;
    if(query_result==NULL) return;
    priv = RCLIB_DB_LIBRARY_QUERY_RESULT(query_result)->priv;
    GVariant *variant;
    if(priv==NULL) return;
    item = g_hash_table_lookup(priv->prop_table, GUINT_TO_POINTER(prop_type));
    if(item==NULL) return;
    sort_table = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
        (GDestroyNotify)g_variant_unref);
    new_positions = g_hash_table_new(g_direct_hash, g_direct_equal);
    for(i=0, ptr=g_sequence_get_begin_iter(item->prop_sequence);
        ptr!=NULL && !g_sequence_iter_is_end(ptr);
        ptr=g_sequence_iter_next(ptr))
    {
        variant = NULL;
        prop_data = g_sequence_get(ptr);
        if(prop_data!=NULL && prop_data->prop_name!=NULL)
        {
            variant = g_variant_new_string(prop_data->prop_name);
        }
        else
        {
            variant = g_variant_new_string("");
        }
        g_hash_table_insert(new_positions, ptr, GINT_TO_POINTER(i));
        g_hash_table_insert(sort_table, ptr, g_variant_take_ref(variant));
        i++;
    }
    if(direction)
    {
        g_sequence_sort_iter(item->prop_sequence,
            rclib_db_variant_sort_dsc_func, sort_table);
    }
    else
    {
        g_sequence_sort_iter(item->prop_sequence,
            rclib_db_variant_sort_asc_func, sort_table);
    }
    g_hash_table_destroy(sort_table);
    length = g_hash_table_size(new_positions);
    order = g_new(gint, length);
    for(i=0, ptr=g_sequence_get_begin_iter(item->prop_sequence);
        ptr!=NULL && !g_sequence_iter_is_end(ptr);
        ptr=g_sequence_iter_next(ptr))
    {
        order[i] = GPOINTER_TO_INT(g_hash_table_lookup(new_positions,
            ptr));
        i++;
    }
    g_hash_table_destroy(new_positions);
    g_signal_emit(query_result, db_library_query_result_signals[
        SIGNAL_LIBRARY_QUERY_RESULT_PROP_REORDERED], 0, prop_type, order);
    g_free(order);
    item->sort_direction = direction;
}
