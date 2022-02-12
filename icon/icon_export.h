
#ifndef ICON_EXPORT_H
#define ICON_EXPORT_H

#ifdef ICON_STATIC_DEFINE
#  define ICON_EXPORT
#  define ICON_NO_EXPORT
#else
#  ifndef ICON_EXPORT
#    ifdef icon_EXPORTS
        /* We are building this library */
#      define ICON_EXPORT 
#    else
        /* We are using this library */
#      define ICON_EXPORT 
#    endif
#  endif

#  ifndef ICON_NO_EXPORT
#    define ICON_NO_EXPORT 
#  endif
#endif

#ifndef ICON_DEPRECATED
#  define ICON_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef ICON_DEPRECATED_EXPORT
#  define ICON_DEPRECATED_EXPORT ICON_EXPORT ICON_DEPRECATED
#endif

#ifndef ICON_DEPRECATED_NO_EXPORT
#  define ICON_DEPRECATED_NO_EXPORT ICON_NO_EXPORT ICON_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef ICON_NO_DEPRECATED
#    define ICON_NO_DEPRECATED
#  endif
#endif

#endif /* ICON_EXPORT_H */
