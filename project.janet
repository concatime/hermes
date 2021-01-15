(post-deps
  (import shlex)
  (import sh)
  (import path))

(declare-project
  :name "hermes"
  :author "Andrew Chambers"
  :license "MIT"
  :url "https://github.com/andrewchambers/hermes"
  :repo "git+https://github.com/andrewchambers/hermes.git"
  :dependencies [
    "https://github.com/janet-lang/sqlite3.git"
    "https://github.com/janet-lang/argparse.git"
    "https://github.com/janet-lang/path.git"
    "https://github.com/andrewchambers/janet-uri.git"
    "https://github.com/andrewchambers/janet-jdn.git"
    "https://github.com/andrewchambers/janet-flock.git"
    "https://github.com/andrewchambers/janet-posix-spawn.git"
    "https://github.com/andrewchambers/janet-fork.git"
    "https://github.com/andrewchambers/janet-sh.git"
    "https://github.com/andrewchambers/janet-base16.git"
    "https://github.com/andrewchambers/janet-shlex.git"
  ])

(post-deps

### User config

(def *static-build* (= (or (os/getenv "HERMES_STATIC_BUILD") "no") "yes"))

### End of user config

(def *lib-zlib-cflags*
  (shlex/split
    (sh/$<_ pkg-config --cflags ;(if *static-build* ['--static] []) zlib)))

(def *lib-zlib-lflags*
  (shlex/split
    (sh/$<_ pkg-config --libs ;(if *static-build* ['--static] []) zlib)))

(defn src-file?
  [path]
  (def ext (path/ext path))
  (case ext
    ".janet" true
    ".c" true
    ".h" true
    false))

(def hermes-src
  (->>  (os/dir "./src")
        (map |(string "src/" $))
        (filter src-file?)))

(def hermes-headers
  (filter |(string/has-suffix? ".h" $) hermes-src))

(def signify-src
  (->>  (os/dir "./third-party/signify")
        (map |(string "./third-party/signify/" $))
        (filter src-file?)))

(rule "build/hermes-signify" signify-src
  (eprint "building signify")
  (def wd (os/cwd))
  (defer (os/cd wd)
    (os/cd "./third-party/signify")
    (sh/$ make ;(if *static-build* ["EXTRA_LDFLAGS=--static"] []))
    (sh/$ cp -v signify ../../build/hermes-signify)))

(defn declare-simple-c-prog
  [&keys {
    :name name
    :src src
    :extra-cflags extra-cflags
    :extra-lflags extra-lflags
  }]
  (default extra-cflags [])
  (default extra-lflags [])
  (def out (string "build/" name))
  (rule out src
    (sh/$
      (or (os/getenv "CC") "cc")
      ;extra-cflags
      ;(if *static-build* ["--static"] [])
      ;src
      ;extra-lflags
      "-o" ,out))

  (each h hermes-headers
    (add-dep out h)))

(declare-simple-c-prog
  :name "hermes-tempdir"
  :src ["src/fts.c" "src/hermes-tempdir-main.c"])

(declare-simple-c-prog
  :name "hermes-namespace-container"
  :src ["src/hermes-namespace-container-main.c"])

(declare-native
  :name "_hermes"
  :headers ["src/hermes.h"
            "src/sha1.h"
            "src/sha256.h"
            "src/fts.h"]
  :source ["src/hermes.c"
           "src/scratchvec.c"
           "src/sha1.c"
           "src/sha256.c"
           "src/hash.c"
           "src/pkgfreeze.c"
           "src/deps.c"
           "src/hashscan.c"
           "src/base16.c"
           "src/storify.c"
           "src/os.c"
           "src/unpack2.c"
           "src/tar.c"
           "src/z.c"
           "src/common/err_.c"
           "src/common/strcpy_v.c"
           "src/common/strcpy_vv.c"
           "src/fts.c"]
  :cflags ["-std=c99" "-D" "_POSIX_C_SOURCE=200809L" ;*lib-zlib-cflags*]
  :lflags [;*lib-zlib-lflags*])


(declare-executable
  :name "hermes"
  :entry "src/hermes-main.janet"
  :lflags [;*lib-zlib-lflags*
           ;(if *static-build* ["-static"] [])]
  :deps hermes-src)

(declare-executable
  :name "hermes-pkgstore"
  :entry "src/hermes-pkgstore-main.janet"
  :cflags ["-std=c99"]
  :lflags [;(if *static-build* ["-static"] [])
           ;*lib-zlib-lflags*]
  :deps hermes-src)

(declare-executable
  :name "hermes-builder"
  :entry "src/hermes-builder-main.janet"
  :lflags [;(if *static-build* ["-static"] [])
           ;*lib-zlib-lflags*]
  :deps hermes-src)

(each bin ["hermes" "hermes-pkgstore" "hermes-builder"]
  (def bin (string "build/" bin))
  (add-dep bin "build/_hermes.so")
  (add-dep bin "build/_hermes.a")
  (add-dep bin "build/_hermes.meta.janet"))

(map |(add-dep "build" $)
  (map |(string "build/" $)
    [ "hermes"
      "hermes-pkgstore"
      "hermes-builder"
      "hermes-tempdir"
      "hermes-signify"
      "hermes-namespace-container" ]))

(phony "clean-third-party" []
  (def wd (os/cwd))
  (defer (os/cd wd)
    (os/cd "./third-party/signify")
    (sh/$ make clean > :null)))


(add-dep "clean" "clean-third-party")

)
