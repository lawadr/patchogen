# Patchogen

Patchogen is a simple generator of self-contained native patch executables to transform one version of a directory hierarchy into another.

## Build

Let us assume the following:

* Name of the product: *ExampleApp*
* Name of the resulting patch executable: *ExampleAppPatch*
* Old version number: *1.0*
* Directory path of the old version: */path/to/old/version/*
* New version number: *1.1*
* Directory path of the new version: */path/to/new/version/*

A patch executable can be generated with the following CMake commands:

```
cmake -DPRODUCT_NAME=ExampleApp -DEXECUTABLE_NAME=ExampleAppPatch -DOLD_VERSION=1.0 -DOLD_PATH=/path/to/old/version/ -DNEW_VERSION=1.1 -DNEW_PATH=/path/to/new/version/ /path/to/patchogen/
cmake --build .
```

## Usage

The resulting patch executable can be run from a directory containing a copy of the old version to transform it into a copy of the new version.

Alternatively, the path to a copy of the old version can be specified as the sole command line argument:

```
./ExampleAppPatch /path/to/old/version/
```
