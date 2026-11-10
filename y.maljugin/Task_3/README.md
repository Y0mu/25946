Run `make` on Solaris, then run `./setuid_test` from this directory. Create a
local `data.txt` and set its permissions to `600` before the test. The program
prints the real and effective UID, tries to open `data.txt`, drops to the real
UID, and tries again.

To demonstrate the setuid behavior with a second account, arrange access to
the test directory, run the binary first without setuid and then with
`chmod u+s setuid_test`. After the demonstration, run `chmod u-s setuid_test`.
Do not commit the setuid binary or private test data.
