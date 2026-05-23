# pymsym

[![PyPI version](https://badge.fury.io/py/pymsym.svg)](https://pypi.python.org/pypi/pymsym/)
[![Downloads](https://img.shields.io/pypi/dm/pymsym.svg)](https://pypi.python.org/pypi/pymsym/)

libmsym is a C library dealing with point group symmetry in molecules. pymsym is its python interface.

## about
this library was originally developed by Marcus Johansson. the original library can be found [here](https://github.com/mcodev31/libmsym) and seems unmaintained - after reaching out to the initial authors, i've decided to fork this so i can maintain it.

for more info, read [the blog post](https://corinwagen.github.io/public/blog/20240425_pymsym.html).

## installation

use pip:

```
pip install pymsym
```

## api

there are two high-level functions, ``pymsym.get_point_group()`` and ``pymsym.get_symmetry_number()``.

here's a simple introduction:

```
import pymsym

# water
atomic_numbers = [8, 1, 1]
positions = [
  [0.007544053252786398, 0.39774343371391296, 0.0],
  [-0.7671031355857849, -0.18439316749572754, 0.0],
  [0.7595590949058533, -0.21335026621818542, 0.0]
 ]

pymsym.get_point_group(atomic_numbers, positions) # returns C2v
print(pymsym.get_symmetry_number(atomic_numbers, positions)) # returns 2
```

the original lower-level classes from libmsym can also be accessed (``pymsym.Element``, etc). 

## point groups

here's all the point groups that pymsym can handle. some of these haven't been tested very well, please let me know if there are issues. 

```
point_groups = [
    "C1", "Cs", "Ci", "C2", "C3", "C4", "C5", "C6", "C7", "C8",
    "D2", "D3", "D4", "D5", "D6", "D7", "D8", "C2v", "C3v", "C4v",
    "C5v", "C6v", "C7v", "C8v", "C2h", "C3h", "C4h", "C5h", "C6h",
    "C7h", "C8h", "D2h", "D3h", "D4h", "D5h", "D6h", "D7h", "D8h",
    "D2d", "D3d", "D4d", "D5d", "D6d", "D7d", "D8d", "S4", "S6",
    "S8", "T", "Th", "Td", "O", "Oh", "Cinfv", "Dinfh", "I", "Kh"
]
```


*Corin Wagen, 2024*
