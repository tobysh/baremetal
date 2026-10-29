Reserve the extra byte first: the multiplication must be at most `SIZE_MAX - 1`.
===
Handle `width == 0` before division, then reject when `count > (SIZE_MAX - 1) / width`.
