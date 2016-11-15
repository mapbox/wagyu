## Building and Testing

### Requirements

* Git
* make
* clang++ 3.5 or later or g++-5 or later

#### OS X

* Xcode Command Line Tools

### Steps

1. Clone the waygu repo using git and initialize the git submodules.

```
git clone https://github.com/mapbox/wagyu.git
cd wagyu
git submodule update --init
```

This will install Mapbox's `mason` package manager as well as [test fixtures](https://github.com/mapnik/geometry-test-data). `mason` is going to install required headers, such as `boost`, `rapidjson`, and `mapbox/geometry`.

2. The default `make` builds and runs the tests (`make test`).

```
make
```

`make` will build the library with [Catch](https://github.com/philsquared/Catch) test executables. It will then run the Catch unit tests. If they all pass, you should see something like:

```
All tests passed (178 assertions in 13 test cases)
```

Then, it will run the `fixture-tester` executable with all of the geometry tests. If all of these pass, you should see something like:

```
./tests/run-geometry-tests.sh ./fixture-tester
 ✓ 2900/2900  ✗ 0/2900
```

 `run-geometry-tests.sh` is feeding `fixture-tester` all of the input polyjson polygons we have in the test fixtures. The `✓` shows how many fixtures passed, and the `✗` shows how many fixtures failed.

 That's it, you have now built and tested `wagyu`!

### Including in External Project

You may want to use wagyu in your own project. Since wagyu is a header-only library, all you have to do is include `mapbox/geometry/polygon.hpp` and `mapbox/geometry/wagyu/wagyu.hpp`. `polygon.hpp` is the polygon portion of the Mapbox geometry library, a boost-compliant polygon geometry container.

### Debugging

`wagyu` has many `DEBUG` flags [throughout the code](https://github.com/mapbox/wagyu/blob/79d85c720c8fb9ab37d0b677ccf12f83d1015ad7/include/mapbox/geometry/wagyu/local_minimum.hpp#L56-L113) that will help you make sense of the library and what it is doing. To see log messages during execution of the code:

```
make debug
```

