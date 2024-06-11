# Minos Concurrent Skip List ![Project Logo](prince_of_liles.png)




## Usage

You can include the code below in your `cmake` project and `#include <skiplist.h>` in your sources.

``` cmake
FetchContent_Declare(minos
                     GIT_REPOSITORY https://github.com/gesalous/minos)
FetchContent_GetProperties(minos)
if(NOT minos_POPULATED)
  FetchContent_Populate(minos)
  add_subdirectory(${minos_SOURCE_DIR} ${minos_BINARY_DIR})
  include_directories(${minos_SOURCE_DIR}/include)
  FetchContent_MakeAvailable(minos)
endif()
target_link_libraries(YOUR_LIB_NAME minos)
```

## Build, Testing and Installation

``` sh
cd minos
mkdir build;cd build
cmake ..
make

# To run tests
ctest --verbose

# To install
sudo make install
```

## Bibliography

### The implementation of Minos is based on the following papers

1. William Pugh. 1990. Skip lists: a probabilistic alternative to balanced trees. Commun. ACM 33, 6 (June 1990), 668–676. https://doi.org/10.1145/78973.78977
2. William Pugh. 1990. Concurrent maintenance of skip lists. Technical Report. University of Maryland at College Park, USA.

