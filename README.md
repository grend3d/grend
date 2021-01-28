![screenshot](http://lisp.us.to/data/post7-data/tile-gameplay.png)

A procedural landscape demo to test engine features.

Building
========

You'll need all of the dependencies for grend installed first, see the
![README](https://github.com/grend3d/grend/README.md) there for details.

### In-tree library (for one-off builds)

You can do a local build by initializing the grend repo in the source
root:

    $ git clone --recurse-submodules "https://github.com/grend3d/grend"
	$ mkdir build; cd build
	$ cmake .. -DCMAKE_INSTALL_PREFIX=foobar
	$ make && make install
	$ LD_LIBRARY_PATH=foobar/lib ./foobar/bin/<executable>

### Library install, using pkg-config

You could also install the library global or locally, then use pkg-config
to find build flags:

	$ # asssume the library has already been installed at $GREND_DIR
	$ mkdir build; cd build
	$ PKG_CONFIG_PATH=$GREND_DIR/lib/pkgconfig cmake .. -DCMAKE_INSTALL_PREFIX=foobar
	$ make && make-install
	$ LD_LIBRARY_PATH=$GREND_DIR/lib ./foobar/bin/<executable>
	$ # For a global install you won't need to specify library/config paths.
