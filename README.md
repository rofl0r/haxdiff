haxdiff
=======

a simple format and implementation for human-readable binary diffs.

format description
------------------

the haxdiff/1.0 format looks pretty much like a standard unified text patch
(as produced with diff -u), and can therefore make use of existing
syntax highlighting facilities in an editor (e.g. `nano`).

a patch consists of one or more hunks, with the following layout:

- a hunk header, starting with `@@ `, followed by the offset, `,-`,
  number of bytes removed, `,+` number of bytes inserted.
  all 3 values are in hexadecimal without leading `0x`.
  example: `@@ 17b0,-4,+4`
- zero or more lines starting with `- ` and a hexadecimal byte sequence.
  if provided, the total number of bytes in all `- ` sequences must be
  identical to the number specified for `-` in the hunk header.
  the total length of each line shall not exceed 1000 bytes, but it is
  advisable to keep it to 80 chars for readability.
  the `minus` byte sequence denotes the bytes that will be overwritten
  by the patch, and its purpose is to make sure the input file is in
  the expected format, plus it allows a reader to see the actual
  difference and even to manually apply a patch using a hexeditor.
  the `-` lines are optional and may be omitted.
- one or more lines starting with `+ ` and a hexadecimal byte sequence.
  the same rules mentioned for the minus lines apply, except that these
  are not optional.
- if the modified file M is bigger than the original file O, the last hunk
  header shall have the form of `@@ offset,-0,+len` where offset is identical
  to O's filesize, and len the difference between O and M (aka M size-O size).
  it shall be followed by only `+ ` lines describing all the additional
  bytes of M.
- if the modified file M is smaller than the original file O, the last hunk
  header shall have the form of `@@ offset,-len,+0` where offset is identical
  to M's filesize, and len the difference between M and O (aka O size-M size).
  the header may be followed by only `- ` lines describing all the removed
  bytes.
- lines not starting with `@`, `-` and `+` shall be ignored.
- lines can be terminated by either a single `\n` or `\r\n` characters.
- all hexadecimal bytes shall be expressed with lower-case characters for
  the range `a-f`.
- all hunks shall be sorted by ascending offset.

full example:

```diff
@@ 17b0,-4,+4
- 04020004
+ 00000000
@@ 3dc14,-4,+4
- 04020004
+ 00000000
@@ b666c,-8,+8
- 0e48396801600e48
+ 0048004701bb3e08
@@ 3ebcb0,-8,+0
- ffffffffffffffff
```

advantages and disadvantages of the format
------------------------------------------
compared with other binary patch solutions, this format/impl has the
following advantages:

- human readable and simple
- patch can even be applied by hand using a hexeditor
- reuses syntax highlighting capabilities of text editors
- patching and diffing are really fast, making a diff of a 4MB rom completes
  in 0.0 seconds.
- no limits for filesize of patched file
- supports truncation and expansion
- can optionally verify that the file to be patched matches the patch
- creating a haxdiff and looking at it in an editor with syntax highlighting
  for standard diffs pretty much replaces all usecases for tools like
  `vbindiff`.
- as only 16 hexadecimal characters, `@-+,` and newline characters are
  used, standard compression tools can very efficiently compress these
  patches. for example a 172kb patch used for testing compresses to less than
  1KB using gzip.
- written in C, no external dependencies.

disadvantages:

- uncompressed haxdiff patches are naturally bigger than patches in binary
  format, though it is likely they are similar in size or smaller when
  compressed.
- while the format theoretically allows to insert more or less bytes than
  replaced, this isn't implemented in the reference implementation.
  it is therefore currently not that useful for some usecases like complex
  executable file deltas (for example upgrading a big application).
- in the ref. implementation there are no award-winning algorithms used that
  e.g. detect whether some data has been put in front of the patched file.

reference implementation
------------------------

the `haxdiff` reference implementation allows both to create and apply patches.
patches are always created including the optional `-` lines, so a patch's
consistency can be controlled.
it currently does not allow for hunks that have differing `+` and `-` - except
for truncation and expansion cases.

license
-------

the haxdiff reference implementation is (C) 2021 rofl0r and licensed as MIT.
the format itself is licensed as public domain.

compilation
-----------

run `make`.
if you want to use specific CFLAGS, you can use either of
`make CFLAGS="-O2 -g2"` or `echo "CFLAGS = -O0 -g3" > config.mak`.

usage
-----

    haxdiff MODE FILE1 FILE2

    creates or applies a binary diff.

    MODE: d - diff, p - patch, P - force patch

    in mode d, create a diff between files FILE1 and FILE2,
    and write a patch to stdout.
    the generated patch describes how to generate FILE2 from FILE1.
    in patch mode, FILE1 denotes the file to be patched, and FILE2
    the patched file that will be written (e.g. output file).
    the patch itself is read from stdin.
    if mode is p, the replaced bytes must match those described in
    the patch. if mode is P, the replaced bytes will be ignored.
    if mode is d, the patch will be written to stdout.

