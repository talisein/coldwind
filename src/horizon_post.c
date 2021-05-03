#include "config.h"
#include "horizon_post.h"

typedef struct
{
    gint64    post_number;
    gint64    resto;
    gint      sticky;
    gint      closed;
    gint64    time;
    gchar    *now;
    gchar    *name;
    gchar    *trip;
    gchar    *id;
    gchar    *capcode;
    gchar    *country_code;
    gchar    *country_name;
    gchar    *email;
    gchar    *subject;
    gchar    *comment;
    gint64    renamed_filename;
    gchar    *filename;
    gchar    *ext;
    gchar    *md5;
    gint64    fsize;
    gint      image_width;
    gint      image_height;
    gint      thumbnail_width;
    gint      thumbnail_height;
    gint      is_file_deleted;
    gint      is_spoiler;
    gint      custom_spoiler;
    gint      replies;
    gint      images;
    gint      bumplimit;
    gint      imagelimit;
    gchar    *image_url;
    gchar    *thumb_url;
    gchar    *board;
    gboolean  rendered;
    gint64    thread_id;
} HorizonPostPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(HorizonPost, horizon_post, G_TYPE_OBJECT);

enum
{
  PROP_0,

  PROP_POST_NUMBER,
  PROP_RESTO,
  PROP_STICKY,
  PROP_CLOSED,
  PROP_NOW,
  PROP_TIME,
  PROP_NAME,
  PROP_TRIP,
  PROP_ID,
  PROP_CAPCODE,
  PROP_COUNTRY,
  PROP_COUNTRY_NAME,
  PROP_EMAIL,
  PROP_SUBJECT,
  PROP_COMMENT,
  PROP_RENAMED_FILENAME,
  PROP_FILENAME,
  PROP_EXT,
  PROP_FSIZE,
  PROP_MD5,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_THUMBNAIL_WIDTH,
  PROP_THUMBNAIL_HEIGHT,
  PROP_FILEDELETED,
  PROP_SPOILER,
  PROP_CUSTOM_SPOILER,
  PROP_REPLIES,
  PROP_IMAGES,
  PROP_BUMPLIMIT,
  PROP_IMAGELIMIT,
  PROP_IMAGE_URL,
  PROP_THUMB_URL,
  PROP_BOARD,
  PROP_RENDERED,
  PROP_THREAD_ID,

  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
horizon_post_set_property(GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    HorizonPost *self = HORIZON_POST(object);
    HorizonPostPrivate *priv = horizon_post_get_instance_private(self);

    switch (property_id)
        {
        case PROP_POST_NUMBER:
            priv->post_number = g_value_get_int64(value);
            break;
        case PROP_RESTO:
            priv->resto = g_value_get_int64(value);
            break;
        case PROP_STICKY:
            priv->sticky   = g_value_get_int(value);
            break;
        case PROP_CLOSED:
            priv->closed  = g_value_get_int(value);
            break;
        case PROP_NOW:
            g_free(priv->now);
            priv->now  = g_value_dup_string(value);
            break;
        case PROP_TIME:
            priv->time  = g_value_get_int64(value);
            break;
        case PROP_NAME:
            g_free(priv->name);
            priv->name  = g_value_dup_string(value);
            break;
        case PROP_TRIP:
            g_free(priv->trip);
            priv->trip  = g_value_dup_string(value);
            break;
        case PROP_ID:
            g_free(priv->id);
            priv->id  = g_value_dup_string(value);
            break;
        case PROP_CAPCODE:
            g_free(priv->capcode);
            priv->capcode  = g_value_dup_string(value);
            break;
        case PROP_COUNTRY:
            g_free(priv->country_code);
            priv->country_code  = g_value_dup_string(value);
            break;
        case PROP_COUNTRY_NAME:
            g_free(priv->country_name);
            priv->country_name  = g_value_dup_string(value);
            break;
        case PROP_EMAIL:
            g_free(priv->email);
            priv->email  = g_value_dup_string(value);
            break;
        case PROP_SUBJECT:
            g_free(priv->subject);
            priv->subject  = g_value_dup_string(value);
            break;
        case PROP_COMMENT:
            g_free(priv->comment);
            priv->comment  = g_value_dup_string(value);
            break;
        case PROP_RENAMED_FILENAME:
            priv->renamed_filename  = g_value_get_int64(value);
            break;
        case PROP_FILENAME:
            g_free(priv->filename);
            priv->filename  = g_value_dup_string(value);
            break;
        case PROP_EXT:
            g_free(priv->ext);
            priv->ext  = g_value_dup_string(value);
            break;
        case PROP_FSIZE:
            priv->fsize  = g_value_get_int64(value);
            break;
        case PROP_MD5:
            g_free(priv->md5);
            priv->md5  = g_value_dup_string(value);
            break;
        case PROP_WIDTH:
            priv->image_width  = g_value_get_int(value);
            break;
        case PROP_HEIGHT:
            priv->image_height  = g_value_get_int(value);
            break;
        case PROP_THUMBNAIL_WIDTH:
            priv->thumbnail_width  = g_value_get_int(value);
            break;
        case PROP_THUMBNAIL_HEIGHT:
            priv->thumbnail_height  = g_value_get_int(value);
            break;
        case PROP_FILEDELETED:
            priv->is_file_deleted  = g_value_get_int(value);
            break;
        case PROP_SPOILER:
            priv->is_spoiler  = g_value_get_int(value);
            break;
        case PROP_CUSTOM_SPOILER:
            priv->custom_spoiler  = g_value_get_int(value);
            break;
        case PROP_REPLIES:
            priv->replies = g_value_get_int(value);
            break;
        case PROP_IMAGES:
            priv->images = g_value_get_int(value);
            break;
        case PROP_BUMPLIMIT:
            priv->bumplimit = g_value_get_int(value);
            break;
        case PROP_IMAGELIMIT:
            priv->imagelimit = g_value_get_int(value);
            break;
        case PROP_IMAGE_URL:
            g_free(priv->image_url);
            priv->image_url = g_value_dup_string(value);
            break;
        case PROP_THUMB_URL:
            g_free(priv->thumb_url);
            priv->thumb_url = g_value_dup_string(value);
            break;
        case PROP_BOARD:
            g_free(priv->board);
            priv->board = g_value_dup_string(value);
            break;
        case PROP_RENDERED:
            priv->rendered = g_value_get_boolean(value);
            break;
        case PROP_THREAD_ID:
            priv->thread_id = g_value_get_int64(value);
            break;
        default:
            /* We don't have any other property... */
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
        }
}

static void
horizon_post_get_property(GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    HorizonPost *self = HORIZON_POST(object);
    HorizonPostPrivate *priv = horizon_post_get_instance_private(self);

    switch (property_id)
        {
        case PROP_POST_NUMBER:
            g_value_set_int64(value, priv->post_number);
            break;
        case PROP_RESTO:
            g_value_set_int64(value, priv->resto);
            break;
        case PROP_STICKY:
            g_value_set_int(value, priv->sticky);
            break;
        case PROP_CLOSED:
            g_value_set_int(value, priv->closed);
            break;
        case PROP_NOW:
            g_value_set_string(value, priv->now);
            break;
        case PROP_TIME:
            g_value_set_int64(value, priv->time);
            break;
        case PROP_NAME:
            g_value_set_string(value, priv->name);
            break;
        case PROP_TRIP:
            g_value_set_string(value, priv->trip);
            break;
        case PROP_ID:
            g_value_set_string(value, priv->id);
            break;
        case PROP_CAPCODE:
            g_value_set_string(value, priv->capcode);
            break;
        case PROP_COUNTRY:
            g_value_set_string(value, priv->country_code);
            break;
        case PROP_COUNTRY_NAME:
            g_value_set_string(value, priv->country_name);
            break;
        case PROP_EMAIL:
            g_value_set_string(value, priv->email);
            break;
        case PROP_SUBJECT:
            g_value_set_string(value, priv->subject);
            break;
        case PROP_COMMENT:
            g_value_set_string(value, priv->comment);
            break;
        case PROP_RENAMED_FILENAME:
            g_value_set_int64(value, priv->renamed_filename);
            break;
        case PROP_FILENAME:
            g_value_set_string(value, priv->filename);
            break;
        case PROP_EXT:
            g_value_set_string(value, priv->ext);
            break;
        case PROP_FSIZE:
            g_value_set_int64(value, priv->fsize);
            break;
        case PROP_MD5:
            g_value_set_string(value, priv->md5);
            break;
        case PROP_WIDTH:
            g_value_set_int(value, priv->image_width);
            break;
        case PROP_HEIGHT:
            g_value_set_int(value, priv->image_height);
            break;
        case PROP_THUMBNAIL_WIDTH:
            g_value_set_int(value, priv->thumbnail_width);
            break;
        case PROP_THUMBNAIL_HEIGHT:
            g_value_set_int(value, priv->thumbnail_height);
            break;
        case PROP_FILEDELETED:
            g_value_set_int(value, priv->is_file_deleted);
            break;
        case PROP_SPOILER:
            g_value_set_int(value, priv->is_spoiler);
            break;
        case PROP_CUSTOM_SPOILER:
            g_value_set_int(value, priv->custom_spoiler);
            break;
        case PROP_REPLIES:
            g_value_set_int(value, priv->replies);
            break;
        case PROP_IMAGES:
            g_value_set_int(value, priv->images);
            break;
        case PROP_BUMPLIMIT:
            g_value_set_int(value, priv->bumplimit);
            break;
        case PROP_IMAGELIMIT:
            g_value_set_int(value, priv->imagelimit);
            break;
        case PROP_IMAGE_URL:
            g_value_set_string(value, horizon_post_get_image_url(self));
            break;
        case PROP_THUMB_URL:
            g_value_set_string(value, horizon_post_get_thumb_url(self));
            break;
        case PROP_BOARD:
            g_value_set_string(value, priv->board);
            break;
        case PROP_RENDERED:
            g_value_set_boolean(value, priv->rendered);
            break;
        case PROP_THREAD_ID:
            g_value_set_int64(value, priv->thread_id);
            break;
        default:
            /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
    }
}

static void
horizon_post_init(HorizonPost *self)
{
}

static void
horizon_post_dispose(GObject *gobject)
{
  /*
   * In dispose, you are supposed to free all types referenced from this
   * object which might themselves hold a reference to self. Generally,
   * the most simple solution is to unref all members on which you own a
   * reference.
   */

  /* dispose might be called multiple times, so we must guard against
   * calling g_object_unref() on an invalid GObject.
   */
  /*
  if(self->an_object)
    {
      g_object_unref(self->an_object);

      self->an_object = NULL;
    }
  */
  /* Chain up to the parent class */
  G_OBJECT_CLASS(horizon_post_parent_class)->dispose(gobject);
}

static void
horizon_post_finalize(GObject *gobject)
{
    HorizonPost *self = HORIZON_POST(gobject);
    HorizonPostPrivate *priv = horizon_post_get_instance_private(self);

    g_free(priv->now);
    g_free(priv->name);
    g_free(priv->trip);
    g_free(priv->id);
    g_free(priv->capcode);
    g_free(priv->country_code);
    g_free(priv->country_name);
    g_free(priv->email);
    g_free(priv->subject);
    g_free(priv->comment);
    g_free(priv->filename);
    g_free(priv->ext);
    g_free(priv->md5);
    g_free(priv->thumb_url);
    g_free(priv->image_url);
    g_free(priv->board);

    /* Chain up to the parent class */
    G_OBJECT_CLASS(horizon_post_parent_class)->finalize(gobject);
}

static void
horizon_post_class_init(HorizonPostClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = horizon_post_set_property;
    gobject_class->get_property = horizon_post_get_property;
    gobject_class->dispose      = horizon_post_dispose;
    gobject_class->finalize     = horizon_post_finalize;

    obj_properties[PROP_POST_NUMBER] =
        g_param_spec_int64("no", /* name */
                           "post number", /* nick */
                           "Post's unique identifier", /* blurb */
                           1  /* minimum value */,
                           9999999999999 /* maximum value */,
                           1  /* default value */,
                           G_PARAM_READWRITE);

    obj_properties[PROP_RESTO] =
        g_param_spec_int64("resto",
                           "Reply to",
                           "Post is in reply to this post id",
                           0  /* minimum value */,
                           9999999999999 /* maximum value */,
                           0  /* default value */,
                           G_PARAM_READWRITE);
    obj_properties[PROP_STICKY] =
        g_param_spec_int("sticky",
                         "is sticky",
                         "",
                         0,
                         1,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_CLOSED] =
        g_param_spec_int("closed",
                         "thread is closed",
                         "",
                         0,
                         1,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_NOW] =
        g_param_spec_string("now",
                            "Date and time",
                            "",
                            "no-now-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_TIME] =
        g_param_spec_int64("time",
                           "UNIX timestamp",
                           "",
                           0,
                           G_MAXINT64,
                           0, /* default value */
                           G_PARAM_READWRITE);
    obj_properties[PROP_NAME] =
        g_param_spec_string("name",
                            "Name",
                            "",
                            "no-name-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_TRIP] =
        g_param_spec_string("trip",
                            "Tripcode",
                            "",
                            "no-trip-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_ID] =
        g_param_spec_string("id",
                            "ID",
                            "Identifies if Mod or Admin",
                            "no-id-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_CAPCODE] =
        g_param_spec_string("capcode",
                            "Capcode",
                            "",
                            "no-capcode-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_COUNTRY] =
        g_param_spec_string("country",
                            "Country Code",
                            "",
                            "no-country-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_COUNTRY_NAME] =
        g_param_spec_string("country_name",
                            "Country Name",
                            "",
                            "no-country-name-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_EMAIL] =
        g_param_spec_string("email",
                            "Email",
                            "",
                            "no-email-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_SUBJECT] =
        g_param_spec_string("sub",
                            "Subject",
                            "",
                            "no-sub-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_COMMENT] =
        g_param_spec_string("com",
                            "Comment",
                            "",
                            "no-com-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_RENAMED_FILENAME] =
        g_param_spec_int64("tim",
                           "Renamed filename",
                           "",
                           0,
                           G_MAXINT64,
                           0, /* default value */
                           G_PARAM_READWRITE);
    obj_properties[PROP_FILENAME] =
        g_param_spec_string("filename",
                            "Original filename",
                            "",
                            "no-filename-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_EXT] =
        g_param_spec_string("ext",
                            "Filename extension",
                            "",
                            "no-ext-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_FSIZE] =
        g_param_spec_int64("fsize",
                           "File size",
                           "",
                           0,
                           G_MAXINT64,
                           0, /* default value */
                           G_PARAM_READWRITE);
    obj_properties[PROP_MD5] =
        g_param_spec_string("md5",
                            "File MD5SUM",
                            "24 character base64 MD5",
                            "no-md5-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_WIDTH] =
        g_param_spec_int("w",
                         "Image width",
                         "",
                         0,
                         10000,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_HEIGHT] =
        g_param_spec_int("h",
                         "Image height",
                         "",
                         0,
                         10000,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_THUMBNAIL_WIDTH] =
        g_param_spec_int("tn_w",
                         "Thumbnail width",
                         "",
                         0,
                         250,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_THUMBNAIL_HEIGHT] =
        g_param_spec_int("tn_h",
                         "Thumbnail height",
                         "",
                         0,
                         250,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_FILEDELETED] =
        g_param_spec_int("filedeleted",
                         "File deleted?",
                         "",
                         0,
                         1,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_SPOILER] =
        g_param_spec_int("spoiler",
                         "Spoiler Image?",
                         "",
                         0,
                         1,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_CUSTOM_SPOILER] =
        g_param_spec_int("custom_spoiler",
                         "Custom Spoilers?",
                         "",
                         0,
                         99,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_REPLIES] =
        g_param_spec_int("replies",
                         "Thread Replies",
                         "",
                         0,
                         99999,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_IMAGES] =
        g_param_spec_int("images",
                         "Thread images",
                         "",
                         0,
                         99999,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_BUMPLIMIT] =
        g_param_spec_int("bumplimit",
                         "Thread Bumplimit?",
                         "",
                         0,
                         1,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_IMAGELIMIT] =
        g_param_spec_int("imagelimit",
                         "Thread Imagelimit?",
                         "",
                         0,
                         1,
                         0, /* default value */
                         G_PARAM_READWRITE);
    obj_properties[PROP_IMAGE_URL] =
        g_param_spec_string("image_url",
                            "URL for image",
                            "",
                            "no-image-url-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_THUMB_URL] =
        g_param_spec_string("thumb_url",
                            "URL for thumbnail",
                            "",
                            "no-thumb-url-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_BOARD] =
        g_param_spec_string("board",
                            "Image board",
                            "",
                            "no-board-set", /* default value */
                            G_PARAM_READWRITE);
    obj_properties[PROP_RENDERED] =
        g_param_spec_boolean("rendered",
                             "Whether the post is rendered in a postview",
                             "",
                             FALSE,
                             G_PARAM_READWRITE);

    obj_properties[PROP_THREAD_ID] =
        g_param_spec_int64("thread_id", /* name */
                           "Parent thread id", /* nick */
                           "Id of the thread that contains this post", /* blurb */
                           1  /* minimum value */,
                           9999999999999 /* maximum value */,
                           1  /* default value */,
                           G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class,
                                      N_PROPERTIES,
                                      obj_properties);

}


const gchar *
horizon_post_get_md5(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    if (priv->md5 == NULL)
        return NULL;
    else
        return priv->md5;
}

gint64 horizon_post_get_fsize(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->fsize;
}

gint64 horizon_post_get_post_number(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->post_number;
}

gint64 horizon_post_get_time(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->time;
}

gint64 horizon_post_get_renamed_filename(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->renamed_filename;
}

const gchar *
horizon_post_get_name(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->name;
}

const gchar *
horizon_post_get_tripcode(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->trip;
}

const gchar *
horizon_post_get_subject(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->subject;
}

const gchar *
horizon_post_get_comment(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->comment;
}

const gchar *
horizon_post_get_original_filename(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->filename;
}

const gchar *
horizon_post_get_ext(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->ext;
}

gint64 horizon_post_get_width(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->image_width;
}

gint64 horizon_post_get_height(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->image_height;
}

gint64 horizon_post_get_thumbnail_width(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->thumbnail_width;
}

gint64 horizon_post_get_thumbnail_height(const HorizonPost *post)
{
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->thumbnail_height;
}

gint horizon_post_get_sticky(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->sticky;
}

gint horizon_post_get_closed(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->closed;
}

gint horizon_post_get_deleted(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->is_file_deleted;
}

gint horizon_post_get_spoiler(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->is_spoiler;
}

gint horizon_post_get_replies(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->replies;
}

gint horizon_post_get_images(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->images;
}

gboolean horizon_post_get_bumplimit(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), FALSE);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->bumplimit==1?TRUE:FALSE;
}

gboolean horizon_post_get_imagelimit(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), FALSE);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->imagelimit==1?TRUE:FALSE;
}

const gchar *
horizon_post_get_board(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->board;
}

const gchar *
horizon_post_set_board(HorizonPost *post, const gchar *board) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->board = g_strdup(board);
}

gint64
horizon_post_get_thread_id(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->thread_id;
}

gint64
horizon_post_set_thread_id(HorizonPost *post, const gint64 id) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), 0);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->thread_id = id;
}

const gchar *
horizon_post_get_thumb_url(HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    if (!priv->thumb_url) {
        g_return_val_if_fail(priv->board, NULL);

        priv->thumb_url = g_strdup_printf("https://%s/%s/%"
                                          G_GINT64_FORMAT
                                          "s.jpg",
                                          coldwind_config_get_thumb_cdn(),
                                          priv->board,
                                          priv->renamed_filename);
    }

    return priv->thumb_url;
}

void
horizon_post_set_thumb_url(HorizonPost *post, const gchar* url) {
    g_return_if_fail(HORIZON_IS_POST((HorizonPost*)post));
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    if (priv->thumb_url)
        g_free(priv->thumb_url);

    priv->thumb_url = g_strdup(url);
}

const gchar *
horizon_post_get_image_url(HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), NULL);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    if (!priv->image_url) {
        g_return_val_if_fail(priv->board, NULL);

        priv->image_url = g_strdup_printf("https://%s/%s/"
                                          "%"G_GINT64_FORMAT
                                          "%s",
                                          coldwind_config_get_media_cdn(),
                                          priv->board,
                                          priv->renamed_filename,
                                          priv->ext);
    }

    return priv->image_url;
}

gboolean
horizon_post_is_gif(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), FALSE);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return g_str_has_suffix(priv->ext, "gif");
}

gboolean
horizon_post_has_image(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), FALSE);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->fsize > 0;
}

gboolean
horizon_post_is_rendered(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), FALSE);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->rendered;
}

gboolean
horizon_post_set_rendered(HorizonPost *post, gboolean rendered) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), FALSE);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->rendered = rendered;
}

gboolean
horizon_post_is_same_post(const HorizonPost *left, const HorizonPost *right) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)left), FALSE);
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)right), FALSE);
    HorizonPostPrivate *lpriv = horizon_post_get_instance_private((HorizonPost*)left);
    HorizonPostPrivate *rpriv = horizon_post_get_instance_private((HorizonPost*)right);

    return lpriv->post_number == rpriv->post_number &&
        lpriv->sticky == rpriv->sticky &&
        lpriv->closed == rpriv->closed &&
        lpriv->is_file_deleted == rpriv->is_file_deleted;
}

gboolean
horizon_post_is_not_same_post(const HorizonPost *left, const HorizonPost *right) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)left), FALSE);
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)right), FALSE);
    HorizonPostPrivate *lpriv = horizon_post_get_instance_private((HorizonPost*)left);
    HorizonPostPrivate *rpriv = horizon_post_get_instance_private((HorizonPost*)right);

    return lpriv->post_number != rpriv->post_number ||
        lpriv->sticky != rpriv->sticky ||
        lpriv->closed != rpriv->closed ||
        lpriv->is_file_deleted != rpriv->is_file_deleted;
}

const gchar *
horizon_post_get_capcode(const HorizonPost *post) {
    g_return_val_if_fail(HORIZON_IS_POST((HorizonPost*)post), FALSE);
    HorizonPostPrivate *priv = horizon_post_get_instance_private((HorizonPost*)post);

    return priv->capcode;
}
