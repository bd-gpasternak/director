'''
This module exports a variable named `colors` which has as members all the
colors available in vtkNamedColors.  To see the available colors visit:

    https://htmlpreview.github.io/?https://github.com/Kitware/vtk-examples/blob/gh-pages/VTKNamedColorPatches.html

The color members are accessed using lower case strings, while dict lookup
is case insensitive, for example:

    >>> colors.darkred == colors['DarkRed']
'''
from director import vtkAll as vtk
from director.fieldcontainer import FieldContainer

import numpy as np


def _get_named_colors():
    colors = {}
    named_colors = vtk.vtkNamedColors()
    rgba = list(range(4))
    for name in named_colors.GetColorNames().split():
        named_colors.GetColor(name, rgba)
        colors[name] = np.array(rgba)
    return colors


class ColorsContainer(FieldContainer):
    def __getitem__(self, name):
        return getattr(self, name.lower())


colors = ColorsContainer(**_get_named_colors())
