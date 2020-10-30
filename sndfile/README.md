If the Xcode build setting `CLANG_ENABLE_MODULES` is true compilation
fails because the Darwin module imports bsm which also defines a function
called `au_open()`:

```
In file included from /Users/sbooth/Development/AudioXCFrameworks/sndfile/libsndfile-1.0.28/src/ogg_opus.c:35:
/Users/sbooth/Development/AudioXCFrameworks/sndfile/libsndfile-1.0.28/src/common.h:873:6: error: conflicting types for 'au_open'
int             au_open         (SF_PRIVATE *psf) ;
                ^
In module 'Darwin' imported from /Users/sbooth/Development/AudioXCFrameworks/sndfile/libsndfile-1.0.28/src/ogg_opus.c:22:
/Users/sbooth/Downloads/Xcode-beta.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX11.0.sdk/usr/include/bsm/audit_record.h:205:10: note: previous declaration is here
int      au_open(void)
```

Rather than trying to figure out a cause I disabled modules for this project.
