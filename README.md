# bsp2obj
convert Quake BSP maps to Wavefront OBJ models

## Building

Simply run `make`. This will compile an executable `bsp2obj`.

## Usage

`./bsp2obj example.bsp` creates an `example.obj` with the main geometry and `example_N.obj` files with entity brushes.

## Notes

The material file references textures with relative paths to .jpg files. Textures are not exported automatically.
