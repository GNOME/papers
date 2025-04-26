# Build on MacOS

This is a rough guide about building and packaging Papers to a standalone package on MacOS. We are not talking about how to build a homebrew or macports package.

Thanks to the [gtk-osx-build](https://gitlab.gnome.org/GNOME/gtk-osx) and [gtk-mac-bunlder](https://gitlab.gnome.org/GNOME/gtk-mac-bundler) project, we don't need to build standalone package from scratch. So the whole process can be summarized as:

1. Setup `gtk-osx-build` and `gtk-mac-bundler`.
2. Edit the configuration of `jhbuild` and points the module set to the one we provided.
3. Build and install Papers through jhbuild.
4. Use gtk-mac-bundler to bundle the Papers and its dependencies into the Papers.app.
5. Run some predefined scripts to turn Papers.app into Papers.dmg.

WARNING: `gtk-osx-build` is based on jhbuild and is not compatible with homebrew. Rename your homebrew directory before bootstrapping `gtk-osx-build`. The best practice is creating a new user and doing all the work with it.

The essential configuration of `jhbuildrc-custom` are:

```python
modulesets_dir = "/Users/papersdev/papers/build-aux/"
use_local_modulesets = True
moduleset = "papers"
```

## File Structure

Here is the descriptions of files inside this directory:

| file/directory |                         description                          |
| :------------: | :----------------------------------------------------------: |
| papers.module  | The predefined module set file for `jhbuild`, which describes how to build Papers and its dependencies |
|  Papers.icns   |           The icons of Papers to build the bundle            |
|   Info.plist   |                  The metadata of the bundle                  |
| papers.bundle  |             The input file of `gtk-mac-bundler`              |
|    patches/    |              Patches used by the jhbuild module              |
|     dmg.sh     |        A sample script to build the Papers.dmg image         |
|    Makefile    |                      A sample Makefile                       |

## Feature Matrix

| Document Type | Support | Registered as Handler |
| :-----------: | :-----: | :-------------------: |
|      pdf      |    ✓    |           ✓           |
| cbz, cbr, cb7 |    ✓    |           ✓           |
|     djvu      |    ✓    |                       |
|   tiff, tif   |    ✓    |                       |
