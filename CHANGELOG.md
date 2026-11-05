# Changelog

## v0.1.0 (first public release)

- Header-only C17 library in `Core/` (libc + libm only): noise generator, RK4 solver with five test systems, least squares, candidate term library, STLSQ sparse regression, finite-difference and Savitzky-Golay derivatives, uneven-clock repair (linear, Hermite, cubic spline, PCHIP), CSV loader, and an experimental neural "network is the curve" derivative estimator.
- `EPnC_App`: Raylib GUI. Drop a `.csv` (columns `t,x,y`) or pick a demo, watch STLSQ throw out terms round by round, and check the discovered equation by simulating it over the data. Three themes, adjustable text size, screenshot button.
- 11 test programs in `tests/` (`test_01` to `test_12`, `test_04` was retired), each with `-h` help.
- Two example files in `examples/`.

Known limits: see "Limitations" in the README.
