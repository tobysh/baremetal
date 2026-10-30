Read headers with tar.NewReader and stop only on io.EOF; propagate other errors.
===
Inspect Header.Mode directly using Unix octal masks. Do not use os.FileMode SUID constants against the tar numeric mode.
