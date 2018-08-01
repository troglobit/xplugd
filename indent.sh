#!/bin/sh
#
# Don't touch comment content/formatting, only placement.
# ignore any ~/.indent.pro
#
# With the -T <type> we can inform indent about non-ansi types
# that we've added, so indent doesn't insert spaces in odd places.
#
indent -linux                                                                   \
       --ignore-profile                                                         \
       --preserve-mtime                                                         \
       --break-after-boolean-operator                                           \
       --blank-lines-after-procedures                                           \
       --blank-lines-after-declarations                                         \
       --dont-break-function-decl-args                                          \
       --dont-break-procedure-type                                              \
       --leave-preprocessor-space                                               \
       --line-length132                                                         \
       --honour-newlines                                                        \
       --space-after-if                                                         \
       --space-after-for                                                        \
       --space-after-while                                                      \
       --leave-optional-blank-lines                                             \
       --dont-format-comments                                                   \
       --no-blank-lines-after-commas                                            \
       --no-space-after-parentheses                                             \
       --no-space-after-casts                                                   \
       -T size_t -T timeval_t -T pid_t -T pthread_t -T FILE                     \
       -T time_t -T uint32_t -T uint16_t -T uint8_t -T uchar -T uint -T ulong   \
       -T Display -T XRRScreenResources -T XRROutputChangeNotifyEvent -T XEvent \
       $*
