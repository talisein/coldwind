#include "config.h"

const char * coldwind_config_get_thumb_cdn(void)
{
    return config::FOURCHAN_THUMB_CDN.c_str();
}

const char * coldwind_config_get_media_cdn(void)
{
    return config::FOURCHAN_MEDIA_CDN.c_str();
}

const char * coldwind_config_get_JSON_cdn(void)
{
    return config::FOURCHAN_JSON_CDN.c_str();
}
