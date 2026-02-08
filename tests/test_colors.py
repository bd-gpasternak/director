import numpy as np

from director.colors import colors


def test_colors_case_insensitive_lookup():
    assert np.array_equal(colors.darkred, colors["DarkRed"])
