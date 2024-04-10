# Point data conversion plugin ![Build Status](https://github.com/ManiVaultStudio/PointDataConversionPlugin/actions/workflows/build.yml/badge.svg?branch=main)

Point data conversion plugin for the [ManiVault](https://github.com/ManiVaultStudio/core) visual analytics framework.

```bash
git clone git@github.com:ManiVaultStudio/PointDataConversionPlugin.git
```

Applies an element-wise transformation to a selected dataset in-place.

Implemented transformations:
- [Inverse hyperbolic sine](https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions) (`asinh`)
- [Binary logarithm](https://en.wikipedia.org/wiki/Binary_logarithm) (`log2`)

## How to use
- Right-click on a point dataset and select `Transform` -> `Conversion` and chose the transformation function
