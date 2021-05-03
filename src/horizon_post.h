#ifndef HORIZON_POST_H
#define HORIZON_POST_H

#include <glib-object.h>

G_BEGIN_DECLS

#define HORIZON_TYPE_POST                  (horizon_post_get_type ())
G_DECLARE_DERIVABLE_TYPE (HorizonPost, horizon_post, HORIZON, POST, GObject);

struct _HorizonPostClass
{
    GObjectClass   parent_class;
};


const gchar * horizon_post_get_name              (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_tripcode          (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_comment           (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_subject           (const HorizonPost *post) G_GNUC_PURE;
gint64        horizon_post_get_time              (const HorizonPost *post) G_GNUC_PURE;
gint64        horizon_post_get_post_number       (const HorizonPost *post) G_GNUC_PURE;
gint64        horizon_post_get_renamed_filename  (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_original_filename (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_ext               (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_md5               (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_thumb_url         (HorizonPost *post);
void          horizon_post_set_thumb_url         (HorizonPost *post, const gchar*);
const gchar * horizon_post_get_image_url         (HorizonPost *post);
gint64        horizon_post_get_thumbnail_width   (const HorizonPost *post) G_GNUC_PURE;
gint64        horizon_post_get_thumbnail_height  (const HorizonPost *post) G_GNUC_PURE;
gint64        horizon_post_get_width             (const HorizonPost *post) G_GNUC_PURE;
gint64        horizon_post_get_height            (const HorizonPost *post) G_GNUC_PURE;
gint64        horizon_post_get_fsize             (const HorizonPost *post) G_GNUC_PURE;
gboolean      horizon_post_has_image             (const HorizonPost *post) G_GNUC_PURE;
gint          horizon_post_get_sticky            (const HorizonPost *post) G_GNUC_PURE;
gint          horizon_post_get_closed            (const HorizonPost *post) G_GNUC_PURE;
gint          horizon_post_get_deleted           (const HorizonPost *post) G_GNUC_PURE;
gint          horizon_post_get_spoiler           (const HorizonPost *post) G_GNUC_PURE;
gint          horizon_post_get_replies           (const HorizonPost *post) G_GNUC_PURE;
gint          horizon_post_get_images            (const HorizonPost *post) G_GNUC_PURE;
gboolean      horizon_post_get_bumplimit         (const HorizonPost *post) G_GNUC_PURE;
gboolean      horizon_post_get_imagelimit        (const HorizonPost *post) G_GNUC_PURE;
gboolean      horizon_post_is_gif                (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_get_board             (const HorizonPost *post) G_GNUC_PURE;
const gchar * horizon_post_set_board             (HorizonPost *post, const gchar *board) G_GNUC_MALLOC;
gint64        horizon_post_get_thread_id         (const HorizonPost *post) G_GNUC_PURE;
gint64        horizon_post_set_thread_id         (HorizonPost *post, const gint64 id);
gboolean      horizon_post_set_rendered          (HorizonPost *post, const gboolean rendered);
gboolean      horizon_post_is_rendered           (const HorizonPost *post) G_GNUC_PURE;
gboolean      horizon_post_is_same_post          (const HorizonPost *left, const HorizonPost *right) G_GNUC_PURE;
gboolean      horizon_post_is_not_same_post      (const HorizonPost *left, const HorizonPost *right) G_GNUC_PURE;
const gchar * horizon_post_get_capcode           (const HorizonPost *post) G_GNUC_PURE;

G_END_DECLS

#endif
