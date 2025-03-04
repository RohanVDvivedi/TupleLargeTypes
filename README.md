# TupleLargeTypes

A sibling project of TupleIndexer (for worm) and TupleStore, to build larger than page data types with comparable prefixes.

## Setup instructions
**Install dependencies :**
 * [TupleStore](https://github.com/RohanVDvivedi/TupleStore)
 * [TupleIndexer](https://github.com/RohanVDvivedi/TupleIndexer)

**Download source code :**
 * `git clone https://github.com/RohanVDvivedi/TupleLargeTypes.git`

**Build from source :**
 * `cd TupleLargeTypes`
 * `make clean all`

**Install from the build :**
 * `sudo make install`
 * ***Once you have installed from source, you may discard the build by*** `make clean`

## Using The library
 * add `-ltuplelargetypes -ltupleindexer -ltuplestore -lcutlery` linker flag, while compiling your application
 * do not forget to include appropriate public api headers as and when needed. this includes
   * `#include<blob_large.h>`
   * `#include<text_large.h>`

## Instructions for uninstalling library

**Uninstall :**
 * `cd TupleLargeTypes`
 * `sudo make uninstall`
